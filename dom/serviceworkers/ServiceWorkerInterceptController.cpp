/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ServiceWorkerInterceptController.h"

#include "mozilla/BasePrincipal.h"
#include "mozilla/StorageAccess.h"
#include "nsContentUtils.h"
#include "nsIChannel.h"
#include "ServiceWorkerManager.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS(ServiceWorkerInterceptController,
                  nsINetworkInterceptController)

NS_IMETHODIMP
ServiceWorkerInterceptController::ShouldPrepareForIntercept(
    nsIURI* aURI, nsIChannel* aChannel, bool* aShouldIntercept) {
  *aShouldIntercept = false;

  nsCOMPtr<nsILoadInfo> loadInfo = aChannel->LoadInfo();

  RefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();

  // For subresource requests we base our decision solely on the client's
  // controller value.  Any settings that would have blocked service worker
  // access should have been set before the initial navigation created the
  // window.
  if (!nsContentUtils::IsNonSubresourceRequest(aChannel)) {
    const Maybe<ServiceWorkerDescriptor>& controller =
        loadInfo->GetController();
    // For child intercept, only checking the loadInfo controller existence.
    if (!ServiceWorkerParentInterceptEnabled()) {
      *aShouldIntercept = controller.isSome();
      return NS_OK;
    }

    // If the controller doesn't handle fetch events, return false
    if (controller.isSome()) {
      *aShouldIntercept = controller.ref().HandlesFetch();

      // The service worker has no fetch event handler, try to schedule a
      // soft-update through ServiceWorkerRegistrationInfo.
      // Get ServiceWorkerRegistrationInfo by the ServiceWorkerInfo's principal
      // and scope
      if (!*aShouldIntercept && swm) {
        RefPtr<ServiceWorkerRegistrationInfo> registration =
            swm->GetRegistration(controller.ref().GetPrincipal().get(),
                                 controller.ref().Scope());
        MOZ_ASSERT(registration);
        registration->MaybeScheduleTimeCheckAndUpdate();
      }
    } else {
      *aShouldIntercept = false;
    }
    return NS_OK;
  }

  nsCOMPtr<nsIPrincipal> principal = BasePrincipal::CreateContentPrincipal(
      aURI, loadInfo->GetOriginAttributes());

  // First check with the ServiceWorkerManager for a matching service worker.
  if (!swm || !swm->IsAvailable(principal, aURI, aChannel)) {
    return NS_OK;
  }

  // Then check to see if we are allowed to control the window.
  // It is important to check for the availability of the service worker first
  // to avoid showing warnings about the use of third-party cookies in the UI
  // unnecessarily when no service worker is being accessed.
  if (StorageAllowedForChannel(aChannel) != StorageAccess::eAllow) {
    return NS_OK;
  }

  *aShouldIntercept = true;
  return NS_OK;
}

NS_IMETHODIMP
ServiceWorkerInterceptController::ChannelIntercepted(
    nsIInterceptedChannel* aChannel) {
  // Note, do not cancel the interception here.  The caller will try to
  // ResetInterception() on error.

  RefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
  if (!swm) {
    return NS_ERROR_FAILURE;
  }

  ErrorResult error;
  swm->DispatchFetchEvent(aChannel, error);
  if (NS_WARN_IF(error.Failed())) {
    return error.StealNSResult();
  }

  return NS_OK;
}

}  // namespace dom
}  // namespace mozilla

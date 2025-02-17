/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef HttpConnectionMgrShell_h__
#define HttpConnectionMgrShell_h__

#include "nsISupports.h"

class nsIEventTarget;
class nsIHttpUpgradeListener;
class nsIInterfaceRequestor;

namespace mozilla {
namespace net {

class ARefBase;
class EventTokenBucket;
class HttpTransactionShell;
class nsHttpConnectionInfo;
class HttpConnectionBase;
class nsHttpConnectionMgr;
class NullHttpTransaction;

//----------------------------------------------------------------------------
// Abstract base class for HTTP connection manager in chrome process
//----------------------------------------------------------------------------

// f5379ff9-2758-4bec-9992-2351c258aed6
#define HTTPCONNECTIONMGRSHELL_IID                   \
  {                                                  \
    0xf5379ff9, 0x2758, 0x4bec, {                    \
      0x99, 0x92, 0x23, 0x51, 0xc2, 0x58, 0xae, 0xd6 \
    }                                                \
  }

class HttpConnectionMgrShell : public nsISupports {
 public:
  NS_DECLARE_STATIC_IID_ACCESSOR(HTTPCONNECTIONMGRSHELL_IID)

  enum nsParamName : uint32_t {
    MAX_URGENT_START_Q,
    MAX_CONNECTIONS,
    MAX_PERSISTENT_CONNECTIONS_PER_HOST,
    MAX_PERSISTENT_CONNECTIONS_PER_PROXY,
    MAX_REQUEST_DELAY,
    THROTTLING_ENABLED,
    THROTTLING_SUSPEND_FOR,
    THROTTLING_RESUME_FOR,
    THROTTLING_READ_LIMIT,
    THROTTLING_READ_INTERVAL,
    THROTTLING_HOLD_TIME,
    THROTTLING_MAX_TIME,
    PROXY_BE_CONSERVATIVE
  };

  MOZ_MUST_USE virtual nsresult Init(
      uint16_t maxUrgentExcessiveConns, uint16_t maxConnections,
      uint16_t maxPersistentConnectionsPerHost,
      uint16_t maxPersistentConnectionsPerProxy, uint16_t maxRequestDelay,
      bool throttleEnabled, uint32_t throttleVersion,
      uint32_t throttleSuspendFor, uint32_t throttleResumeFor,
      uint32_t throttleReadLimit, uint32_t throttleReadInterval,
      uint32_t throttleHoldTime, uint32_t throttleMaxTime,
      bool beConservativeForProxy) = 0;

  MOZ_MUST_USE virtual nsresult Shutdown() = 0;

  // called from main thread to post a new request token bucket
  // to the socket thread
  MOZ_MUST_USE virtual nsresult UpdateRequestTokenBucket(
      EventTokenBucket* aBucket) = 0;

  // Close all idle persistent connections and prevent any active connections
  // from being reused. Optional connection info resets CI specific
  // information such as Happy Eyeballs history.
  MOZ_MUST_USE virtual nsresult DoShiftReloadConnectionCleanup(
      nsHttpConnectionInfo*) = 0;

  // called to force the connection manager to prune its list of idle
  // connections.
  MOZ_MUST_USE virtual nsresult PruneDeadConnections() = 0;

  // this cancels all outstanding transactions but does not shut down the mgr
  virtual void AbortAndCloseAllConnections(int32_t, ARefBase*) = 0;

  // called to update a parameter after the connection manager has already
  // been initialized.
  MOZ_MUST_USE virtual nsresult UpdateParam(nsParamName name,
                                            uint16_t value) = 0;

  // Causes a large amount of connection diagnostic information to be
  // printed to the javascript console
  virtual void PrintDiagnostics() = 0;

  virtual nsresult UpdateCurrentTopLevelOuterContentWindowId(
      uint64_t aWindowId) = 0;

  // adds a transaction to the list of managed transactions.
  MOZ_MUST_USE virtual nsresult AddTransaction(HttpTransactionShell*,
                                               int32_t priority) = 0;

  // Add a new transaction with a sticky connection from |transWithStickyConn|.
  MOZ_MUST_USE virtual nsresult AddTransactionWithStickyConn(
      HttpTransactionShell* trans, int32_t priority,
      HttpTransactionShell* transWithStickyConn) = 0;

  // called to reschedule the given transaction.  it must already have been
  // added to the connection manager via AddTransaction.
  MOZ_MUST_USE virtual nsresult RescheduleTransaction(HttpTransactionShell*,
                                                      int32_t priority) = 0;

  void virtual UpdateClassOfServiceOnTransaction(HttpTransactionShell*,
                                                 uint32_t classOfService) = 0;

  // cancels a transaction w/ the given reason.
  MOZ_MUST_USE virtual nsresult CancelTransaction(HttpTransactionShell*,
                                                  nsresult reason) = 0;

  // called when a connection is done processing a transaction.  if the
  // connection can be reused then it will be added to the idle list, else
  // it will be closed.
  MOZ_MUST_USE virtual nsresult ReclaimConnection(HttpConnectionBase* conn) = 0;

  // called to force the transaction queue to be processed once more, giving
  // preference to the specified connection.
  MOZ_MUST_USE virtual nsresult ProcessPendingQ(nsHttpConnectionInfo*) = 0;

  // Try and process all pending transactions
  MOZ_MUST_USE virtual nsresult ProcessPendingQ() = 0;

  // called to get a reference to the socket transport service.  the socket
  // transport service is not available when the connection manager is down.
  MOZ_MUST_USE virtual nsresult GetSocketThreadTarget(nsIEventTarget**) = 0;

  // called to indicate a transaction for the connectionInfo is likely coming
  // soon. The connection manager may use this information to start a TCP
  // and/or SSL level handshake for that resource immediately so that it is
  // ready when the transaction is submitted. No obligation is taken on by the
  // connection manager, nor is the submitter obligated to actually submit a
  // real transaction for this connectionInfo.
  MOZ_MUST_USE virtual nsresult SpeculativeConnect(
      nsHttpConnectionInfo*, nsIInterfaceRequestor*, uint32_t caps = 0,
      NullHttpTransaction* = nullptr) = 0;

  // "VerifyTraffic" means marking connections now, and then check again in
  // N seconds to see if there's been any traffic and if not, kill
  // that connection.
  MOZ_MUST_USE virtual nsresult VerifyTraffic() = 0;

  virtual void BlacklistSpdy(const nsHttpConnectionInfo* ci) = 0;

  // clears the connection history mCT
  MOZ_MUST_USE virtual nsresult ClearConnectionHistory() = 0;

  // called by the main thread to execute the taketransport() logic on the
  // socket thread after a 101 response has been received and the socket
  // needs to be transferred to an expectant upgrade listener such as
  // websockets.
  // @param aTrans: a transaction that contains a sticky connection. We'll
  //                take the transport of this connection.
  MOZ_MUST_USE virtual nsresult CompleteUpgrade(
      HttpTransactionShell* aTrans,
      nsIHttpUpgradeListener* aUpgradeListener) = 0;

  virtual nsHttpConnectionMgr* AsHttpConnectionMgr() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(HttpConnectionMgrShell,
                              HTTPCONNECTIONMGRSHELL_IID)

#define NS_DECL_HTTPCONNECTIONMGRSHELL                                       \
  virtual nsresult Init(                                                     \
      uint16_t maxUrgentExcessiveConns, uint16_t maxConnections,             \
      uint16_t maxPersistentConnectionsPerHost,                              \
      uint16_t maxPersistentConnectionsPerProxy, uint16_t maxRequestDelay,   \
      bool throttleEnabled, uint32_t throttleVersion,                        \
      uint32_t throttleSuspendFor, uint32_t throttleResumeFor,               \
      uint32_t throttleReadLimit, uint32_t throttleReadInterval,             \
      uint32_t throttleHoldTime, uint32_t throttleMaxTime,                   \
      bool beConservativeForProxy) override;                                 \
  virtual nsresult Shutdown() override;                                      \
  virtual nsresult UpdateRequestTokenBucket(EventTokenBucket* aBucket)       \
      override;                                                              \
  virtual nsresult DoShiftReloadConnectionCleanup(nsHttpConnectionInfo*)     \
      override;                                                              \
  virtual nsresult PruneDeadConnections() override;                          \
  virtual void AbortAndCloseAllConnections(int32_t, ARefBase*) override;     \
  virtual nsresult UpdateParam(nsParamName name, uint16_t value) override;   \
  virtual void PrintDiagnostics() override;                                  \
  virtual nsresult UpdateCurrentTopLevelOuterContentWindowId(                \
      uint64_t aWindowId) override;                                          \
  virtual nsresult AddTransaction(HttpTransactionShell*, int32_t priority)   \
      override;                                                              \
  virtual nsresult AddTransactionWithStickyConn(                             \
      HttpTransactionShell* trans, int32_t priority,                         \
      HttpTransactionShell* transWithStickyConn) override;                   \
  virtual nsresult RescheduleTransaction(HttpTransactionShell*,              \
                                         int32_t priority) override;         \
  void virtual UpdateClassOfServiceOnTransaction(                            \
      HttpTransactionShell*, uint32_t classOfService) override;              \
  virtual nsresult CancelTransaction(HttpTransactionShell*, nsresult reason) \
      override;                                                              \
  virtual nsresult ReclaimConnection(HttpConnectionBase* conn) override;     \
  virtual nsresult ProcessPendingQ(nsHttpConnectionInfo*) override;          \
  virtual nsresult ProcessPendingQ() override;                               \
  virtual nsresult GetSocketThreadTarget(nsIEventTarget**) override;         \
  virtual nsresult SpeculativeConnect(                                       \
      nsHttpConnectionInfo*, nsIInterfaceRequestor*, uint32_t caps = 0,      \
      NullHttpTransaction* = nullptr) override;                              \
  virtual nsresult VerifyTraffic() override;                                 \
  virtual void BlacklistSpdy(const nsHttpConnectionInfo* ci) override;       \
  virtual nsresult ClearConnectionHistory() override;                        \
  virtual nsresult CompleteUpgrade(HttpTransactionShell* aTrans,             \
                                   nsIHttpUpgradeListener* aUpgradeListener) \
      override;                                                              \
  nsHttpConnectionMgr* AsHttpConnectionMgr() override;

}  // namespace net
}  // namespace mozilla

#endif  // HttpConnectionMgrShell_h__

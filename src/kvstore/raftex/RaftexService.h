/* Copyright (c) 2018 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License.
 */

#ifndef RAFTEX_RAFTEXSERVICE_H_
#define RAFTEX_RAFTEXSERVICE_H_

#include <folly/RWSpinLock.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>

#include "common/base/Base.h"
#include "interface/gen-cpp2/RaftexService.h"

namespace nebula {
namespace raftex {

class RaftPart;
class IOThreadPoolObserver;

/**
 * @brief Class to handle raft thrift server, also distribute request to RaftPart.
 * Only heartbeat is processed in io thread, other requests are processed in worker thread
 */
class RaftexService : public cpp2::RaftexServiceSvIf {
 public:
  /**
   * @brief Create a raft service
   *
   * @param pool IOThreadPool to use
   * @param workers Worker thread pool to use
   * @param port Listen port of thrift server
   * @return std::shared_ptr<RaftexService>
   */
  static std::shared_ptr<RaftexService> createService(
      std::shared_ptr<folly::IOThreadPoolExecutor> pool,
      std::shared_ptr<folly::Executor> workers,
      uint16_t port = 0);

  /**
   * @brief Destroy the Raftex Service
   */
  virtual ~RaftexService();

  /**
   * @brief Return the raft thrift server port
   */
  uint32_t getServerPort() const {
    return serverPort_;
  }

  /**
   * @brief Get the io thread pool
   *
   * @return std::shared_ptr<folly::IOThreadPoolExecutor>
   */
  std::shared_ptr<folly::IOThreadPoolExecutor> getIOThreadPool() const;

  /**
   * @brief Get the woker thread
   *
   * @return std::shared_ptr<folly::Executor>
   */
  std::shared_ptr<folly::Executor> getThreadManager();

  /**
   * @brief Start the raft thrift server
   *
   * @return Whether start succeed
   */
  bool start();

  /**
   * @brief Set the state to stopped
   */
  void stop();

  /**
   * @brief Wait until the thrift server has been stopped
   */
  void waitUntilStop();

  /**
   * @brief Handle leader election request in worker thread
   *
   * @param resp
   * @param req
   */
  void askForVote(cpp2::AskForVoteResponse& resp, const cpp2::AskForVoteRequest& req) override;

  /**
   * @brief Get the raft part state of given partition
   *
   * @param resp
   * @param req
   */
  void getState(cpp2::GetStateResponse& resp, const cpp2::GetStateRequest& req) override;

  /**
   * @brief Handle append log request in worker thread
   *
   * @param resp
   * @param req
   */
  void appendLog(cpp2::AppendLogResponse& resp, const cpp2::AppendLogRequest& req) override;

  /**
   * @brief Handle send snapshot reqtuest in worker thread
   *
   * @param resp
   * @param req
   */
  void sendSnapshot(cpp2::SendSnapshotResponse& resp,
                    const cpp2::SendSnapshotRequest& req) override;

  /**
   * @brief Handle heartbeat request in io thread
   *
   * @param callback Thrift callback
   * @param req
   */
  void async_eb_heartbeat(
      std::unique_ptr<apache::thrift::HandlerCallback<cpp2::HeartbeatResponse>> callback,
      const cpp2::HeartbeatRequest& req) override;

  /**
   * @brief Register the RaftPart to the service
   */
  void addPartition(std::shared_ptr<RaftPart> part);

  /**
   * @brief Unregister the RaftPart to the service
   */
  void removePartition(std::shared_ptr<RaftPart> part);

  /**
   * @brief Find the RaftPart by spaceId and partId
   *
   * @param spaceId
   * @param partId
   * @return std::shared_ptr<RaftPart>
   */
  std::shared_ptr<RaftPart> findPart(GraphSpaceID spaceId, PartitionID partId);

 private:
  /**
   * @brief Start the thrift server
   *
   * @param pool IO thread pool
   * @param workers Worker thread pool
   * @param port Thrift port to listener
   */
  void initThriftServer(std::shared_ptr<folly::IOThreadPoolExecutor> pool,
                        std::shared_ptr<folly::Executor> workers,
                        uint16_t port = 0);

  /**
   * @brief Prepare the setup of thrift server
   *
   * @return Return whether succeed
   */
  bool setup();
  void serve();

  /**
   * @brief Wait until the service is ready to serve
   */
  void waitUntilReady();

  RaftexService() = default;

 private:
  std::unique_ptr<apache::thrift::ThriftServer> server_;
  std::unique_ptr<std::thread> serverThread_;
  uint32_t serverPort_;

  enum RaftServiceStatus { STATUS_NOT_RUNNING = 0, STATUS_SETUP_FAILED = 1, STATUS_RUNNING = 2 };
  std::atomic_int status_{STATUS_NOT_RUNNING};

  folly::RWSpinLock partsLock_;
  std::unordered_map<std::pair<GraphSpaceID, PartitionID>, std::shared_ptr<RaftPart>> parts_;
};

}  // namespace raftex
}  // namespace nebula

#endif  // RAFTEX_RAFTEXSERVICE_H_

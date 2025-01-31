/* Copyright (c) 2021 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License.
 */

#include "graph/executor/admin/DropHostsExecutor.h"

#include "graph/planner/plan/Admin.h"

namespace nebula {
namespace graph {

folly::Future<Status> DropHostsExecutor::execute() {
  auto *dhNode = asNode<DropHosts>(node());
  return qctx()
      ->getMetaClient()
      ->dropHosts(dhNode->getHosts())
      .via(runner())
      .thenValue([this](StatusOr<bool> resp) {
        SCOPED_TIMER(&execTime_);
        NG_RETURN_IF_ERROR(resp);
        if (!resp.value()) {
          return Status::Error("Drop Hosts failed!");
        }
        return Status::OK();
      });
}

}  // namespace graph
}  // namespace nebula

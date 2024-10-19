/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef BF_P4C_MAU_DUMP_JSON_GRAPH_H_
#define BF_P4C_MAU_DUMP_JSON_GRAPH_H_

#include "bf-p4c/logging/pass_manager.h"
#include "bf-p4c/mau/table_dependency_graph.h"

using namespace P4;

class DumpJsonGraph : public PassManager {
    FlowGraph fg;
    DependencyGraph &dg;
    Util::JsonObject *dgJson;
    cstring passContext;
    bool placed;

    void end_apply(const IR::Node *root) override;

 public:
    DumpJsonGraph(DependencyGraph &dg, Util::JsonObject *dgJson, cstring passContext, bool placed);
};

#endif /* BF_P4C_MAU_DUMP_JSON_GRAPH_H_ */

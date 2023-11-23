#include "backends/p4tools/common/lib/trace_event.h"

#include <ostream>

#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "ir/visitor.h"

namespace P4Tools {

std::ostream &operator<<(std::ostream &os, const TraceEvent &event) {
    event.print(os);
    return os;
}

TraceEvent::TraceEvent() = default;

const TraceEvent *TraceEvent::subst(const SymbolicEnv & /*env*/) const { return this; }

const TraceEvent *TraceEvent::apply(Transform & /*visitor*/) const { return this; }

const TraceEvent *TraceEvent::evaluate(const Model & /*model*/, bool /*doComplete*/) const {
    return this;
}

}  // namespace P4Tools

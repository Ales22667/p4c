#ifndef BACKENDS_P4TOOLS_COMMON_LIB_MODEL_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_MODEL_H_

#include <map>
#include <utility>

#include "absl/container/btree_map.h"
#include "ir/ir.h"
#include "ir/solver.h"
#include "ir/visitor.h"

namespace P4::P4Tools {

/// Symbolic maps map a state variable to a IR::Expression.
using SymbolicMapType = absl::btree_map<IR::StateVariable, const IR::Expression *>;

/// Represents a solution found by the solver. A model is a concretized form of a symbolic
/// environment. All the expressions in a Model must be of type IR::Literal.
class Model {
 private:
    /// The mapping of symbolic variables to values. Usually generated by an SMT solver.
    SymbolicMapping symbolicMap;

    // A visitor that iterates over an input expression and visits all the variables in the input
    // expression. These variables correspond to variables that were used for constraints in model
    // generated by the SMT solver. A variable needs to be
    /// completed if it is not present in the model computed by the solver that produced the model.
    /// This typically happens when a variable is not needed to solve a set of constraints.
    class SubstVisitor : public Transform {
        const Model &self;

        /// If doComplete is true, the visitor will not throw an error if a variable does not have a
        /// substitution and instead assign a random or zero value (depending on whether there is a
        /// seed) to the variable..
        bool doComplete;

     public:
        const IR::Literal *preorder(IR::StateVariable *var) override;
        const IR::Literal *preorder(IR::SymbolicVariable *var) override;
        const IR::Literal *preorder(IR::TaintExpression *var) override;

        explicit SubstVisitor(const Model &model, bool doComplete);
    };

 public:
    // Maps an expression to its value in the model.
    using ExpressionMap = std::map<const IR::Expression *, const IR::Literal *>;

    /// A model is initialized with a symbolic map. Usually, these are derived from the solver.
    explicit Model(SymbolicMapping symbolicMap) : symbolicMap(std::move(symbolicMap)) {}

    Model(const Model &) = default;
    Model(Model &&) = default;
    Model &operator=(const Model &) = default;
    Model &operator=(Model &&) = default;
    ~Model() = default;

    /// Evaluates a P4 expression in the context of this model.
    ///
    /// A BUG occurs if the given expression refers to a variable that is not bound by this model.
    /// If the input list @param resolvedExpressions is not null, we also collect the resolved value
    /// of this expression.
    const IR::Literal *evaluate(const IR::Expression *expr, bool doComplete,
                                ExpressionMap *resolvedExpressions = nullptr) const;

    // Evaluates a P4 StructExpression in the context of this model. Recursively calls into
    // @evaluate and substitutes all members of this list with a Value type.
    const IR::StructExpression *evaluateStructExpr(
        const IR::StructExpression *structExpr, bool doComplete,
        ExpressionMap *resolvedExpressions = nullptr) const;

    // Evaluates a P4 BaseListExpression in the context of this model. Recursively calls into
    // @evaluate and substitutes all members of this list with a Value type.
    const IR::BaseListExpression *evaluateListExpr(
        const IR::BaseListExpression *listExpr, bool doComplete,
        ExpressionMap *resolvedExpressions = nullptr) const;

    /// Tries to retrieve @param var from the model.
    /// If @param checked is true, this function throws a BUG if the variable can not be found.
    /// Otherwise, it returns a nullptr.
    [[nodiscard]] const IR::Expression *get(const IR::SymbolicVariable *var, bool checked) const;

    /// Set (and possibly override) a symbolic variable in the model.
    /// This is for example useful for payload calculation or concolic execution after we have
    /// reached a leaf state.
    void set(const IR::SymbolicVariable *var, const IR::Expression *val);

    /// Merge a sumbolic map into this model, do NOT override existing values.
    void mergeMap(const SymbolicMapping &sourceMap);

    /// @returns the symbolic map contained within this model.
    [[nodiscard]] const SymbolicMapping &getSymbolicMap() const;
};

}  // namespace P4::P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_MODEL_H_ */

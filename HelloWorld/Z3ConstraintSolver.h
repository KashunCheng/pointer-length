//
// Created by kashun on 5/8/25.
//

#ifndef Z3CONSTRAINTSOLVER_H
#define Z3CONSTRAINTSOLVER_H
#include "z3++.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/FormatVariadic.h"
#include <llvm/IR/Constants.h>
#include <string>

using namespace z3;
using llvm::DenseMap;
using llvm::Value;
using std::string;

class Z3ConstraintSolver {

public:
  context context;

private:
  func_decl_vector ptr_proj;
  sort ptr_sort;
  DenseMap<const Value *, string> expr_names;
  DenseMap<const Value *, expr> ptr_to_expr;
  DenseMap<const Value *, expr> integer_to_expr;

  [[nodiscard]] const char *value_name(const Value *value) {
    assert(value != nullptr);
    if (expr_names.contains(value)) {
      return expr_names[value].c_str();
    }
    auto [fst, _] = expr_names.insert(
        std::make_pair(value, llvm::formatv("{0:x}", value).str()));
    return fst->second.c_str();
  }

public:
  solver solver;

  explicit Z3ConstraintSolver();
  expr int_const(const Value *value) {
    assert(value != nullptr);
    if (const auto CI = dyn_cast<llvm::ConstantInt>(value)) {
      // assert(CI->getBitWidth() <= 32);
      return context.int_val(CI->getZExtValue());
    }
    const auto it = integer_to_expr.find(value);
    if (it == integer_to_expr.end()) {
      auto expr = context.int_const(value_name(value));
      integer_to_expr.insert(std::make_pair(value, expr));
      return expr;
    }
    return it->second;
  }

  expr int_const(const uint64_t value) {
    return context.int_val(value);
  }

  // [[nodiscard]] expr int_val(const int value) { return context.int_val(value); }

  expr ptr_const(const Value *value) {
    const auto it = ptr_to_expr.find(value);
    if (it == ptr_to_expr.end()) {
      auto expr = context.constant(value_name(value), ptr_sort);
      ptr_to_expr.insert(std::make_pair(value, expr));
      return expr;
    }
    return it->second;
  }

  bool contains_int_const(const Value *value) const {
    return ptr_to_expr.contains(value);
  }

  bool contains_ptr_const(const Value *value) const {
    return ptr_to_expr.contains(value);
  }

  [[nodiscard]] expr addr(const expr &ptr) const { return ptr_proj[0](ptr); }
  [[nodiscard]] expr length(const expr &ptr) const { return ptr_proj[1](ptr); }
  [[nodiscard]] expr capacity(const expr &ptr) const {
    return ptr_proj[2](ptr);
  }
};

#endif // Z3CONSTRAINTSOLVER_H

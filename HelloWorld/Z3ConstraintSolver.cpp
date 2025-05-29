//
// Created by kashun on 5/8/25.
//

#include "Z3ConstraintSolver.h"

Z3ConstraintSolver::Z3ConstraintSolver()
    : ptr_proj(context), ptr_sort(context), solver(context) {
  const char *ptr_sort_names[3] = {"ptr", "cap", "len"};
  const sort sorts[3] = {context.int_sort(), context.int_sort(),
                         context.int_sort()};
  const auto make_ptr =
      context.tuple_sort("Array", 3, ptr_sort_names, sorts, ptr_proj);
  ptr_sort = make_ptr.range();
}

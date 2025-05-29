//
// Created by kashun on 5/1/25.
//

#ifndef POINTERDEFRESOLUTION_H
#define POINTERDEFRESOLUTION_H
#include "ReversedCallGraph.h"
#include "Z3ConstraintSolver.h"
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Instructions.h>
#include <z3++.h>
#include <cpptrace/cpptrace.hpp>


using std::variant;

using llvm::AllocaInst;
using llvm::CallInst;
using llvm::DenseSet;
using llvm::Function;
using llvm::SmallVector;
using z3::context;

struct ParamsInfo {
  SmallVector<unsigned> PointerIndices;
  SmallVector<unsigned> IntegerIndices;
};

struct SolverResult {
  bool IsLength = false;
  bool IsCapacity = false;
  bool LeqLength = false;
  bool LeqCapacity = false;
};

// struct MallocDescriptor {
//   CallInst const *MallocCall;
// };
//
// using PointerSource = variant<const AllocaInst *, MallocDescriptor>;
using PointerSink = const Function *;
// using GraphVertex = struct {
//   const Function *F;
//   SmallVector<PointerSource> Sources;
// };
// using GraphEdge = SmallVector<const CallBase *>;
// using Graph = adjacency_list<vecS, vecS, directedS, GraphVertex, GraphEdge>;
// using Vertex = boost::graph_traits<Graph>::vertex_descriptor;

class PointerDefResolution {
  // Vertex Sink;
  // Graph G;
  // DenseMap<const Function *, Vertex> FunctionToVertex;
  // DenseSet<const void *> Visited;
  //
  // Vertex InsertVertex(GraphVertex &&v) {
  //   auto F = v.F;
  //   auto vertex = add_vertex(std::forward<GraphVertex>(v), G);
  //   FunctionToVertex[F] = vertex;
  //   return vertex;
  // }

  void PointerPropertyCheck(Z3ConstraintSolver &CS,
                            const ReversedCallGraph &RCG,
                            const CallGraphNode *Caller, const Function *Callee,
                            unsigned CallArgsPtrIdx,
                            unsigned CallArgsIntegerIdx, int calls);

public:
  explicit PointerDefResolution(const CallGraph &CG,
                                const ReversedCallGraph &RCG, PointerSink PS,
                                ParamsInfo PI);
};

[[noreturn]] inline void todo() {
  cpptrace::generate_trace().print();
  throw std::logic_error("Not yet implemented");
}

#endif // POINTERDEFRESOLUTION_H

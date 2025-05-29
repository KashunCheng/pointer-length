//
// Created by kashun on 5/1/25.
//

#include "ReversedCallGraph.h"
ReversedCallGraph::ReversedCallGraph(const CallGraph &CG) {
  for (auto &KV : CG) {
    const auto Caller = KV.second.get();
    assert(Caller != nullptr);
    if (!NodeToVertex.contains(Caller)) {
      NodeToVertex[Caller] = add_vertex(Caller, G);
      VertexToNode[NodeToVertex[Caller]] = Caller;
    }
    for (auto &CallRecord : *Caller) {
      const auto Callee = CallRecord.second;
      assert(Callee != nullptr);
      if (!NodeToVertex.contains(Callee)) {
        NodeToVertex[Callee] = add_vertex(Callee, G);
        VertexToNode[NodeToVertex[Callee]] = Callee;
      }
      add_edge(NodeToVertex[Callee], NodeToVertex[Caller], G);
#ifndef NDEBUG
      llvm::errs() << "[ReversedCallGraph] Adding an edge from ";
      if (Callee->getFunction()) {
        llvm::errs() << Callee->getFunction()->getName();
      } else {
        llvm::errs() << "null";
      }
      llvm::errs() << " to ";
      if (Caller->getFunction()) {
        llvm::errs() << Caller->getFunction()->getName();
      } else {
        llvm::errs() << "null";
      }
      llvm::errs() << "\n";
#endif
    }
  }
}
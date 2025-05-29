//
// Created by kashun on 5/1/25.
//

#ifndef REVERSEDCALLGRAPH_H
#define REVERSEDCALLGRAPH_H
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <llvm/Analysis/CallGraph.h>

using llvm::CallBase;
using llvm::CallGraph;
using llvm::CallGraphNode;
using llvm::DenseMap;

using boost::add_edge;
using boost::add_vertex;
using boost::adjacency_list;
using boost::adjacent_vertices;
using boost::directedS;
using boost::vecS;

class ReversedCallGraph {
  using GraphVertex = const CallGraphNode *;
  using GraphEdge = const CallBase *;
  using Graph = adjacency_list<vecS, vecS, directedS, GraphVertex, GraphEdge>;
  using Vertex = boost::graph_traits<Graph>::vertex_descriptor;

  Graph G;
  DenseMap<const CallGraphNode *, Vertex> NodeToVertex;
  DenseMap<Vertex, const CallGraphNode *> VertexToNode;

public:
  explicit ReversedCallGraph(const CallGraph &CG);

  auto iterator(const CallGraphNode *node) const {
#ifndef NDEBUG
    llvm::errs() << "[ReversedCallGraph] Iterating over the node ";
    if (node->getFunction()) {
      llvm::errs() << node->getFunction()->getName();
    } else {
      llvm::errs() << "null";
    }
    llvm::errs() << "\n";
    const auto [Neighbour, NeighbourEnd] =
        adjacent_vertices(NodeToVertex.at(node), G);
    for (auto I = Neighbour; I != NeighbourEnd; ++I) {
      llvm::errs() << "[ReversedCallGraph] Found a neighbour ";
      if (VertexToNode.at(*I)->getFunction()) {
        llvm::errs() << VertexToNode.at(*I)->getFunction()->getName();
      } else {
        llvm::errs() << "null";
      }
      llvm::errs() << "\n";
    }
    llvm::errs() << "[ReversedCallGraph] Finished iterating over the node\n";
#endif
    return adjacent_vertices(NodeToVertex.at(node), G);
  }

  const CallGraphNode *operator[](const Vertex &v) const {
    return VertexToNode.at(v);
  }
};

#endif // REVERSEDCALLGRAPH_H

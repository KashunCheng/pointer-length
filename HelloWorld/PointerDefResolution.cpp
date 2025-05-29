//
// Created by kashun on 5/1/25.
//

#include "PointerDefResolution.h"
#include <numeric>

using llvm::BinaryOperator;
using llvm::dyn_cast;
using llvm::GetElementPtrInst;
using llvm::Value;

using z3::expr;

constexpr int max_depth = 1;

void TaintReturnValueAnalysis(const Function *F) { assert(F != nullptr); }

SolverResult AnalyzeCB(Z3ConstraintSolver &Ctx, const CallBase *CB,
                       unsigned PtrIndex, unsigned IntegerIndex) {
  assert(CB != nullptr);
#ifndef NDEBUG
  llvm::errs() << "[PointerDefResolution] AnalyzeCB: " << CB->getName() << "\n";
  llvm::errs() << "[PointerDefResolution] AnalyzeCB: ";
  CB->dump();
#endif
  solver &Solver = Ctx.solver;
  const Value *InterestingPtr = CB->getOperand(PtrIndex);
  const Value *InterestingInteger = CB->getOperand(IntegerIndex);
  const Value *Ptr = InterestingPtr;
  const Value *Integer = InterestingInteger;
  DenseMap<const Value *, expr> PtrToExpr;
  DenseMap<const Value *, expr> IntegerToExpr;

  PtrToExpr.insert(std::make_pair(Ptr, Ctx.ptr_const(Ptr)));
  IntegerToExpr.insert(std::make_pair(Integer, Ctx.int_const(Integer)));
  auto &CurrentPtrExpr = PtrToExpr.find(Ptr)->getSecond();
  auto &CurrentIntegerExpr = IntegerToExpr.find(Integer)->getSecond();
  for (auto I = CB->getPrevNonDebugInstruction(); I != nullptr;
       I = I->getPrevNonDebugInstruction()) {
    if (auto *GEP = dyn_cast<GetElementPtrInst>(I)) {
      if (GEP != Ptr) {
        continue;
      }
      if (!GEP->isInBounds()) {
        todo();
      }
      if (GEP->indices().end() - GEP->indices().begin() != 2) {
        todo();
      }
      if (GEP->getNumIndices() == 0) {
        todo();
      }
      const auto GEPOffset = (GEP->indices().end() - 1)->get();
      if (!IntegerToExpr.contains(GEPOffset)) {
        IntegerToExpr.insert(
            std::make_pair(GEPOffset, Ctx.int_const(GEPOffset)));
      }
      const auto &GEPOffsetExpr = IntegerToExpr.find(GEPOffset)->getSecond();

      const auto GEPFromPtr = GEP->getPointerOperand();
      if (!PtrToExpr.contains(GEPFromPtr)) {
        PtrToExpr.insert(std::make_pair(GEPFromPtr, Ctx.ptr_const(GEPFromPtr)));
      }
      const auto &GEPFromPtrExpr = PtrToExpr.find(GEPFromPtr)->getSecond();
      Solver.add(Ctx.addr(CurrentPtrExpr) ==
                 Ctx.addr(GEPFromPtrExpr) + GEPOffsetExpr);
      Solver.add(Ctx.length(CurrentPtrExpr) ==
                 Ctx.length(GEPFromPtrExpr) - GEPOffsetExpr);
      Solver.add(Ctx.capacity(CurrentPtrExpr) ==
                 Ctx.capacity(GEPFromPtrExpr) - GEPOffsetExpr);
      Ptr = GEPFromPtr;
      CurrentPtrExpr = GEPFromPtrExpr;
    } else if (auto *AI = dyn_cast<AllocaInst>(I)) {
      if (AI != Ptr) {
        continue;
      }
      const auto AllocaType = AI->getAllocatedType();
      if (const auto AT = dyn_cast<llvm::ArrayType>(AllocaType)) {
        if (AT->getArrayElementType()->isArrayTy()) {
          todo();
        }
        Solver.add(Ctx.capacity(CurrentPtrExpr) ==
                   Ctx.int_const(AT->getArrayNumElements()));
      } else {
        todo();
      }
    } else if (auto *BO = dyn_cast<BinaryOperator>(I)) {
      if (BO != Ptr && BO != Integer) {
        continue;
      }
      if (BO == Ptr) {
        todo();
        continue;
      }
      switch (BO->getOpcode()) {
      case BinaryOperator::Add:

      default:
        llvm::errs() << "Unhandled BinaryOperator " << BO->getOpcodeName() << "\n";
        todo();
      }
    } else {
      if (I == Integer || I == Ptr) {
        todo();
      }
    }
  }

  SolverResult result;

  Solver.push();
  Solver.add(Ctx.capacity(PtrToExpr.find(InterestingPtr)->second) !=
             IntegerToExpr.find(InterestingInteger)->second);
  result.IsCapacity = Solver.check() == unsat;
  Solver.pop();

  Solver.push();
  Solver.add(Ctx.capacity(PtrToExpr.find(InterestingPtr)->second) <
             IntegerToExpr.find(InterestingInteger)->second);
  result.LeqCapacity = Solver.check() == unsat;
  Solver.pop();

  Solver.push();
  Solver.add(Ctx.length(PtrToExpr.find(InterestingPtr)->second) !=
             IntegerToExpr.find(InterestingInteger)->second);
  result.IsLength = Solver.check() == unsat;
  Solver.pop();

  Solver.push();
  Solver.add(Ctx.length(PtrToExpr.find(InterestingPtr)->second) <
             IntegerToExpr.find(InterestingInteger)->second);
  result.LeqLength = Solver.check() == unsat;
  Solver.pop();

  llvm::outs() << "IsCapacity: " << result.IsCapacity << '\n';
  llvm::outs() << "LeqCapacity: " << result.LeqCapacity << '\n';
  llvm::outs() << "IsLength: " << result.IsLength << '\n';
  llvm::outs() << "LeqLength: " << result.LeqLength << '\n';
  return result;
}

void PointerDefResolution::PointerPropertyCheck(
    Z3ConstraintSolver &CS, const ReversedCallGraph &RCG,
    const CallGraphNode *Caller, const Function *Callee,
    const unsigned CallArgsPtrIdx, const unsigned CallArgsIntegerIdx,
    const int calls) {
  if (calls < 0) {
    return;
  }
  assert(Caller != nullptr);
  assert(Caller->getFunction() != nullptr);
  assert(Callee != nullptr);
  assert(Callee->getArg(CallArgsPtrIdx)->getType()->isPointerTy());
  // if (Visited.contains(Caller)) {
  //   // Add the edge to the graph
  //   return;
  // }
#ifndef NDEBUG
  llvm::errs() << "[PointerDefResolution] InitializePointerGraph: "
               << Caller->getFunction()->getName() << " calls "
               << Callee->getName() << "\n";
#endif
  // Visited.insert(Caller);
  const auto CallerSelfContained = std::reduce(
      Caller->begin(), Caller->end(), true,
      [Callee, CallArgsPtrIdx, CallArgsIntegerIdx,
       &CS](bool acc, const CallGraphNode::CallRecord &arg) {
        auto &[Tracking, OtherCallee] = arg;
        if (!Tracking) {
          return acc;
        }
        if (OtherCallee->getFunction() == nullptr) {
          return acc;
        }
        if (OtherCallee->getFunction() != Callee) {
          return acc;
        }
        if (const auto CB = dyn_cast<CallBase>(*Tracking)) {
          const auto [IsLength, IsCapacity, LeqLength, LeqCapacity] =
              AnalyzeCB(CS, CB, CallArgsPtrIdx, CallArgsIntegerIdx);
          return IsCapacity || LeqCapacity || IsLength || LeqLength;
        }
        return acc;
      });
#ifndef NDEBUG
  llvm::errs() << "[PointerDefResolution] InitializePointerGraph: "
               << Caller->getFunction()->getName()
               << " is self-contained: " << CallerSelfContained << "\n";
#endif
  // if (CallerSelfContained) {
  //   return;
  // }
  const auto [CallerOfCallerBegin, CallerOfCallerEnd] = RCG.iterator(Caller);
  for (auto I = CallerOfCallerBegin; I != CallerOfCallerEnd; ++I) {
    const auto NextCallee = Caller->getFunction();
    const auto NextCaller = RCG[*I];
    if (NextCaller->getFunction() == nullptr) {
      continue;
    }
    PointerPropertyCheck(CS, RCG, NextCaller, NextCallee, CallArgsPtrIdx,
                         CallArgsIntegerIdx, calls - 1);
  }
}

PointerDefResolution::PointerDefResolution(const CallGraph &CG,
                                           const ReversedCallGraph &RCG,
                                           const PointerSink PS,
                                           const ParamsInfo PI) {
  assert(PS != nullptr);
  // Sink = InsertVertex({PS, {}});
  const auto SinkNode = CG[PS];
  const auto [CallerBegin, CallerEnd] = RCG.iterator(SinkNode);
  for (auto I = CallerBegin; I != CallerEnd; ++I) {
    const auto Caller = RCG[*I];
    assert(Caller != nullptr);
    if (Caller->getFunction() == nullptr) {
      continue;
    }
    for (auto depth = 0; depth < max_depth; depth++) {
      for (auto ptr : PI.PointerIndices) {
        for (auto i : PI.IntegerIndices) {
          Z3ConstraintSolver cs;
          PointerPropertyCheck(cs, RCG, Caller, PS, ptr, i, depth);
        }
      }
    }
  }
}

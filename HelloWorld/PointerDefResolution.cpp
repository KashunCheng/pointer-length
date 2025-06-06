//
// Created by kashun on 5/1/25.
//

#include "PointerDefResolution.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include <numeric>

using llvm::BinaryOperator;
using llvm::dyn_cast;
using llvm::GetElementPtrInst;
using llvm::Value;

using z3::expr;

constexpr int max_depth = 1;

void TaintReturnValueAnalysis(const Function *F) { assert(F != nullptr); }

static bool IsValueConstantInt(const Value *V) {
  return llvm::isa<llvm::ConstantInt>(V);
}

bool Z3AccumulateConstantOffset(Z3ConstraintSolver &Ctx, llvm::Type *SourceType,
                                llvm::ArrayRef<const Value *> Index,
                                const llvm::DataLayout &DL, expr &Offset) {
  Offset = Ctx.int_const(uint64_t(0));

  // Fast path for canonical getelementptr i8 form.
  if (SourceType->isIntegerTy(8) && !Index.empty()) {
    auto *CI = dyn_cast<llvm::ConstantInt>(Index.front());
    if (CI && CI->getType()->isIntegerTy()) {
      Offset = Offset + Ctx.int_const(CI->getValue().getZExtValue());
      return true;
    }
    todo();
    return false;
  }

  auto AccumulateOffset = [&](uint64_t Index, uint64_t Size) -> bool {
    Offset = Offset + Ctx.int_const(Index * Size);
    return true;
  };

  auto begin = llvm::generic_gep_type_iterator<decltype(Index.begin())>::begin(
      SourceType, Index.begin());
  auto end =
      llvm::generic_gep_type_iterator<decltype(Index.end())>::end(Index.end());
  for (auto GTI = begin, GTE = end; GTI != GTE; ++GTI) {
    // Scalable vectors are multiplied by a runtime constant.
    bool ScalableType = GTI.getIndexedType()->isScalableTy();

    Value *V = GTI.getOperand();
    llvm::StructType *STy = GTI.getStructTypeOrNull();
    // Handle ConstantInt if possible.
    auto *ConstOffset = dyn_cast<llvm::ConstantInt>(V);
    if (ConstOffset && ConstOffset->getType()->isIntegerTy()) {
      if (ConstOffset->isZero())
        continue;
      // if the type is scalable and the constant is not zero (vscale * n * 0 =
      // 0) bailout.
      if (ScalableType) {
        todo();
      }
      // Handle a struct index, which adds its field offset to the pointer.
      if (STy) {
        unsigned ElementIdx = ConstOffset->getZExtValue();
        const llvm::StructLayout *SL = DL.getStructLayout(STy);
        // Element offset is in bytes.
        if (SL->getElementOffset(ElementIdx).isScalable()) {
          todo();
        }
        if (!AccumulateOffset(SL->getElementOffset(ElementIdx).getFixedValue(),
                              1))
          return false;
        continue;
      }
      if (!AccumulateOffset(ConstOffset->getValue().getZExtValue(),
                            GTI.getSequentialElementStride(DL)))
        return false;
      continue;
    } else {
      todo();
    }
  }
  return true;
}

struct DominanceInfo {
  SmallVector<expr> taken_exprs;
  SmallVector<expr> not_taken_exprs;
};

expr join_all(const SmallVector<expr> &exprs, Z3ConstraintSolver &Ctx) {
  return std::reduce(exprs.begin(), exprs.end(), Ctx.bool_const(true),
                     [](const expr &a, const expr &b) { return a && b; });
}

std::pair<SmallVector<expr>, const llvm::PHINode *>
AnalyzeBB(Z3ConstraintSolver &Ctx, const llvm::Instruction *I,
          DenseSet<const Value *> &InterestingPtrs,
          DenseSet<const Value *> &InterestingIntegers,
          std::optional<DominanceInfo> dominance_info) {
  DenseSet<const Value *> InterestingBooleans;
  SmallVector<expr> Results;
  assert(I != nullptr);
  for (; I != nullptr; I = I->getPrevNonDebugInstruction()) {
    I->dump();
    if (auto *GEP = dyn_cast<GetElementPtrInst>(I)) {
      if (!InterestingPtrs.contains(GEP)) {
        continue;
      }
      if (GEP->getNumIndices() == 0) {
        todo();
      }
      const auto CurrentPtrExpr = Ctx.ptr_const(GEP);
      expr GEPOffsetExpr(Ctx.context);
      {
        SmallVector<const Value *> Indices(
            llvm::drop_begin(GEP->operand_values()));
        Z3AccumulateConstantOffset(Ctx, GEP->getSourceElementType(), Indices,
                                   GEP->getDataLayout(), GEPOffsetExpr);
      }

      const auto GEPFromPtr = GEP->getPointerOperand();
      const auto &GEPFromPtrExpr = Ctx.ptr_const(GEPFromPtr);
      Results.push_back(Ctx.addr(CurrentPtrExpr) ==
                        Ctx.addr(GEPFromPtrExpr) + GEPOffsetExpr);
      Results.push_back(Ctx.length(CurrentPtrExpr) ==
                        Ctx.length(GEPFromPtrExpr) - GEPOffsetExpr);
      Results.push_back(Ctx.capacity(CurrentPtrExpr) ==
                        Ctx.capacity(GEPFromPtrExpr) - GEPOffsetExpr);
      InterestingPtrs.insert(GEPFromPtr);
      InterestingPtrs.erase(GEP);
    } else if (auto *AI = dyn_cast<AllocaInst>(I)) {
      if (!InterestingPtrs.contains(AI)) {
        continue;
      }
      const auto CurrentPtrExpr = Ctx.ptr_const(AI);
      const auto AllocaType = AI->getAllocatedType();
      if (const auto AT = dyn_cast<llvm::ArrayType>(AllocaType)) {
        if (AT->getArrayElementType()->isArrayTy()) {
          todo();
        }
        Results.push_back(Ctx.capacity(CurrentPtrExpr) ==
                          Ctx.int_const(AT->getArrayNumElements()));
      } else {
        todo();
      }
    } else if (auto *BO = dyn_cast<BinaryOperator>(I)) {
      if (!InterestingIntegers.contains(BO)) {
        continue;
      }
      switch (BO->getOpcode()) {
      case BinaryOperator::Add:
      case BinaryOperator::Sub: {
        const auto *left = BO->getOperand(0);
        const auto leftExpr = Ctx.int_const(left);
        const auto *right = BO->getOperand(1);
        const auto rightExpr = Ctx.int_const(right);
        const auto &BOExpr = Ctx.int_const(BO);
        switch (BO->getOpcode()) {
        case BinaryOperator::Add: {
          Results.push_back(BOExpr == leftExpr + rightExpr);
          break;
        }
        case BinaryOperator::Sub: {
          Results.push_back(BOExpr == leftExpr - rightExpr);
          break;
        }
        default:
          todo();
        }
        if (!IsValueConstantInt(left)) {
          InterestingIntegers.insert(left);
        }
        if (!IsValueConstantInt(right)) {
          InterestingIntegers.insert(right);
        }
        InterestingIntegers.erase(BO);
        break;
      }

      default:
        llvm::errs() << "Unhandled BinaryOperator " << BO->getOpcodeName()
                     << "\n";
        todo();
      }
    } else if (auto *TI = dyn_cast<llvm::TruncInst>(I)) {
      if (!InterestingIntegers.contains(TI)) {
        continue;
      }
      const auto *old = TI->getOperand(0);
      const auto oldExpr = Ctx.int_const(old);
      if (!IsValueConstantInt(old)) {
        InterestingIntegers.insert(old);
      }
      Results.push_back(oldExpr == Ctx.int_const(TI));
      InterestingIntegers.erase(TI);
    } else if (auto *PTII = dyn_cast<llvm::PtrToIntInst>(I)) {
      if (!InterestingIntegers.contains(PTII)) {
        continue;
      }
      const auto *old = PTII->getOperand(0);
      const auto oldExpr = Ctx.ptr_const(old);
      Results.push_back(Ctx.addr(oldExpr) == Ctx.int_const(PTII));
      InterestingIntegers.erase(PTII);
      InterestingPtrs.insert(old);
    } else if (auto *CI = dyn_cast<llvm::CallInst>(I)) {
      const auto *Fn = CI->getCalledFunction();
      if (Fn == nullptr) {
        // indirect call
        todo();
      }
      if (Fn->getName() == "strlen") {
        if (!InterestingIntegers.contains(CI)) {
          continue;
        }
        const auto *old = CI->getOperand(0);
        const auto oldExpr = Ctx.ptr_const(old);
        Results.push_back(Ctx.length(oldExpr) == Ctx.int_const(CI));
        InterestingIntegers.erase(CI);
        InterestingPtrs.insert(old);
        continue;
      } else if (Fn->getName() == "strchr") {
        if (!InterestingPtrs.contains(CI)) {
          continue;
        }
        const auto *old = CI->getOperand(0);
        const auto oldExpr = Ctx.ptr_const(old);
        const auto outExpr = Ctx.ptr_const(CI);
        const auto delta = Ctx.free_int_const();
        Results.push_back((delta >= 0 && delta < Ctx.length(oldExpr) &&
                          Ctx.addr(outExpr) == Ctx.addr(oldExpr) + delta &&
                          Ctx.length(outExpr) == Ctx.length(oldExpr) - delta &&
                          Ctx.capacity(outExpr) == Ctx.capacity(oldExpr) - delta)
                          || Ctx.addr(outExpr) == Ctx.int_const(uint64_t(0)) &&
                          Ctx.capacity(outExpr) == Ctx.int_const(uint64_t(0)));
        InterestingPtrs.erase(CI);
        InterestingPtrs.insert(old);
        continue;
      }
      if (InterestingIntegers.contains(I) || InterestingPtrs.contains(I)) {
        I->dump();
        todo();
      }
    } else if (auto *PHI = dyn_cast<llvm::PHINode>(I)) {
      return std::make_pair(Results, PHI);
    } else if (auto *BI = dyn_cast<llvm::BranchInst>(I)) {
      if (dominance_info == std::nullopt) {
        continue;
      }
      const auto taken_expr = join_all(dominance_info->taken_exprs, Ctx);
      const auto not_taken_expr =
          join_all(dominance_info->not_taken_exprs, Ctx);
      const auto *cond = BI->getOperand(0);
      const auto condExpr = Ctx.bool_const(cond);
      Results.push_back(implies(condExpr, taken_expr));
      Results.push_back(implies(!condExpr, Ctx.bool_const(true)));
      InterestingBooleans.insert(cond);
    } else if (auto *ICmpI = dyn_cast<llvm::ICmpInst>(I)) {
      if (!InterestingBooleans.contains(ICmpI)) {
        continue;
      }
      const bool isPtrCmp = ICmpI->getOperand(0)->getType()->isPointerTy();
      const auto *left = ICmpI->getOperand(0);
      const auto leftExpr =
          isPtrCmp ? Ctx.ptr_const(left) : Ctx.int_const(left);
      const auto *right = ICmpI->getOperand(1);
      const auto rightExpr =
          isPtrCmp ? Ctx.ptr_const(right) : Ctx.int_const(right);
      const auto &ICmpIExpr = Ctx.bool_const(ICmpI);
      if (!IsValueConstantInt(left)) {
        if (isPtrCmp) {
          InterestingPtrs.insert(left);
        } else {
          InterestingIntegers.insert(left);
        }
      }
      if (!IsValueConstantInt(right)) {
        if (isPtrCmp) {
          InterestingPtrs.insert(right);
        } else {
          InterestingIntegers.insert(right);
        }
      }
      Results.push_back(ICmpIExpr ==
                        (isPtrCmp ? (Ctx.addr(leftExpr) == Ctx.addr(rightExpr))
                                  : (leftExpr == rightExpr)));
      InterestingBooleans.erase(ICmpI);
    } else {
      if (InterestingIntegers.contains(I) || InterestingPtrs.contains(I) ||
          InterestingBooleans.contains(I)) {
        I->dump();
        todo();
      }
    }
  }
  return std::make_pair(Results, nullptr);
}

SolverResult SolveConstraints(Z3ConstraintSolver &Ctx, solver &Solver,
                              const Value *InitialPtr,
                              const Value *InitialInteger) {
  SolverResult result;
  Solver.push();
  Solver.add(Ctx.capacity(Ctx.ptr_const(InitialPtr)) !=
             Ctx.int_const(InitialInteger));
  result.IsCapacity = Solver.check() == unsat;
  llvm::errs() << Solver.to_smt2() << '\n';
  Solver.pop();

  Solver.push();
  Solver.add(Ctx.capacity(Ctx.ptr_const(InitialPtr)) <
             Ctx.int_const(InitialInteger));
  result.LeqCapacity = Solver.check() == unsat;
  Solver.pop();

  Solver.push();
  Solver.add(Ctx.length(Ctx.ptr_const(InitialPtr)) !=
             Ctx.int_const(InitialInteger));
  result.IsLength = Solver.check() == unsat;
  Solver.pop();

  Solver.push();
  Solver.add(Ctx.length(Ctx.ptr_const(InitialPtr)) <
             Ctx.int_const(InitialInteger));
  result.LeqLength = Solver.check() == unsat;
  Solver.pop();

  llvm::outs() << "IsCapacity: " << result.IsCapacity << '\n';
  llvm::outs() << "LeqCapacity: " << result.LeqCapacity << '\n';
  llvm::outs() << "IsLength: " << result.IsLength << '\n';
  llvm::outs() << "LeqLength: " << result.LeqLength << '\n';
  return result;
}

SolverResult AnalyzeCB(Z3ConstraintSolver &Ctx, const CallBase *CB,
                       unsigned PtrIndex, unsigned IntegerIndex) {
  assert(CB != nullptr);
#ifndef NDEBUG
  llvm::errs() << "[PointerDefResolution] AnalyzeCB: " << CB->getName() << "\n";
  llvm::errs() << "[PointerDefResolution] AnalyzeCB: ";
  CB->dump();
#endif
  solver &Solver = Ctx.solver;
  const Value *InitialPtr = CB->getOperand(PtrIndex);
  const Value *InitialInteger = CB->getOperand(IntegerIndex);
  DenseSet<const Value *> InterestingPtrs;
  DenseSet<const Value *> InterestingIntegers;
  InterestingPtrs.insert(InitialPtr);
  InterestingIntegers.insert(InitialInteger);
  // auto DominatorBBLastInst = CB;
  {
    const auto [constraints, phi] =
        AnalyzeBB(Ctx, CB->getPrevNonDebugInstruction(), InterestingPtrs,
                  InterestingIntegers, std::nullopt);
    for (const auto &e : constraints) {
      Solver.add(e);
    }
    auto result = SolveConstraints(Ctx, Solver, InitialPtr, InitialInteger);
    if (result.IsCapacity || result.IsLength || result.LeqCapacity ||
        result.LeqLength) {
      return result;
    }
    const auto bb0 = phi->getIncomingBlock(0);
    const auto bb1 = phi->getIncomingBlock(1);
    if (bb0->getUniquePredecessor() == nullptr ||
        bb1->getUniquePredecessor() == nullptr ||
        bb0->getUniquePredecessor() != bb1->getUniquePredecessor()) {
      todo();
    }
    const auto dom = bb0->getUniquePredecessor();
    InterestingIntegers.insert(phi->getIncomingValue(0));
    InterestingIntegers.insert(phi->getIncomingValue(1));
    auto [bb0_constraints, bb0_phi] =
        AnalyzeBB(Ctx, &*bb0->rbegin(), InterestingPtrs, InterestingIntegers,
                  std::nullopt);
    bb0_constraints.push_back(Ctx.int_const(phi) == Ctx.int_const(phi->getIncomingValue(0)));
    if (bb0_phi != nullptr) {
      todo();
    }
    auto [bb1_constraints, bb1_phi] =
        AnalyzeBB(Ctx, &*bb1->rbegin(), InterestingPtrs, InterestingIntegers,
                  std::nullopt);
    bb1_constraints.push_back(Ctx.int_const(phi) == Ctx.int_const(phi->getIncomingValue(1)));
    if (bb1_phi != nullptr) {
      todo();
    }
    const DominanceInfo di{
        bb0_constraints,
        bb1_constraints,
    };
    const auto [dom_constraints, dom_phi] = AnalyzeBB(
        Ctx, &*dom->rbegin(), InterestingPtrs, InterestingIntegers, di);
    for (const auto &e : dom_constraints) {
      Solver.add(e);
    }
    result = SolveConstraints(Ctx, Solver, InitialPtr, InitialInteger);
    if (result.IsCapacity || result.IsLength || result.LeqCapacity ||
        result.LeqLength) {
      return result;
    }
    todo();
  }
  todo();
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
          return;
        }
      }
    }
  }
}

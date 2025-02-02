//=============================================================================
// FILE:
//    HelloWorld.cpp
//
// DESCRIPTION:
//    Visits all functions in a module, prints their names and the number of
//    arguments via stderr. Strictly speaking, this is an analysis pass (i.e.
//    the functions are not modified). However, in order to keep things simple
//    there's no 'print' method here (every analysis pass should implement it).
//
// USAGE:
//    New PM
//      opt -load-pass-plugin=libHelloWorld.dylib -passes="hello-world" `\`
//        -disable-output <input-llvm-file>
//
//
// License: MIT
//=============================================================================
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"
#include <filesystem>

using namespace llvm;
using std::make_tuple;
//-----------------------------------------------------------------------------
// HelloWorld implementation
//-----------------------------------------------------------------------------
// No need to expose the internals of the pass to the outside world - keep
// everything in an anonymous namespace.
namespace {

// This method implements what the pass does
void visitor(Function &F) {
  int id = 0;
  DenseMap<Value *, int> valueToId;
  StringMap<int> exprToId;

  auto createValue = [&](Value *value) {
    auto it = valueToId.find(value);
    if (it != valueToId.end()) {
      return make_tuple(it->getSecond(), true);
    }
    id++;
    valueToId[value] = id;
    return make_tuple(id, false);
  };

  auto createExpr = [&](StringRef expr) {
    auto it = exprToId.find(expr);
    if (it != exprToId.end()) {
      return make_tuple(it->getValue(), true);
    }
    id++;
    exprToId[expr] = id;
    return make_tuple(id, false);
  };

  auto dupValue = [&]<bool print = true>(int refValueId, Value *value) {
    valueToId[value] = refValueId;
    if (print) {
      errs() << formatv("{0} = {0}", refValueId);
    }
  };

  auto evaluateExpr = [&](Value *lhs, StringRef rhs) {
    const auto [refExprId, exprRedundant] = createExpr(rhs);
    errs() << formatv("{0} = {1}", refExprId, rhs);
    if (exprRedundant) {
      errs() << " (redundant)";
    }
    dupValue.template operator()<false>(refExprId, lhs);
  };

  for (auto &B : F) {
    for (auto &I : B) {
      if (dyn_cast<AllocaInst>(&I) || dyn_cast<ReturnInst>(&I)) {
        continue;
      }
      errs() << formatv("{0,-40}", I);
      if (auto *SI = dyn_cast<StoreInst>(&I)) {
        const auto valueId = get<0>(createValue(SI->getValueOperand()));
        dupValue(valueId, SI->getPointerOperand());
      } else if (auto *LI = dyn_cast<LoadInst>(&I)) {
        const auto ptrId = get<0>(createValue(LI->getPointerOperand()));
        dupValue(ptrId, LI);
      } else if (I.isBinaryOp()) {
        assert(I.getOpcode() == Instruction::Add ||
               I.getOpcode() == Instruction::Sub ||
               I.getOpcode() == Instruction::Mul ||
               I.getOpcode() == Instruction::SDiv ||
               I.getOpcode() == Instruction::UDiv);
        const auto op0 = I.getOperand(0);
        const auto op1 = I.getOperand(1);
        const auto op0Id = get<0>(createValue(op0));
        const auto op1Id = get<0>(createValue(op1));
        const auto exprName = I.getOpcodeName();
        const auto expr = formatv("{0} {1} {2}", op0Id, exprName, op1Id).str();
        evaluateExpr(&I, expr);
      }
      errs() << "\n";
    }
  }
}

// New PM implementation
struct HelloWorld : PassInfoMixin<HelloWorld> {
  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be)
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    errs() << "ValueNumbering: ";
    const auto M = F.getParent();
    try {
      const auto stem = std::filesystem::path(M->getSourceFileName()).stem();
      errs() << stem;
    } catch (std::exception &e) {
      errs() << "<unknown>";
    }
    errs() << '\n';
    visitor(F);
    return PreservedAnalyses::all();
  }

  // Without isRequired returning true, this pass will be skipped for functions
  // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
  // all functions with optnone.
  static bool isRequired() { return true; }
};
} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getHelloWorldPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "HelloWorld", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "hello-world") {
                    FPM.addPass(HelloWorld());
                    return true;
                  }
                  return false;
                });
          }};
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize HelloWorld when added to the pass pipeline on the
// command line, i.e. via '-passes=hello-world'
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getHelloWorldPluginInfo();
}

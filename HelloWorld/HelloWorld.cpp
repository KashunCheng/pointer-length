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
#include "PointerDefResolution.h"
#include "ReversedCallGraph.h"

#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;
//-----------------------------------------------------------------------------
// HelloWorld implementation
//-----------------------------------------------------------------------------
// No need to expose the internals of the pass to the outside world - keep
// everything in an anonymous namespace.
namespace {

cl::opt<std::string>
    TargetFuncName("func", cl::desc("Name of the function to trace"),
                   cl::value_desc("function name"), cl::init(""));

// New PM implementation
struct HelloWorld : PassInfoMixin<HelloWorld> {
  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be)
  [[maybe_unused]] static PreservedAnalyses run(Module &M,
                                                ModuleAnalysisManager &MAM) {
    // 1) Build/get the CallGraph:
    const CallGraph &CG = MAM.getResult<CallGraphAnalysis>(M);
    const ReversedCallGraph RCG(CG);
    // 2) Find the node for your "interesting" Function:
    const Function *F = M.getFunction(TargetFuncName);
    if (!F) {
      errs() << "ERROR: no function called '" << TargetFuncName
             << "' in module\n";
      return PreservedAnalyses::all();
    }
    ParamsInfo PI;
    for (auto &arg:F->args()) {
      if (arg.getType()->isPointerTy() || arg.getType()->isArrayTy()) {
        PI.PointerIndices.push_back(arg.getArgNo());
      }else if (arg.getType()->isIntegerTy()) {
        PI.IntegerIndices.push_back(arg.getArgNo());
      }
    }
    if (PI.PointerIndices.empty()) {
      errs() << "ERROR: no pointer argument in function '" << TargetFuncName
             << "'\n";
      return PreservedAnalyses::all();
    }
    if (PI.IntegerIndices.empty()) {
      errs() << "ERROR: no integer argument in function '" << TargetFuncName
             << "'\n";
      return PreservedAnalyses::all();
    }
    PointerDefResolution(CG, RCG, F, PI);
    return PreservedAnalyses::all();
  }

  // Without isRequired returning true, this pass will be skipped for functions
  // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
  // all functions with optnone.
  [[maybe_unused]] static bool isRequired() { return true; }
};
} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getHelloWorldPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "HelloWorld", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "hello-world") {
                    MPM.addPass(HelloWorld());
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

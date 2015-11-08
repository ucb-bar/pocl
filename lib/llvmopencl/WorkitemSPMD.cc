// LLVM function pass to simply initialize local variables assuming
// an explicitly parallel machines will run the code for the whole work group
// 
// Copyright (c) 2012-2013 Pekka Jääskeläinen / Tampere University of Technology
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define DEBUG_TYPE "workitem-spmd"

#include "WorkitemSPMD.h"
#include "Workgroup.h"
#include "Barrier.h"
#include "Kernel.h"
#include "config.h"
#include "pocl.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/CommandLine.h"
#ifdef LLVM_3_1
#include "llvm/Support/IRBuilder.h"
#include "llvm/Support/TypeBuilder.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/ValueSymbolTable.h"
#elif defined LLVM_3_2
#include "llvm/IRBuilder.h"
#include "llvm/TypeBuilder.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/ValueSymbolTable.h"
#else
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ValueSymbolTable.h"
#endif
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "WorkitemHandlerChooser.h"

#include <iostream>
#include <map>
#include <sstream>
#include <vector>

//#define DUMP_CFGS

#include "DebugHelpers.h"

//#define DEBUG_WORK_ITEM_SPMD

#include "VariableUniformityAnalysis.h"

#define CONTEXT_ARRAY_ALIGN 64

using namespace llvm;
using namespace pocl;

namespace {
  static
  RegisterPass<WorkitemSPMD> X("workitemspmd", 
                                "Workitem spmd generation pass");
}

char WorkitemSPMD::ID = 0;

void
WorkitemSPMD::getAnalysisUsage(AnalysisUsage &AU) const
{

  AU.addRequired<PostDominatorTree>();
#ifdef LLVM_OLDER_THAN_3_7
  AU.addRequired<LoopInfo>();
#else
  AU.addRequired<LoopInfoWrapperPass>();
#endif
#ifdef LLVM_3_1
  AU.addRequired<TargetData>();
#endif
#if (defined LLVM_3_2 || defined LLVM_3_3 || defined LLVM_3_4)
  AU.addRequired<DominatorTree>();
#else
  AU.addRequired<DominatorTreeWrapperPass>();
#endif

  AU.addRequired<VariableUniformityAnalysis>();
  AU.addPreserved<pocl::VariableUniformityAnalysis>();

  AU.addRequired<pocl::WorkitemHandlerChooser>();
  AU.addPreserved<pocl::WorkitemHandlerChooser>();

}

bool
WorkitemSPMD::runOnFunction(Function &F)
{
  if (!Workgroup::isKernelToProcess(F))
    return false;

  if (getAnalysis<pocl::WorkitemHandlerChooser>().chosenHandler() != 
      pocl::WorkitemHandlerChooser::POCL_WIH_SPMD)
    return false;

  #if (defined LLVM_3_2 || defined LLVM_3_3 || defined LLVM_3_4)
  DT = &getAnalysis<DominatorTree>();
  #else
  DTP = &getAnalysis<DominatorTreeWrapperPass>();
  DT = &DTP->getDomTree();
  #endif
#ifdef LLVM_OLDER_THAN_3_7
  LI = &getAnalysis<LoopInfo>();
#else
  LI = &getAnalysis<LoopInfoWrapperPass>();
#endif
  PDT = &getAnalysis<PostDominatorTree>();

//  F.viewCFGOnly();

  bool changed = ProcessFunction(F);

#ifdef DUMP_CFGS
  dumpCFG(F, F.getName().str() + "_after_wispmd.dot", 
          original_parallel_regions);
#endif

#if 0
  std::cerr << "### after:" << std::endl;
  F.viewCFG();
#endif

#if (defined LLVM_3_2 || defined LLVM_3_3 || defined LLVM_3_4)
  changed |= fixUndominatedVariableUses(DT, F);
#else
  changed |= fixUndominatedVariableUses(DTP, F);
#endif

#if 0
  /* Split large BBs so we can print the Dot without it crashing. */
  changed |= chopBBs(F, *this);
  F.viewCFG();
#endif
  contextArrays.clear();

  return changed;
}

bool
WorkitemSPMD::ProcessFunction(Function &F)
{
  Kernel *K = cast<Kernel> (&F);
  Initialize(K);

  K->addLocalSizeInitCode(WGLocalSizeX, WGLocalSizeY, WGLocalSizeZ);
  ParallelRegion::insertLocalIdInit(&F.getEntryBlock(), 0, 0, 0);
  return true;
  // colins FIXME: how does spmd handle multi-dim workgroups
}

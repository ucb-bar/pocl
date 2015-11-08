// Header for WorkitemSPMD function pass.
// 
// Copyright (c) 2012 Pekka Jääskeläinen / TUT
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

#ifndef _POCL_WORKITEM_SPMD_H
#define _POCL_WORKITEM_SPMD_H

#include "pocl.h"

#if (defined LLVM_3_2 || defined LLVM_3_3 || defined LLVM_3_4)
#include "llvm/Analysis/Dominators.h"
#endif

#include "llvm/ADT/Twine.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include <map>
#include <vector>
#include "WorkitemHandler.h"
#include "ParallelRegion.h"

namespace llvm {
  struct PostDominatorTree;
}

namespace pocl {
  class Workgroup;

  class WorkitemSPMD : public pocl::WorkitemHandler {

  public:
    static char ID;

  WorkitemSPMD() : pocl::WorkitemHandler(ID) {}

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
    virtual bool runOnFunction(llvm::Function &F);

  private:

    typedef std::vector<llvm::BasicBlock *> BasicBlockVector;
    typedef std::set<llvm::Instruction* > InstructionIndex;
    typedef std::vector<llvm::Instruction* > InstructionVec;
    typedef std::map<std::string, llvm::Instruction*> StrInstructionMap;

    llvm::DominatorTree *DT;
#ifdef LLVM_OLDER_THAN_3_7
    llvm::LoopInfo *LI;
#else
    llvm::LoopInfoWrapperPass *LI;
#endif
    llvm::PostDominatorTree *PDT;
#if ! (defined LLVM_3_2 || defined LLVM_3_3 || defined LLVM_3_4)
    llvm::DominatorTreeWrapperPass *DTP;
#endif

    ParallelRegion::ParallelRegionVector *original_parallel_regions;

    StrInstructionMap contextArrays;

    virtual bool ProcessFunction(llvm::Function &F);

  };
}

#endif

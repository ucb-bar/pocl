// Header for AutomaticLocals.cc module pass.
// 
// Copyright (c) 2011 Universidad Rey Juan Carlos
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

#ifndef _POCL_AUTOMATICLOCALS_H
#define _POCL_AUTOMATICLOCALS_H

#include "config.h"
#if (defined LLVM_3_1 or defined LLVM_3_2)
#include "llvm/Module.h"
#include "llvm/Function.h"
#else
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#endif
#include "llvm/Pass.h"

namespace pocl {
  class AutomaticLocals : public llvm::ModulePass {
  
  public:
    static char ID;
    AutomaticLocals() : ModulePass(ID) {}
    
    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
    virtual bool runOnModule(llvm::Module &M);

  private:
    llvm::Function *ProcessAutomaticLocals(llvm::Function *F);
  };
}

#endif

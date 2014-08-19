/*
    Copyright (c) 2012 Tampere University of Technology.

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
 */
/**
 * @file WorkItemAliasAnalysis.h
 *
 * Declaration of WorkItemAliasAnalysis class.
 *
 * @author Vladimír Guzma 2012
 */
#ifndef _POCL_WORKITEMALIASANALYSIS_H
#define _POCL_WORKITEMALIASANALYSIS_H

#include "config.h"

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Pass.h"

namespace pocl{
/// WorkItemAliasAnalysis - This is a simple alias analysis
/// implementation that uses pocl metadata to make sure memory accesses from
/// different work items are not aliasing.
class WorkItemAliasAnalysis : public llvm::ImmutablePass, public llvm::AliasAnalysis {
public:
    static char ID; 
    WorkItemAliasAnalysis() : ImmutablePass(ID) {}

    /// getAdjustedAnalysisPointer - This method is used when a pass implements
    /// an analysis interface through multiple inheritance.  If needed, it
    /// should override this to adjust the this pointer as needed for the
    /// specified pass info.
    virtual void *getAdjustedAnalysisPointer(llvm::AnalysisID PI) {
        if (PI == &AliasAnalysis::ID)
            return (AliasAnalysis*)this;
        return this;
    }
    virtual void initializePass() {
        InitializeAliasAnalysis(this);
    }
    
    private:
        virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
        virtual AliasAnalysis::AliasResult alias(const Location &LocA, const Location &LocB);

    };
}

#endif

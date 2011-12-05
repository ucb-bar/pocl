// LLVM function pass add required barriers to loops.
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

#include "LoopBarriers.h"
#include "Workgroup.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"

using namespace llvm;
using namespace pocl;

#define BARRIER_FUNCTION_NAME "barrier"

static bool is_barrier(Instruction *i);
static CallInst *new_barrier();

static Function *barrier = NULL;

namespace {
  static
  RegisterPass<LoopBarriers> X("loop-barriers",
                               "Add needed barriers to loops");
}

char LoopBarriers::ID = 0;

void
LoopBarriers::getAnalysisUsage(AnalysisUsage &AU) const
{
  AU.addRequired<DominatorTree>();
  AU.addPreserved<DominatorTree>();
}

bool
LoopBarriers::doInitialization(Loop *L, LPPassManager &LPM)
{
  Module *m = L->getHeader()->getParent()->getParent();
  barrier = m->getFunction(BARRIER_FUNCTION_NAME);

  return false;
}

bool
LoopBarriers::runOnLoop(Loop *L, LPPassManager &LPM)
{
  if (!Workgroup::isKernelToProcess(*L->getHeader()->getParent()))
    return false;

  DT = &getAnalysis<DominatorTree>();

  bool changed = ProcessLoop(L, LPM);

  DT->verifyAnalysis();

  return changed;
}


bool
LoopBarriers::ProcessLoop(Loop *L, LPPassManager &LPM)
{
  for (Loop::block_iterator i = L->block_begin(), e = L->block_end();
       i != e; ++i) {
    for (BasicBlock::iterator j = (*i)->begin(), e = (*i)->end();
         j != e; ++j) {
      if (is_barrier(j)) {
        // Found a barrier on this loop, proceed:
        // 1) add a barrier on the loop preheader.
        // 2) add a barrier on the latches
        
        // Add a barrier on the preheader to ensure all WIs reach
        // the loop header with all the previous code already 
        // executed.
        BasicBlock *preheader = L->getLoopPreheader();
        if (preheader == NULL)
          report_fatal_error("Non-canonicalized loop found!\n");
        if ((preheader->size() == 1) ||
            (!is_barrier(preheader->getTerminator()->getPrevNode()))) {
          // Avoid adding a barrier here if there is already a barrier
          // just before the terminator.
          new_barrier()->insertBefore(preheader->getTerminator());
          preheader->setName(preheader->getName() + ".loopbarrier");
        }

        // Now add the barriers on the latches.
        BasicBlock *latch = L->getLoopLatch();
        if (latch != NULL) {
          // This loop has only one latch. Do not check for dominance, we
          // are probably running before BTR.
          // Avoid adding a barrier here if the latch happens to have a
          // barrier just before the terminator.
          if ((latch->size() == 1) ||
              (!is_barrier(latch->getTerminator()->getPrevNode()))) {
            new_barrier()->insertBefore(latch->getTerminator());
            latch->setName(latch->getName() + ".latchbarrier");
          }

          return true;
        }

        // Modified code from llvm::LoopBase::getLoopLatch to
        // go trough all the latches.
        BasicBlock *Header = L->getHeader();
        typedef GraphTraits<Inverse<BasicBlock *> > InvBlockTraits;
        InvBlockTraits::ChildIteratorType PI = InvBlockTraits::child_begin(Header);
        InvBlockTraits::ChildIteratorType PE = InvBlockTraits::child_end(Header);
        BasicBlock *Latch = NULL;
        for (; PI != PE; ++PI) {
          InvBlockTraits::NodeType *N = *PI;
          if (L->contains(N)) {
            Latch = N;
            // Latch found in the loop, see if the barrier dominates it
            // (otherwise if might no even belong to this "tail", see
            // forifbarrier1 graph test).
            if (DT->dominates(j->getParent(), Latch)) {
              // If there is a barrier happens before the latch terminator,
              // there is no need to add an additional barrier.
              if ((Latch->size() == 1) ||
                  (!is_barrier(Latch->getTerminator()->getPrevNode()))) {
                new_barrier()->insertBefore(Latch->getTerminator());
                Latch->setName(Latch->getName() + ".latchbarrier");
              }
            }
          }
        }

        return true;
      }
    }
  }

  return false;
}


static bool
is_barrier(Instruction *i)
{
  if (CallInst *c = dyn_cast<CallInst>(i)) {
    if (Function *f = c->getCalledFunction()) {
      if (f == barrier)
        return true;
    }
  }

  return false;
}

static CallInst *
new_barrier()
{
  assert (barrier != NULL && "No barrier function!");
  Constant *zero =
    ConstantInt::get(barrier->getArgumentList().front().getType(), 0);
  SmallVector<Value *, 1> sv(1, zero);
  return CallInst::Create(barrier, ArrayRef<Value *>(sv));
}
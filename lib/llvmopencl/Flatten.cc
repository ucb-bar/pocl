// LLVM module pass to inline required functions (those accessing
// per-workgroup variables) into the kernel.
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

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"

using namespace llvm;

namespace {
  class Flatten : public ModulePass {

  public:
    static char ID;
    Flatten() : ModulePass(ID) {}

    virtual bool runOnModule(Module &M);
  };

}

char Flatten::ID = 0;
static RegisterPass<Flatten> X("flatten", "Kernel function flattening pass");

static const char *workgroup_variables[] = {
  "_local_id_x", "_local_id_y", "_local_id_z",
  "_local_size_x", "_local_size_y", "_local_size_z",
  "_work_dim",
  "_num_groups_x", "_num_groups_y", "_num_groups_z",
  "_group_id_x", "_group_id_y", "_group_z",
  "_global_offset_x", "_global_offset_y", "_global_offset_z",
  NULL};

bool
Flatten::runOnModule(Module &M)
{
  SmallPtrSet<Function *, 8> functions_to_inline;
  SmallVector<Value *, 8> pending;

  const char **s = workgroup_variables;
  while (*s != NULL) {
    GlobalVariable *gv = M.getGlobalVariable(*s);
    if (gv != NULL)
      pending.push_back(gv);

    ++s;
  }

  while (!pending.empty()) {
    Value *v = pending.back();
    pending.pop_back();

    if (Function *f = dyn_cast<Function>(v)) {
      // Prevent infinite looping on recursive functions
      // (though OpenCL does not allow this?)
      if (functions_to_inline.count(f))
	continue;

      functions_to_inline.insert(f);
    }

    for (Value::use_iterator i = v->use_begin(), e = v->use_end();
	 i != e; ++i)
      pending.push_back(*i);
  }

  for (SmallPtrSet<Function *, 8>::iterator i = functions_to_inline.begin(),
	 e = functions_to_inline.end();
       i != e; ++i) {
    (*i)->removeFnAttr(Attribute::NoInline);
    (*i)->addFnAttr(Attribute::AlwaysInline);
  }

  return true;
}
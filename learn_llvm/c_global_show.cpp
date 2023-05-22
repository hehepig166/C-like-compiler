#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"

using namespace std;
using namespace llvm;


int main()
{

    auto llvmContext    = make_unique<LLVMContext>();
    auto module         = make_unique<Module>("ir_global", *llvmContext);
    auto irBuilder      = make_unique<IRBuilder<>>(*llvmContext);

    // int a;
    module->getOrInsertGlobal("a", irBuilder->getInt32Ty());

    module->print(errs(), nullptr);

}
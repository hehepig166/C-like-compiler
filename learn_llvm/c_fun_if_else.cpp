#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"

#include <vector>

using namespace std;
using namespace llvm;


int max(int a, int b) {
    int c;
    if (a<b) {
        c = b;
    } else {
        c = a;
    }
    return c;
}


int main()
{

    auto llvmContext    = make_unique<LLVMContext>();
    auto module         = make_unique<Module>("ir_global", *llvmContext);
    auto irBuilder      = make_unique<IRBuilder<>>(*llvmContext);

    Type *intType = irBuilder->getInt32Ty();

    // 函数类型
    auto fType  = FunctionType::get(intType, {intType, intType}, false);

    // 函数实体
    Function *func = Function::Create(fType, GlobalValue::ExternalLinkage, "max", *module);

    vector<string> param{"a", "b"};
    int idx = 0;
    for (Function::arg_iterator it = func->arg_begin(); it != func->arg_end(); ++it, ++idx) {
        it->setName(param[idx]);
    }

    // 基本块
    BasicBlock *entry = BasicBlock::Create(*llvmContext, "entry", func);
    BasicBlock *thenBB = BasicBlock::Create(*llvmContext, "", func);
    BasicBlock *elseBB = BasicBlock::Create(*llvmContext, "", func);
    BasicBlock *mergeBB = BasicBlock::Create(*llvmContext, "", func);

    irBuilder->SetInsertPoint(entry);
    Value *c = irBuilder->CreateAlloca(irBuilder->getInt32Ty(), nullptr, "c");
    Value *cmp_ret = irBuilder->CreateICmpSLT(func->getArg(0), func->getArg(1));
    irBuilder->CreateCondBr(cmp_ret, thenBB, elseBB);

    irBuilder->SetInsertPoint(thenBB);
    irBuilder->CreateStore(func->getArg(1), c);
    irBuilder->CreateBr(mergeBB);

    irBuilder->SetInsertPoint(elseBB);
    irBuilder->CreateStore(func->getArg(0), c);

    irBuilder->SetInsertPoint(mergeBB);
    Value *c_val = irBuilder->CreateLoad(irBuilder->getInt32Ty(), c);
    irBuilder->CreateRet(c_val);

    //irBuilder->CreateRet(irBuilder->getInt32(0));

    verifyFunction(*func);
    module->print(errs(), nullptr);

    return 0;
}
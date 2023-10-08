#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "IRBuilder.hpp"
#include "Module.hpp"
#include "Type.hpp"

#include <iostream>
#include <memory>

// 定义一个从常数值获取/创建 ConstantInt 类实例化的宏，方便多次调用
#define CONST_INT(num) \
    ConstantInt::get(num, module)

// 定义一个从常数值获取/创建 ConstantFP 类实例化的宏，方便多次调用
#define CONST_FP(num) \
    ConstantFP::get(num, module)

int main() {
    // 创建一个 Module 实例
    auto module = new Module();
    // 创建一个 IRBuilder 实例（后续创建指令均使用此实例操作）
    auto builder = new IRBuilder(nullptr, module);
    // 从 Module 处取出 32 位整形 type 的实例
    Type *Int32Type = module->get_int32_type();

    // main()
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}), "main", module);
    auto bb = BasicBlock::create(module, "entry", mainFun);
    builder->set_insert_point(bb);
    // int a; int i;
    auto aAlloca = builder->create_alloca(Int32Type);
    auto iAlloca = builder->create_alloca(Int32Type);
    // a = 10; i = 0;
    builder->create_store(CONST_INT(10), aAlloca);
    auto aLoad = builder->create_load(aAlloca);
    builder->create_store(CONST_INT(0), iAlloca);
    auto iLoad = builder->create_load(iAlloca);
    // while (i < 10)
    auto loop = BasicBlock::create(module, "loop", mainFun);
    auto loopEnd = BasicBlock::create(module, "loopEnd", mainFun);
    auto icmp = builder->create_icmp_lt(iLoad, CONST_INT(10));
    builder->create_cond_br(icmp, loop, loopEnd);
    // loop
    builder->set_insert_point(loop);
    // i = i + 1;
    iLoad = builder->create_load(iAlloca);
    auto add = builder->create_iadd(iLoad, CONST_INT(1));
    builder->create_store(add, iAlloca);
    // a = a + i;
    aLoad = builder->create_load(aAlloca);
    add = builder->create_iadd(aLoad, iLoad);
    builder->create_store(add, aAlloca);
    // br loop
    icmp = builder->create_icmp_lt(iLoad, CONST_INT(10));
    builder->create_cond_br(icmp, loop, loopEnd);
    // loopEnd
    builder->set_insert_point(loopEnd);
    // return a;
    aLoad = builder->create_load(aAlloca);
    builder->create_ret(aLoad);

    // 输出 module 中的所有 IR 指令
    std::cout << module->print();
    delete module;
    return 0;
}

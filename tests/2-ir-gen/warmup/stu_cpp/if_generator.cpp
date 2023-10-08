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
    Type *FloatType = module->get_float_type();

    // main()
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}), "main", module);
    auto bb = BasicBlock::create(module, "entry", mainFun);
    builder->set_insert_point(bb);
    // float a = 5.555
    auto aAlloca = builder->create_alloca(FloatType);
    builder->create_store(CONST_FP(5.555), aAlloca);
    // if (a > 1) statement
    auto aLoad = builder->create_load(aAlloca);
    auto icmp = builder->create_fcmp_gt(aLoad, CONST_FP(1));
    auto trueBB = BasicBlock::create(module, "trueBB", mainFun);
    auto falseBB = BasicBlock::create(module, "falseBB", mainFun);
    builder->create_cond_br(icmp, trueBB, falseBB);
    builder->set_insert_point(trueBB);
    // True
    builder->create_ret(CONST_INT(233));
    // False
    builder->set_insert_point(falseBB);
    builder->create_ret(CONST_INT(0));

    // 输出 module 中的所有 IR 指令
    std::cout << module->print();
    delete module;
    return 0;
}

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

    // callee()
    // 声明函数参数类型的 vector，此处的 vector 数组含有两个元素，每个均为 Int32Type，表明待创建函数有两个参数，类型均为 Int32Type 实例代表的类型
    std::vector<Type *> IntArg(1, Int32Type);
    auto calleeFun = Function::create(FunctionType::get(Int32Type, IntArg), "callee", module);
    auto bb = BasicBlock::create(module, "entry", calleeFun);
    builder->set_insert_point(bb);
    auto retAlloca = builder->create_alloca(Int32Type);
    builder->create_store(CONST_INT(0), retAlloca);
    // args: int a
    auto aAlloca = builder->create_alloca(Int32Type);
    std::vector<Value *> args;
    for (auto &arg: calleeFun->get_args()) {
        args.push_back(&arg);
    }
    builder->create_store(args[0], aAlloca);
    // return 2 * a
    auto aLoad = builder->create_load(aAlloca);
    auto aMul2 = builder->create_imul(CONST_INT(2), aLoad);
    builder->create_ret(aMul2);

    // main()
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}), "main", module);
    bb = BasicBlock::create(module, "entry", mainFun);
    builder->set_insert_point(bb);
    retAlloca = builder->create_alloca(Int32Type);
    builder->create_store(CONST_INT(0), retAlloca); // main 函数默认 ret 0
    // return callee(110)
    auto call = builder->create_call(calleeFun, {CONST_INT(110)});
    builder->create_ret(call);

    // 输出 module 中的所有 IR 指令
    std::cout << module->print();
    delete module;
    return 0;
}

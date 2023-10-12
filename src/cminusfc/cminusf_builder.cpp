#include "cminusf_builder.hpp"

#define CONST_FP(num) ConstantFP::get((float)num, module.get())
#define CONST_INT(num) ConstantInt::get(num, module.get())
#define GEN_LABEL(label_name) sprintf(label_name, "_generated_label_%08x", context.label++)

// Types
Type *VOID_T;
Type *INT1_T;
Type *INT32_T;
Type *INT32PTR_T;
Type *FLOAT_T;
Type *FLOATPTR_T;
// Zeros
ConstantZero *ZERO_INT;
ConstantZero *ZERO_FP;

/*
 * use CMinusfBuilder::Scope to construct scopes
 * scope.enter: enter a new scope
 * scope.exit: exit current scope
 * scope.push: add a new binding to current scope
 * scope.find: find and return the value bound to the name
 */

Value* CminusfBuilder::visit(ASTProgram &node) {
    VOID_T = module->get_void_type();
    INT1_T = module->get_int1_type();
    INT32_T = module->get_int32_type();
    INT32PTR_T = module->get_int32_ptr_type();
    FLOAT_T = module->get_float_type();
    FLOATPTR_T = module->get_float_ptr_type();
    ZERO_INT = ConstantZero::get(INT32_T, module.get());
    ZERO_FP = ConstantZero::get(FLOAT_T, module.get());

    Value *ret_val = nullptr;
    for (auto &decl : node.declarations) {
        ret_val = decl->accept(*this);
    }
    return ret_val;
}

Value* CminusfBuilder::visit(ASTNum &node) {
    if (node.type == TYPE_INT) {
        context.expression = CONST_INT(node.i_val);
    } else if (node.type == TYPE_FLOAT) {
        context.expression = CONST_FP(node.f_val);
    }
    return context.expression;
}

Value* CminusfBuilder::visit(ASTVarDeclaration &node) {
    GlobalVariable *g_var;
    if (node.num == nullptr) { // Declared a variable
        if (node.type == TYPE_INT) {
            g_var = GlobalVariable::create(node.id, module.get(), INT32PTR_T, false, ZERO_INT);
        } else if (node.type == TYPE_FLOAT) {
            g_var = GlobalVariable::create(node.id, module.get(), FLOATPTR_T, false, ZERO_FP);
        }
    } else { // Declared an array
        ArrayType *arr_type;
        if (node.type == TYPE_INT) {
            arr_type = ArrayType::get(INT32_T, node.num->i_val);
        } else if (node.type == TYPE_FLOAT) {
            arr_type = ArrayType::get(FLOAT_T, node.num->i_val);
        }
        ConstantZero *zero_arr = ConstantZero::get(arr_type, module.get());
        g_var = GlobalVariable::create(node.id, module.get(), arr_type, false, zero_arr);
    }
    scope.push(node.id, g_var);
    return nullptr;
}

Value* CminusfBuilder::visit(ASTFunDeclaration &node) {
    FunctionType *fun_type;
    Type *ret_type;
    std::vector<Type *> param_types;
    if (node.type == TYPE_INT)
        ret_type = INT32_T;
    else if (node.type == TYPE_FLOAT)
        ret_type = FLOAT_T;
    else
        ret_type = VOID_T;

    for (auto &param : node.params) {
        if (param->isarray) {
            if (param->type == TYPE_INT)
                param_types.push_back(INT32PTR_T);
            else if (param->type == TYPE_FLOAT)
                param_types.push_back(FLOATPTR_T);
        } else {
            if (param->type == TYPE_INT)
                param_types.push_back(INT32_T);
            else if (param->type == TYPE_FLOAT)
                param_types.push_back(FLOAT_T);
        }
    }

    fun_type = FunctionType::get(ret_type, param_types);
    auto func = Function::create(fun_type, node.id, module.get());
    scope.push(node.id, func);
    context.func = func;
    auto funBB = BasicBlock::create(module.get(), "entry", func);
    builder->set_insert_point(funBB);
    scope.enter();
    std::vector<Value *> args;
    for (auto &arg : func->get_args()) {
        args.push_back(&arg);
    }
    auto arg = func->get_args().begin();
    for (int i = 0; i < node.params.size(); ++i) {
        auto param = node.params[i];
        if (arg == func->get_args().end())
            std::cerr << "Fatal error: too few arguments" << std::endl;
        Value *pv;
        if (param->isarray) {
            if (param->type == TYPE_INT) {
                pv = builder->create_alloca(INT32PTR_T);
            } else if (param->type == TYPE_FLOAT) {
                pv = builder->create_alloca(FLOATPTR_T);
            }
        } else {
            if (param->type == TYPE_INT) {
                pv = builder->create_alloca(INT32_T);
            } else if (param->type == TYPE_FLOAT) {
                pv = builder->create_alloca(FLOAT_T);
            }
        }
        scope.push(param->id, pv);
        builder->create_store(args[i], pv);
        arg++;
    }
    node.compound_stmt->accept(*this);
    if (builder->get_insert_block()->is_terminated()) {
        if (context.func->get_return_type()->is_void_type())
            builder->create_void_ret();
        else if (context.func->get_return_type()->is_float_type())
            builder->create_ret(CONST_FP(0.));
        else
            builder->create_ret(CONST_INT(0));
    }
    scope.exit();
    return nullptr;
}

Value* CminusfBuilder::visit(ASTParam &node) {
    return nullptr;
}

Value* CminusfBuilder::visit(ASTCompoundStmt &node) {
    // Deal with complex statements.
    scope.enter();
    for (auto &decl : node.local_declarations) {
        if (decl->type == TYPE_VOID)
            std::cerr << "Fatal error: void variable" << std::endl;
        if (decl->num != nullptr) { // Array
            Type *arr_type;
            if (decl->type == TYPE_INT)
                arr_type = ArrayType::get(INT32_T, decl->num->i_val);
            else if (decl->type == TYPE_FLOAT)
                arr_type = ArrayType::get(FLOAT_T, decl->num->i_val);
            auto arr_alloca = builder->create_alloca(arr_type);
            scope.push(decl->id, arr_alloca);
        } else { // Variable
            Type *var_type;
            if (decl->type == TYPE_INT)
                var_type = INT32_T;
            else if (decl->type == TYPE_FLOAT)
                var_type = FLOAT_T;
            auto var_alloca = builder->create_alloca(var_type);
            scope.push(decl->id, var_alloca);
        }
        decl->accept(*this);
    }
    context.is_returned = false;
    for (auto &stmt : node.statement_list) {
        stmt->accept(*this);
        if (builder->get_insert_block()->is_terminated())
            break;
    }
    context.is_returned_record = context.is_returned;
    context.is_returned = false;
    scope.exit();
    return nullptr;
}

Value* CminusfBuilder::visit(ASTExpressionStmt &node) {
    node.expression->accept(*this);
    return nullptr;
}

Value* CminusfBuilder::visit(ASTSelectionStmt &node) {
    node.expression->accept(*this);
    Value* cond;
    char label_name[32];
    if (context.expression->get_type()->is_float_type()) {
        cond = builder->create_fcmp_gt(context.expression, CONST_FP(0.));
    } else if (context.expression->get_type()->is_int32_type()) {
        cond = builder->create_icmp_ne(context.expression, CONST_INT(0));
    } else {
        cond = context.expression;
    }
    BasicBlock* origBB = context.curr_block; // save original basic block
    GEN_LABEL(label_name);
    BasicBlock* trueBB = BasicBlock::create(module.get(), label_name, context.func);
    BasicBlock* falseBB;
    bool trueBB_returned = context.is_returned_record;
    bool falseBB_returned = false;
    bool has_else = node.else_statement != nullptr;
    builder->set_insert_point(trueBB);
    context.curr_block = trueBB;
    node.if_statement->accept(*this);
    if (has_else) {
        GEN_LABEL(label_name);
        falseBB = BasicBlock::create(module.get(), label_name, context.func);
        builder->set_insert_point(falseBB);
        context.curr_block = falseBB;
        node.else_statement->accept(*this);
        falseBB_returned = context.is_returned_record;
    }
    GEN_LABEL(label_name);
    BasicBlock* endBB = BasicBlock::create(module.get(), label_name, context.func);
    builder->set_insert_point(origBB);
    if (has_else) {
        builder->create_cond_br(cond, trueBB, falseBB);
    } else {
        builder->create_cond_br(cond, trueBB, endBB);
    }
    if (!trueBB_returned) {
        builder->set_insert_point(trueBB);
        builder->create_br(endBB);
    }
    if (has_else && (!falseBB_returned || falseBB->empty())) {
        builder->set_insert_point(falseBB);
        builder->create_br(endBB);
    }
    builder->set_insert_point(endBB);
    context.curr_block = endBB;
    context.is_returned = false;
    return nullptr;
}

Value* CminusfBuilder::visit(ASTIterationStmt &node) {
    char label_name[32];
    GEN_LABEL(label_name);
    auto startBB = BasicBlock::create(module.get(), label_name, context.func);
    builder->create_br(startBB);
    builder->set_insert_point(startBB);
    context.curr_block = startBB;
    node.expression->accept(*this);
    Type *cond_type = context.expression->get_type();
    Value* cond;
    if (cond_type == INT32_T) {
        cond = builder->create_icmp_ne(context.expression, ZERO_INT);
    } else if (cond_type == FLOAT_T) {
        cond = builder->create_fcmp_ne(context.expression, ZERO_FP);
    } else {
        cond = context.expression;
    }
    GEN_LABEL(label_name);
    auto bodyBB = BasicBlock::create(module.get(), label_name, context.func);
    builder->set_insert_point(bodyBB);
    context.curr_block = bodyBB;
    node.statement->accept(*this);
    if (context.is_returned_record) {
        builder->create_br(startBB);
    }
    GEN_LABEL(label_name);
    auto endBB = BasicBlock::create(module.get(), label_name, context.func);
    builder->set_insert_point(startBB);
    builder->create_cond_br(cond, bodyBB, endBB);
    builder->set_insert_point(endBB);
    context.curr_block = endBB;
    context.is_returned = false;
    return nullptr;
}

Value* CminusfBuilder::visit(ASTReturnStmt &node) {
    if (node.expression == nullptr) {
        builder->create_void_ret();
        return nullptr;
    } else {
        Value* ret_val;
        node.expression->accept(*this);
        auto expr_type = context.expression->get_type();
        if (expr_type == INT1_T) {
            // Boolean automatically converted to int
            ret_val = builder->create_zext(context.expression, INT32_T);
        } else if (expr_type == INT32_T || expr_type == FLOAT_T) {
            ret_val = context.expression;
        } else {
            std::cerr << "Fatal error: return type not supported" << std::endl;
        }
    }
    builder->create_br(context.return_block);
    context.is_returned = true;
    context.is_returned_record = true;
    return nullptr;
}

Value* CminusfBuilder::visit(ASTVar &node) {
    // TODO: This function is empty now.
    // Add some code here.
    return nullptr;
}

Value* CminusfBuilder::visit(ASTAssignExpression &node) {
    // TODO: This function is empty now.
    // Add some code here.
    return nullptr;
}

Value* CminusfBuilder::visit(ASTSimpleExpression &node) {
    // TODO: This function is empty now.
    // Add some code here.
    return nullptr;
}

Value* CminusfBuilder::visit(ASTAdditiveExpression &node) {
    // TODO: This function is empty now.
    // Add some code here.
    return nullptr;
}

Value* CminusfBuilder::visit(ASTTerm &node) {
    // TODO: This function is empty now.
    // Add some code here.
    return nullptr;
}

Value* CminusfBuilder::visit(ASTCall &node) {
    // TODO: This function is empty now.
    // Add some code here.
    return nullptr;
}

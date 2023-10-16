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

    Value *ret_val = nullptr;
    for (auto &decl : node.declarations) {
        ret_val = decl->accept(*this);
    }
    return ret_val;
}

Value* CminusfBuilder::visit(ASTNum &node) {
    if (node.type == TYPE_INT) {
        context.value = CONST_INT(node.i_val);
    } else if (node.type == TYPE_FLOAT) {
        context.value = CONST_FP(node.f_val);
    }
    return nullptr;
}

Value* CminusfBuilder::visit(ASTVarDeclaration &node) {
    Type *var_type;
    if(node.num == nullptr){
        if(node.type == TYPE_INT)
            var_type = module->get_int32_type();
        else
            var_type = module->get_float_type();
    } else{
        if(node.type == TYPE_INT)
            var_type = ArrayType::get(module->get_int32_type(), node.num->i_val);
        else
            var_type = ArrayType::get(module->get_float_type(), node.num->f_val);
    }
    Value *g_var;
    if(scope.in_global() == 1)
        g_var = GlobalVariable::create(node.id, module.get(), var_type, false, ConstantZero::get(var_type, module.get()));
    else
        g_var = builder->create_alloca(var_type);
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
    // auto arg = func->get_args().begin();
    for (long unsigned int i = 0; i < node.params.size(); ++i) {
        Type *arg_type;
        if(node.params[i]->isarray == 1){
            if(node.params[i]->type == TYPE_INT)
                arg_type = INT32PTR_T;
            else
                arg_type = FLOATPTR_T;
        } else {
            if(node.params[i]->type == TYPE_INT)
                arg_type = INT32_T;
            else
                arg_type = FLOAT_T;
        }
        auto arg_alloca = builder->create_alloca(arg_type);
        builder->create_store(args[i], arg_alloca);
        scope.push(node.params[i]->id, arg_alloca);
    }
    node.compound_stmt->accept(*this);
    if (!builder->get_insert_block()->is_terminated()) {
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
        // if (decl->type == TYPE_VOID)
        //     std::cerr << "Fatal error: void variable" << std::endl;
        // if (decl->num != nullptr) { // Array
        //     Type *arr_type;
        //     if (decl->type == TYPE_INT)
        //         arr_type = ArrayType::get(INT32_T, decl->num->i_val);
        //     else if (decl->type == TYPE_FLOAT)
        //         arr_type = ArrayType::get(FLOAT_T, decl->num->i_val);
        //     auto arr_alloca = builder->create_alloca(arr_type);
        //     scope.push(decl->id, arr_alloca);
        // } else { // Variable
        //     Type *var_type;
        //     if (decl->type == TYPE_INT)
        //         var_type = INT32_T;
        //     else if (decl->type == TYPE_FLOAT)
        //         var_type = FLOAT_T;
        //     auto var_alloca = builder->create_alloca(var_type);
        //     scope.push(decl->id, var_alloca);
        // }
        decl->accept(*this);
    }
    // context.is_returned = false;
    for (auto &stmt : node.statement_list) {
        stmt->accept(*this);
        if (builder->get_insert_block()->is_terminated())
            break;
    }
    // context.is_returned_record = context.is_returned;
    // context.is_returned = false;
    scope.exit();
    return nullptr;
}

Value* CminusfBuilder::visit(ASTExpressionStmt &node) {
    if (node.expression != nullptr) {
        node.expression->accept(*this);
    }
    return nullptr;
}

Value* CminusfBuilder::visit(ASTSelectionStmt &node) {
    node.expression->accept(*this);
    char label_name[32];
    // Create basic blocks
    GEN_LABEL(label_name);
    auto trueBB = BasicBlock::create(module.get(), label_name, context.func);
    GEN_LABEL(label_name);
    auto falseBB = BasicBlock::create(module.get(), label_name, context.func);
    GEN_LABEL(label_name);
    auto endBB = BasicBlock::create(module.get(), label_name, context.func);
    // Create comparison to zero
    if(context.value->get_type()->is_integer_type())
        context.value = builder->create_icmp_ne(context.value, CONST_INT(0));
    else
        context.value = builder->create_fcmp_ne(context.value, CONST_FP(0));
    builder->create_cond_br(context.value, trueBB, falseBB);
    // True branch
    builder->set_insert_point(trueBB);
    scope.enter();
    node.if_statement->accept(*this);
    scope.exit();
    if (!builder->get_insert_block()->is_terminated())
        builder->create_br(endBB);
    // False branch
    builder->set_insert_point(falseBB);
    if (node.else_statement != nullptr) {
        scope.enter();
        node.else_statement->accept(*this);
        scope.exit();
        if (!builder->get_insert_block()->is_terminated()) {
            builder->create_br(endBB);
        }
    } else {
        builder->create_br(endBB);
    }
    builder->set_insert_point(endBB);
    return nullptr;
}

Value* CminusfBuilder::visit(ASTIterationStmt &node) {
    char label_name[32];
    GEN_LABEL(label_name); // Condition
    auto condBB = BasicBlock::create(module.get(), label_name, context.func);
    GEN_LABEL(label_name); // Iteration body
    auto bodyBB = BasicBlock::create(module.get(), label_name, context.func);
    GEN_LABEL(label_name); // End
    auto endBB = BasicBlock::create(module.get(), label_name, context.func);

    if (!builder->get_insert_block()->is_terminated())
        builder->create_br(condBB);
    builder->set_insert_point(condBB);
    node.expression->accept(*this);
    if (context.value->get_type()->is_integer_type())
        context.value = builder->create_icmp_ne(context.value, CONST_INT(0));
    else
        context.value = builder->create_fcmp_ne(context.value, CONST_FP(0));

    builder->create_cond_br(context.value, bodyBB, endBB);
    builder->set_insert_point(bodyBB);
    scope.enter();
    node.statement->accept(*this);
    scope.exit();

    if (!builder->get_insert_block()->is_terminated())
        builder->create_br(condBB);

    builder->set_insert_point(endBB);
    return nullptr;
}

Value* CminusfBuilder::visit(ASTReturnStmt &node) {
    if (node.expression == nullptr) {
        builder->create_void_ret();
        return nullptr;
    } else {
        node.expression->accept(*this);
        auto ret_type = context.func->get_function_type()->get_return_type();

        if (ret_type != context.value->get_type()) { // Data type conversion
            if (context.value->get_type()->is_integer_type())
                context.value = builder->create_sitofp(context.value, FLOAT_T);
            else
                context.value = builder->create_fptosi(context.value, INT32_T);
        }
        builder->create_ret(context.value);
    }
    return nullptr;
}

Value* CminusfBuilder::visit(ASTVar &node) {
    context.value = scope.find(node.id);
    if (node.expression == nullptr) {
        if (context.lvalue == 0) {
            if (context.value->get_type()->get_pointer_element_type()->is_array_type())
                context.value = builder->create_gep(context.value, {CONST_INT(0), CONST_INT(0)});
            else
                context.value = builder->create_load(context.value);
        }
    } else {
        char label_name[32];
        auto *lval = context.value;
        bool tmp = context.lvalue;
        context.lvalue = false;
        node.expression->accept(*this);
        context.lvalue = tmp;
        auto *rval = context.value;
        if (rval->get_type()->is_float_type()) {
            rval = builder->create_fptosi(rval, INT32_T);
        }
        GEN_LABEL(label_name);
        auto trueBB = BasicBlock::create(module.get(), label_name, context.func);
        GEN_LABEL(label_name);
        auto falseBB = BasicBlock::create(module.get(), label_name, context.func);
        GEN_LABEL(label_name);
        auto endBB = BasicBlock::create(module.get(), label_name, context.func);
        auto *cond = builder->create_icmp_ge(rval, CONST_INT(0));
        builder->create_cond_br(cond, trueBB, falseBB);
        // True
        builder->set_insert_point(trueBB);
        if (lval->get_type()->get_pointer_element_type()->is_integer_type() || lval->get_type()->get_pointer_element_type()->is_float_type()) {
            context.value = builder->create_gep(lval, {rval});
        } else if (lval->get_type()->get_pointer_element_type()->is_pointer_type()) {
            lval = builder->create_load(lval);
            context.value = builder->create_gep(lval, {rval});
        } else {
            context.value = builder->create_gep(lval, {CONST_INT(0), rval});
        }
        if (context.lvalue == 0) {
            context.value = builder->create_load(context.value);
        }
        builder->create_br(endBB);
        // False
        builder->set_insert_point(falseBB);
        builder->create_call(scope.find("neg_idx_except"), {});
        builder->create_br(endBB);
        // End
        builder->set_insert_point(endBB);
    }
    return nullptr;
}

Value* CminusfBuilder::visit(ASTAssignExpression &node) {
    context.lvalue = true;
    node.var->accept(*this);
    context.lvalue = false;

    auto store = context.value;
    node.expression->accept(*this);
    auto var_alloca = context.value;
    auto var_type = var_alloca->get_type();

    if (store->get_type()->get_pointer_element_type() != var_type) {
        if (var_type->is_integer_type())
            store = builder->create_sitofp(store, FLOAT_T);
        else
            store = builder->create_fptosi(store, INT32_T);
    }
    builder->create_store(var_alloca, store);
    return nullptr;
}

Value* CminusfBuilder::visit(ASTSimpleExpression &node) {
    node.additive_expression_l->accept(*this);
    if (node.additive_expression_r == nullptr) { // Only left expression
        return nullptr;
    }
    auto *lval = context.value;
    node.additive_expression_r->accept(*this);
    auto *rval = context.value;
    // Data type conversion
    if (lval->get_type()->is_float_type() || context.value->get_type()->is_float_type()) { // Has float
        if (lval->get_type()->is_integer_type())
            lval = builder->create_sitofp(lval, FLOAT_T);
        if (rval->get_type()->is_integer_type())
            rval = builder->create_sitofp(rval, FLOAT_T);
        switch (node.op) {
            case OP_LT:
                context.value = builder->create_fcmp_lt(lval, rval);
                break;
            case OP_GT:
                context.value = builder->create_fcmp_gt(lval, rval);
                break;
            case OP_LE:
                context.value = builder->create_fcmp_le(lval, rval);
                break;
            case OP_GE:
                context.value = builder->create_fcmp_ge(lval, rval);
                break;
            case OP_EQ:
                context.value = builder->create_fcmp_eq(lval, rval);
                break;
            case OP_NEQ:
                context.value = builder->create_fcmp_ne(lval, rval);
                break;
        }
    } else {
        switch (node.op) {
            case OP_LT:
                context.value = builder->create_icmp_lt(lval, rval);
                break;
            case OP_GT:
                context.value = builder->create_icmp_gt(lval, rval);
                break;
            case OP_LE:
                context.value = builder->create_icmp_le(lval, rval);
                break;
            case OP_GE:
                context.value = builder->create_icmp_ge(lval, rval);
                break;
            case OP_EQ:
                context.value = builder->create_icmp_eq(lval, rval);
                break;
            case OP_NEQ:
                context.value = builder->create_icmp_ne(lval, rval);
                break;
        }
    }
    context.value = builder->create_zext(context.value, INT32_T);
    return nullptr;
}

Value* CminusfBuilder::visit(ASTAdditiveExpression &node) {
    if (node.additive_expression == nullptr) {
        node.term->accept(*this);
    } else {
        node.additive_expression->accept(*this);
        auto *lval = context.value;
        node.term->accept(*this);
        auto *rval = context.value;

        if (lval->get_type()->is_float_type() || context.value->get_type()->is_float_type()) { // Has float
            if (lval->get_type()->is_integer_type())
                lval = builder->create_sitofp(lval, FLOAT_T);
            if (rval->get_type()->is_integer_type())
                rval = builder->create_sitofp(rval, FLOAT_T);
            if (node.op == OP_PLUS)
                context.value = builder->create_fadd(lval, rval);
            else if (node.op == OP_MINUS)
                context.value = builder->create_fsub(lval, rval);
        } else {
            if (node.op == OP_PLUS)
                context.value = builder->create_iadd(lval, rval);
            else if (node.op == OP_MINUS)
                context.value = builder->create_isub(lval, rval);
        }
    }
    return nullptr;
}

Value* CminusfBuilder::visit(ASTTerm &node) {
    if (node.term == nullptr) {
        node.factor->accept(*this);
        return nullptr;
    }
    node.term->accept(*this);
    auto *lval = context.value;
    node.factor->accept(*this);
    auto *rval = context.value;

    if (lval->get_type()->is_float_type() || context.value->get_type()->is_float_type()) { // Has float
        if (lval->get_type()->is_integer_type())
            lval = builder->create_sitofp(lval, FLOAT_T);
        if (rval->get_type()->is_integer_type())
            rval = builder->create_sitofp(rval, FLOAT_T);
        if (node.op == OP_MUL)
            context.value = builder->create_fmul(lval, rval);
        else if (node.op == OP_DIV)
            context.value = builder->create_fdiv(lval, rval);
    } else {
        if (node.op == OP_MUL)
            context.value = builder->create_imul(lval, rval);
        else if (node.op == OP_DIV)
            context.value = builder->create_isdiv(lval, rval);
    }
    return nullptr;
}

Value* CminusfBuilder::visit(ASTCall &node) {
    auto *func = (Function *)(scope.find(node.id));
    auto param = func->get_function_type()->param_begin();
    std::vector<Value *> args;
    for (auto &arg: node.args) {
        arg->accept(*this);
        auto *v_type = context.value->get_type();
        if (v_type != *param && !v_type->is_pointer_type()) {
            if (v_type->is_integer_type())
                context.value = builder->create_sitofp(context.value, *param);
            else
                context.value = builder->create_fptosi(context.value, *param);
        }
        param++;
        args.push_back(context.value);
    }
    context.value = builder->create_call(func, args);
    return nullptr;
}

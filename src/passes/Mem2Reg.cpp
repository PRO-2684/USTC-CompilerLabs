#include "Mem2Reg.hpp"
#include "IRBuilder.hpp"
#include "Value.hpp"

#include <memory>

void Mem2Reg::run() {
    // 创建支配树分析 Pass 的实例
    doms_ = std::make_unique<Dominators>(m_);
    // 建立支配树
    doms_->run();
    // 以函数为单元遍历实现 Mem2Reg 算法
    for (auto& f : m_->get_functions()) {
        if (f.is_declaration()) {
            continue;
        }
        func_ = &f;
        if (func_->get_basic_blocks().size() >= 1) {
            generate_phi();                    // 对应伪代码中 phi 指令插入的阶段
            rename(func_->get_entry_block());  // 对应伪代码中重命名阶段
        }
        // 后续 DeadCode 将移除冗余的局部变量的分配空间
    }
}

void Mem2Reg::generate_phi() {
    // FIXME:
    // 步骤一：找到活跃在多个 block 的全局名字集合，以及它们所属的 bb 块
    // 步骤二：从支配树获取支配边界信息，并在对应位置插入 Phi 指令
    std::set<BasicBlock*> pendingBlocks;           // 待处理的基本块
    std::set<BasicBlock*> insertedBlocks;          // 已经插入 Phi 指令的基本块
    BasicBlock* cur = nullptr;                     // 当前正在处理的基本块
    for (auto& bb : func_->get_basic_blocks()) {   // 遍历所有基本块
        for (auto& ins : bb.get_instructions()) {  // 遍历基本块中的所有指令
            Value* lv = nullptr;
            if (ins.is_load()) {  // 如果是 load 指令
                auto load = dynamic_cast<LoadInst*>(&ins);
                lv = load->get_lval();
            } else if (ins.is_store()) {  // 如果是 store 指令
                auto store = dynamic_cast<StoreInst*>(&ins);
                lv = store->get_lval();
            } else if (ins.is_alloca()) {  // 如果是 alloca 指令
                auto alloca = dynamic_cast<AllocaInst*>(&ins);
                if (!alloca->get_alloca_type()->is_array_type()) {
                    varDef[alloca].push_back(&bb); // 记录局部变量的定义
                }
                continue;
            }
            auto vd = varDef[lv];
            if (vd.size() != 0 && vd.back() != &bb) {
                varDef[lv].push_back(&bb); // 记录局部变量的定值
            }
        }
    }
    for (auto& bb : func_->get_basic_blocks()) {   // 遍历所有基本块
        for (auto& ins : bb.get_instructions()) {  // 遍历基本块中的所有指令
            // 定义了局部变量的 bb
            if (!ins.is_store()) {
                continue; // 如果不是 store 指令，跳过
            }
            auto store = dynamic_cast<StoreInst*>(&ins);
            auto lv = store->get_lval();
            auto cv = crossVars[lv];
            if (varDef[lv].size() > 1 && (!cv.size() || cv.back() != &bb)) {
                crossVars[lv].push_back(&bb); // 记录跨基本块的局部变量
            }
        }
    }
    for (auto var : varDef) { // 遍历所有局部变量
        if (var.second.size() <= 1) {
            continue;
        }
        pendingBlocks.clear();
        insertedBlocks.clear();
        auto first = var.first;
        for (auto BB : crossVars[first]) {
            pendingBlocks.insert(BB);
        }
        while (!pendingBlocks.empty()) {
            cur = *pendingBlocks.begin();
            pendingBlocks.erase(pendingBlocks.begin());
            for (auto frontBlock : doms_->get_dominance_frontier(cur)) {
                if (insertedBlocks.find(frontBlock) != insertedBlocks.end()) {
                    continue;
                }
                auto phi = PhiInst::create_phi(first->get_type()->get_pointer_element_type(), frontBlock);
                frontBlock->add_instr_begin(phi);
                insertedBlocks.insert(frontBlock);
                phiToVar[phi] = first;
                bool flag = false;
                for (auto bb : crossVars[first]) {
                    if (bb == frontBlock) {
                        flag = true;
                        break;
                    }
                }
                if (!flag) {
                    pendingBlocks.insert(frontBlock);
                }
            }
        }
    }
}

void Mem2Reg::rename(BasicBlock* bb) {
    // FIXME:
    // 步骤三：将 phi 指令作为 lval 的最新定值，lval 即是为局部变量 alloca 出的地址空间
    // 步骤四：用 lval 最新的定值替代对应的load指令
    // 步骤五：将 store 指令的 rval，也即被存入内存的值，作为 lval 的最新定值
    // 步骤六：为 lval 对应的 phi 指令参数补充完整
    // 步骤七：对 bb 在支配树上的所有后继节点，递归执行 rename 操作
    // 步骤八：pop 出 lval 的最新定值
    // 步骤九：清除冗余的指令
    for (auto& ins : bb->get_instructions()) {
        if (ins.is_phi()) {
            auto var = phiToVar[&ins];
            varStack[var].push_back(&ins);
        } else if (ins.is_load()) {
            auto load = dynamic_cast<LoadInst*>(&ins);
            auto lv = load->get_lval();
            if (varDef[lv].size() && varStack[lv].size()) {
                ins.replace_all_use_with(varStack[lv].back());
            }
        } else if (ins.is_store()) {
            auto store = dynamic_cast<StoreInst*>(&ins);
            auto lv = store->get_lval();
            if (varDef[lv].size()) {
                varStack[lv].push_back(store->get_rval());
            }
        }
    }
    for (auto succ : bb->get_succ_basic_blocks()) {
        for (auto& ins : succ->get_instructions()) {
            if (!ins.is_phi()) {
                continue;
            }
            auto var = phiToVar[&ins];
            auto vs = varStack[var];
            if (vs.size()) {
                auto phi = dynamic_cast<PhiInst*>(&ins);
                phi->add_phi_pair_operand(vs.back(), bb);
            }
        }
    }
    for (auto succ : doms_->get_dom_tree_succ_blocks(bb)) {
        rename(succ);
    }
    std::set<Instruction*> discard;
    for (auto& ins : bb->get_instructions()) {
        if (ins.is_phi()) {
            auto var = phiToVar[&ins];
            varStack[var].pop_back();
        } else if (ins.is_store()) {
            auto store = dynamic_cast<StoreInst*>(&ins);
            auto lv = store->get_lval();
            if (varDef[lv].size()) {
                varStack[lv].pop_back();
                discard.insert(store);
            }
        }
    }
    for (auto ins : discard) {
        bb->get_instructions().erase(ins);
    }
}

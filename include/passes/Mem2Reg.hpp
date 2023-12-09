#pragma once

#include "Dominators.hpp"
#include "Instruction.hpp"
#include "Value.hpp"

#include <map>
#include <memory>
#include <unordered_map>

class Mem2Reg : public Pass {
   private:
    Function* func_;
    std::unique_ptr<Dominators> doms_;

    // FIXME: 添加需要的变量

    std::unordered_map<Instruction*, Value*> phiToVar;               // 记录 phi 指令对应的变量
    std::unordered_map<Value*, std::vector<Value*>> varStack;        // 记录在每个基本块中的值
    std::unordered_map<Value*, std::vector<BasicBlock*>> varDef;     // 记录定义变量的基本块
    std::unordered_map<Value*, std::vector<BasicBlock*>> crossVars;  // 记录跨基本块的值

   public:
    Mem2Reg(Module* m)
        : Pass(m) {}
    ~Mem2Reg() = default;

    void run() override;

    void generate_phi();
    void rename(BasicBlock* bb);

    static inline bool is_global_variable(Value* l_val) {
        return dynamic_cast<GlobalVariable*>(l_val) != nullptr;
    }
    static inline bool is_gep_instr(Value* l_val) {
        return dynamic_cast<GetElementPtrInst*>(l_val) != nullptr;
    }

    static inline bool is_valid_ptr(Value* l_val) {
        return not is_global_variable(l_val) and not is_gep_instr(l_val);
    }
};

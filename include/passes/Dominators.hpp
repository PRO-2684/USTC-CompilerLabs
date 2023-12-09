#pragma once

#include "BasicBlock.hpp"
#include "PassManager.hpp"

#include <map>
#include <set>

class Dominators : public Pass {
   public:
    using BBSet = std::set<BasicBlock*>;

    explicit Dominators(Module* m)
        : Pass(m) {}
    ~Dominators() = default;
    void run() override;

    BasicBlock* get_idom(BasicBlock* bb) { return idom.at(bb); }
    const BBSet& get_dominance_frontier(BasicBlock* bb) {
        return domFront.at(bb);
    }
    const BBSet& get_dom_tree_succ_blocks(BasicBlock* bb) {
        return domSucc.at(bb);
    }

   private:
    void create_idom(Function* f);
    void create_dominance_frontier(Function* f);
    void create_dom_tree_succ(Function* f);

    // FIXME: 补充需要的函数

    bool bb_same(BasicBlock* bb1, BasicBlock* bb2);
    bool bb_dom(BasicBlock* bb1, BasicBlock* bb2);

    std::map<BasicBlock*, BasicBlock*> idom{};  // 直接支配
    std::map<BasicBlock*, BBSet> domFront{};    // 支配边界
    std::map<BasicBlock*, BBSet> domSucc{};     // 支配树中的后继
};

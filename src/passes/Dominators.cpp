#include "Dominators.hpp"

void Dominators::run() {
    for (auto& f_ : m_->get_functions()) {
        auto f = &f_;
        if (!f->get_basic_blocks().size()) {
            continue;
        }
        for (auto& bb1 : f->get_basic_blocks()) {
            auto bb = &bb1;
            domFront.insert({bb, {}});
            domSucc.insert({bb, {}});
            idom.insert({bb, {}});
        }
        create_idom(f);
        create_dominance_frontier(f);
        create_dom_tree_succ(f);
    }
}

void Dominators::create_idom(Function* f) {
    // FIXME: 分析得到 f 中各个基本块的 immediate dominator
    auto entry = f->get_entry_block();
    idom[entry] = entry;
    BasicBlock* newIdom = nullptr;
    std::list<BasicBlock*> queue;
    BBSet processed;
    bool flag = true;  // Is changed
    while (flag) {
        flag = false;
        queue.emplace_back(entry);
        processed.clear();
        while (!queue.empty()) {
            auto cur = queue.front();
            queue.pop_front();
            if (!processed.insert(cur).second) {
                continue;
            }
            for (auto succ : cur->get_succ_basic_blocks()) {
                if (succ != nullptr) {
                    queue.emplace_back(succ);
                }
            }
            if (cur == entry) {
                continue;
            }
            for (auto pred : cur->get_pre_basic_blocks()) {
                if (processed.find(pred) != processed.end()) {
                    newIdom = pred;
                }
            }
            for (auto pred : cur->get_pre_basic_blocks()) {
                if (idom[pred] == nullptr) {
                    continue;
                }
                auto p1 = pred, p2 = newIdom;
                while (p1 != p2) {
                    if (bb_same(p1, p2)) {
                        p1 = get_idom(p1);
                    }
                    while (bb_dom(p1, p2)) {
                        p1 = get_idom(p1);
                    }
                    while (bb_dom(p2, p1)) {
                        p2 = get_idom(p2);
                    }
                }
                newIdom = p1;
            }
            if (idom[cur] != newIdom) {
                flag = true;
                idom[cur] = newIdom;
            }
        }
    }
}

void Dominators::create_dominance_frontier(Function* f) {
    // FIXME: 计算基本块的支配边界
    BasicBlock* cur = nullptr;
    auto end = f->get_basic_blocks().end();
    for (auto iter = f->get_basic_blocks().begin(); iter != end; iter++) {
        auto pbs = (&*iter)->get_pre_basic_blocks();
        if (pbs.size() < 2) {
            continue;
        }
        for (auto pred : pbs) {
            cur = pred;
            while (cur != idom[&*iter]) {
                domFront[cur].insert(&*iter);
                cur = idom[cur];
            }
        }
    }
}

void Dominators::create_dom_tree_succ(Function* f) {
    // FIXME: 计算基本块的后继
    auto end = f->get_basic_blocks().end();
    for (auto iter = f->get_basic_blocks().begin(); iter != end; iter++) {
        if (&*iter != f->get_entry_block()) {
            domSucc[idom[&*iter]].insert(&*iter);
        }
    }
}

bool Dominators::bb_same(BasicBlock* bb1, BasicBlock* bb2) {
    // FIXME: 比较基本块是否相等
    if (bb1 == nullptr || bb2 == nullptr) {
        return bb2 == nullptr;
    }
    auto p1 = bb1, p2 = bb2;
    std::list<BasicBlock*> pb1, pb2;
    while (!(pb1 = p1->get_pre_basic_blocks()).empty() && !(pb2 = p2->get_pre_basic_blocks()).empty()) {
        p1 = pb1.front();
        p2 = pb2.front();
    }
    pb1 = p1->get_pre_basic_blocks();
    pb2 = p2->get_pre_basic_blocks();
    return pb1.empty() && pb2.empty();
}

bool Dominators::bb_dom(BasicBlock* bb1, BasicBlock* bb2) {
    // FIXME: 比较基本块是否支配
    if (bb1 == nullptr || bb2 == nullptr) {
        return !(bb1 == nullptr);
    }
    auto p1 = bb1, p2 = bb2;
    std::list<BasicBlock*> pb1, pb2;
    while (!(pb1 = p1->get_pre_basic_blocks()).empty() && !(pb2 = p2->get_pre_basic_blocks()).empty()) {
        p1 = pb1.front();
        p2 = pb2.front();
    }
    pb1 = p1->get_pre_basic_blocks();
    return !pb1.empty();
}

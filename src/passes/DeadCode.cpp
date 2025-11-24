#include "DeadCode.hpp"
#include "logging.hpp"

void DeadCode::run() {
    bool changed{};
    func_info->run();
    do {
        changed = false;
        for (auto &F : m_->get_functions()) {
            auto func = &F;
            mark(func);
            changed |= sweep(func);
        }
    } while (changed);
    LOG_INFO << "dead code pass erased " << ins_count << " instructions";
}

// TODO
void DeadCode::mark(Function *func) {
    work_list.clear();
    marked.clear();

    for (auto &bb : func->get_basic_blocks()) {
        for (auto &ins : bb.get_instructions()) {
            if (is_critical(&ins)) {
                marked[&ins] = true;
                work_list.push_back(&ins);
            }
        }
    }

    while (work_list.empty() == false) {
        auto now = work_list.front();
        work_list.pop_front();

        mark(now);
    }
}

// TODO
void DeadCode::mark(Instruction *ins) {
    for (auto op : ins->get_operands()) {
        auto def = dynamic_cast<Instruction *>(op);
        if (def == nullptr)
            continue;
        if (marked[def])
            continue;
        if (def->get_function() != ins->get_function())
            continue;
        marked[def] = true;
        work_list.push_back(def);
    }
}

// TODO: 删除无用指令
// 提示：
// 1. 遍历函数的基本块，删除所有标记为true的指令
// 2. 删除指令后，可能会导致其他指令的操作数变为无用，因此需要再次遍历函数的基本块
// 3. 如果删除了指令，返回true，否则返回false
// 4. 注意：删除指令时，需要先删除操作数的引用，然后再删除指令本身
// 5. 删除指令时，需要注意指令的顺序，不能删除正在遍历的指令
bool DeadCode::sweep(Function *func) {
    std::unordered_set<Instruction *> wait_del{};
    for (auto &bb : func->get_basic_blocks()) {
        for (auto it = bb.get_instructions().begin();
             it != bb.get_instructions().end();) {
            if (marked[&*it]) {
                ++it;
                continue;
            } else {
                auto tmp = &*it;
                wait_del.insert(tmp);
                it++;
            }
        }
    }
    for (auto inst : wait_del)
        inst->remove_all_operands();
    for (auto inst : wait_del)
        inst->get_parent()->get_instructions().erase(inst);
    ins_count += wait_del.size();
    return not wait_del.empty(); // changed
}

// TODO: 判断指令是否是无用指令
// 提示：
// 1. 如果是函数调用，且函数是纯函数，则无用
// 2. 如果是无用的分支指令，则无用
// 3. 如果是无用的返回指令，则无用
// 4. 如果是无用的存储指令，则无用
bool DeadCode::is_critical(Instruction *ins) {
    if (ins->is_call()) {
        auto call_inst = dynamic_cast<CallInst *>(ins);
        auto callee = dynamic_cast<Function *>(call_inst->get_operand(0));
        if (func_info->is_pure_function(callee))
            return false;
        return true;
    }
    if (ins->is_br() || ins->is_ret())
        return true;
    if (ins->is_store())
        return true;
    return false;
}

#include <iostream>
#include <bits/stdc++.h>
using namespace std;


enum class Operator {
    INCREMENT,
    DECREMENT, 
    WHILE,
    WHILE_NOT
};

enum class HaltReason {
    NORMAL_TERMINATION,
    UNDERFLOW_CRASH,
    MAX_STEPS_EXCEEDED
};

struct run_result {
    HaltReason reason;
    uint64_t steps;
};

class Statement;

struct Program {
    vector<Statement> statements;
    int max_register_used = -1; // Initialize to -1 to indicate no registers used yet

    size_t size() const;
    void pretty_print(int indent = 0) const;
    int compute_max_register_used() const;
   
};


struct Statement 
{
    Operator op;
    uint8_t register_id;
    Program block; // Only required when it is a WHILE or WHILE_NOT statement

    // Constructor for INCREMENT and DECREMENT statements
    Statement(Operator op, uint8_t register_id) : op(op), register_id(register_id) {} 
    // Constructor for WHILE and WHILE_NOT statements - require block inside while
    Statement(Operator op, uint8_t register_id, Program block) : op(op), register_id(register_id), block(block) {} 

    size_t size() const;
 
};

struct Program_runner {
    uint64_t steps = 0;
    uint64_t max_steps = 1000;
    vector<uint64_t> registers;

    Program_runner(size_t num_registers) : registers(num_registers, 0) {}

    bool execute_statement(const Statement& statement)
    {
        if(steps >= max_steps) {
            return false; // exceeded max steps
        }
        switch (statement.op) {
            case Operator::INCREMENT:
                registers[statement.register_id]++;
                steps++;
                break;
            case Operator::DECREMENT:
                steps++; //yet to decide if this counts as a step or not
                if (registers[statement.register_id] == 0) {
                    return false; // crashed cuz underflow
                }
                registers[statement.register_id]--;
                break;
            case Operator::WHILE:
                while (true) {
                    steps++; // add 1 step for every condition check
                    if (steps >= max_steps) return false;
                    if (registers[statement.register_id] == 0) break; // Exit condition

                    for (const auto& inner_stmt : statement.block.statements) {
                        if (!execute_statement(inner_stmt)) {
                            return false;
                        }
                    }
                }
                break;
            case Operator::WHILE_NOT:
                while (true) {
                    steps++; // add 1 step for every condition check
                    if (steps >= max_steps) return false;
                    if (registers[statement.register_id] != 0) break; // Exit condition

                    for (const auto& inner_stmt : statement.block.statements) {
                        if (!execute_statement(inner_stmt)) {
                            return false;
                        }
                    }
                }

                break;
        }
        return true;
    }

    run_result run(const Program& program, bool verbose = false) {

        fill(registers.begin(), registers.end(), 0); // Reset all vars adn then steps to 0
        steps = 0;
        for (const auto& statement : program.statements) {
            if (!execute_statement(statement)) {
                if (steps >= max_steps) {
                    return {HaltReason::MAX_STEPS_EXCEEDED,steps};
                } else {
                    return {HaltReason::UNDERFLOW_CRASH,steps};
                }
            }
        }
        return {HaltReason::NORMAL_TERMINATION,steps};

    }

    run_result verbose_run(const Program& program) {
        program.pretty_print();
        run_result result = run(program);
        cout << "Program Size: " << program.size() << " Steps executed: " << result.steps << " Reason: " << static_cast<int>(result.reason) << endl;
        cout << "Final Register Values: ";
        for (size_t i = 0; i < registers.size(); ++i) {
            cout << static_cast<char>('A' + i) << ": " << registers[i] << " ";
        }
        cout << endl;
        return result;
    
    }

};

// True if `reg` is incremented or decremented anywhere inside this block,
// including inside nested WHILE/WHILE_NOT bodies.
bool writes_register(const Program& block, uint8_t reg) {
    for (const auto& stmt : block.statements) {
        if ((stmt.op == Operator::INCREMENT || stmt.op == Operator::DECREMENT)
            && stmt.register_id == reg) {
            return true;
        }
        if ((stmt.op == Operator::WHILE || stmt.op == Operator::WHILE_NOT)
            && writes_register(stmt.block, reg)) {
            return true;
        }
    }
    return false;
}

// True if this program contains a WHILE/WHILE_NOT whose condition register
// is never modified inside its own body — a trivial infinite loop.
// Checks for a trivial infinite loop, but skips the trailing WHILE if it's
// still "open" (still on the spine, i.e. still eligible to grow a body later).
bool has_trivial_infinite_loop(const Program& block, bool is_spine_tail = true) {
    for (size_t i = 0; i < block.statements.size(); i++) {
        const Statement& stmt = block.statements[i];
        bool is_last = (i + 1 == block.statements.size());
        if (stmt.op == Operator::WHILE || stmt.op == Operator::WHILE_NOT) {
            bool this_one_is_open = is_spine_tail && is_last;
            if (!this_one_is_open && !writes_register(stmt.block, stmt.register_id)) {
                return true;
            }
            // Recurse into its body, remembering whether *it* could still be open.
            if (has_trivial_infinite_loop(stmt.block, this_one_is_open)) return true;
        }
    }
    return false;
}

inline size_t Program::size() const {
    size_t total_size = 0;
    for (const auto& statement : statements) {
        total_size += statement.size();
    }
    return total_size;
}
inline size_t Statement::size() const {
    size_t total_size = 1; // because while also counts as a statement
    for (const auto& statement : block.statements) {
        total_size += statement.size();
    }
    return total_size;
}
inline void Program::pretty_print(int indent) const {
    for (const auto& statement : statements) {
        string spaces = string(indent * 2, ' '); 
        cout << spaces;
        switch (statement.op) {
            case Operator::INCREMENT:
                cout << static_cast<char>('A' + statement.register_id) << "++" << endl;
                break;
            case Operator::DECREMENT:
                cout << static_cast<char>('A' + statement.register_id) << "--" << endl;
                break;
            case Operator::WHILE:
                cout << "while (" << static_cast<char>('A' + statement.register_id) << " != 0)" << endl;
                cout << spaces << "{" << endl;
                statement.block.pretty_print(indent + 1);
                cout << spaces << "}" << endl;
                break;
            case Operator::WHILE_NOT:
                cout << "while (" << static_cast<char>('A' + statement.register_id) << " == 0)" << endl;
                cout << spaces << "{" << endl;
                statement.block.pretty_print(indent + 1);
                cout << spaces << "}" << endl;
                break;
        }
    }
}
inline int Program::compute_max_register_used() const
{
    int max_reg = -1;
    for (const auto& statement : statements) 
    {
        max_reg = max(max_reg, (int)statement.register_id);
        if (statement.op == Operator::WHILE || statement.op == Operator::WHILE_NOT) {
            max_reg = max(max_reg, statement.block.compute_max_register_used());
        }
    }
    return max_reg;
}

void append_op(vector<Statement>& stmts, Operator op, uint8_t reg) {
    stmts.emplace_back(op, reg);
}

vector<Program> grow(const Program& block, int max_reg_used, int REG_CAP = 3) {
    vector<Program> results;

    int min_reg = 0;
    if (!block.statements.empty()) {
        const Statement& last = block.statements.back();
        if (last.op == Operator::INCREMENT || last.op == Operator::DECREMENT)
            min_reg = last.register_id;
    }
    int new_reg_cap = min(max_reg_used + 1, REG_CAP - 1);

    for (int reg = min_reg; reg <= new_reg_cap; reg++) {
        bool is_new_reg = (reg == max_reg_used + 1);
        int new_max = is_new_reg ? reg : max_reg_used;

        Program with_inc = block;
        append_op(with_inc.statements, Operator::INCREMENT, reg);
        with_inc.max_register_used = new_max;
        results.push_back(with_inc);

        Program with_dec = block;
        append_op(with_dec.statements, Operator::DECREMENT, reg);
        with_dec.max_register_used = new_max;
        results.push_back(with_dec);
    }

    // WHILE or WHILE_NOT: free choice of register in [0, max_reg_used + 1].
    for (int reg = 0; reg <= new_reg_cap; reg++) {
        bool is_new_reg = (reg == max_reg_used + 1);
        int new_max = is_new_reg ? reg : max_reg_used;

        Program with_while = block;
        with_while.statements.emplace_back(Operator::WHILE, (uint8_t)reg, Program{});
        with_while.max_register_used = new_max;
        results.push_back(with_while);

        Program with_while_not = block;
        with_while_not.statements.emplace_back(Operator::WHILE_NOT, (uint8_t)reg, Program{});
        with_while_not.max_register_used = new_max;
        results.push_back(with_while_not);
    }

    // If the last statement is a WHILE/WHILE_NOT, recursively grow its body.
    if (!block.statements.empty()) {
        const Statement& last = block.statements.back();
        if (last.op == Operator::WHILE || last.op == Operator::WHILE_NOT) {
            for (Program& grown_body : grow(last.block, max_reg_used)) {
                Program with_grown_body = block;
                with_grown_body.statements.back().block = grown_body;
                with_grown_body.max_register_used = grown_body.max_register_used;
                results.push_back(with_grown_body);
            }
        }
    }

    return results;
}

vector<Program> next_generation(const vector<Program>& gen_n, int REG_CAP = 3) {
    vector<Program> next;
    for (const auto& p : gen_n) {
        vector<Program> children = grow(p, p.max_register_used, REG_CAP);
        for(const auto& child : children) {
            if (!has_trivial_infinite_loop(child)) {
                next.push_back(child);
            }
        }
    }
    return next;
}




    
    





int main() 
{
    
    Program power2 = {
        {
            Statement(Operator::INCREMENT, 0),
            Statement(Operator::INCREMENT, 0),
            Statement(Operator::INCREMENT, 0),
            Statement(Operator::INCREMENT, 0),
            Statement(Operator::INCREMENT, 0),
            Statement(Operator::INCREMENT, 1), 
            Statement(Operator::WHILE, 0, Program
            {
                {
                Statement(Operator::DECREMENT, 0),
                Statement(Operator::WHILE, 1, Program
                {
                    {
                    Statement(Operator::DECREMENT, 1),
                    Statement(Operator::INCREMENT, 2)
                    }
                }),
                Statement(Operator::WHILE, 2, Program
                {
                    {
                    Statement(Operator::DECREMENT, 2),
                    Statement(Operator::INCREMENT, 1),
                    Statement(Operator::INCREMENT, 1)
                    }
                })
                }
            })
        }
    };

    // cout << power2.compute_max_register_used() << endl; // should print 2
    Program_runner runner(3);
    // runner.verbose_run(power2);

    int max_size = 7;
    vector<Program> gen = { Program{} };  // size 0
    for (int size = 1; size <= max_size; size++) {
        gen = next_generation(gen);
        cout << "size " << size << ": " << gen.size() << " programs\n";
        // int max_steps = 0;
        // Program max_program ;
        // for (const auto& p : gen) {
        //     run_result result = runner.run(p);
        //     if (result.reason != HaltReason::MAX_STEPS_EXCEEDED && result.steps > max_steps) {
        //         max_steps = result.steps;
        //         max_program = p;
        //     }
        // }
        // runner.verbose_run(max_program);

    }
    int target = 10;
    int curr = 0;
    vector<Program> filtered_gen;
    for(const auto& p : gen){
        if(!has_trivial_infinite_loop(p, false)){
            filtered_gen.push_back(p);
        }
    }


    cout << "Filtered programs: " << filtered_gen.size() << endl;
    Program BestProgram;
    int currSteps = 0;
    for (size_t i = 0; i < filtered_gen.size(); ++i) 
    {
        if (curr >= target) break;
        run_result result = runner.run(filtered_gen[i]);
        if (result.steps > currSteps && result.reason != HaltReason::MAX_STEPS_EXCEEDED) {
            currSteps = result.steps;
            BestProgram = filtered_gen[i];
        }
    }


    cout << "Best program: " << endl;
    runner.verbose_run(BestProgram);
    cout << endl;

    
}


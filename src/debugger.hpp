#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include <span>
#include <vector>
#include <memory>

class InstrFormat;
class Registers;
class CPU;

class Debugger {
    public:
        Debugger(std::shared_ptr<CPU> cpu) : m_cpu(cpu) {};

        struct Instr {
            std::uint32_t addr;
            std::uint32_t opcode;
            std::string desc;
        };

        Registers& view_registers();
        std::array<Instr, 64> view_nearby_instructions();

    private:
        void decompile_arm_instr(Instr& instr);
        void decompile_thumb_instr(Instr& instr);

        std::shared_ptr<CPU> m_cpu;
};

#endif

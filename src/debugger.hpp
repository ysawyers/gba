#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include <span>
#include <vector>
#include <memory>

class Registers;
class CPU;

class Debugger {
    public:
        Debugger() : m_decompiled_thumb(false) {};

        struct Instr {
            Instr() : addr(0), instr(0) {};

            std::uint32_t addr;
            std::uint32_t instr;
            std::string desc;
        };

        Registers& view_registers(const std::shared_ptr<CPU> cpu);
        std::span<Instr> view_instructions(const std::shared_ptr<CPU> cpu);

    private:
        void translate_arm(Instr& instr);
        void translate_thumb(Instr& instr);

        std::vector<Instr> m_decompiled_instr_cache;
        bool m_decompiled_thumb;
};

#endif

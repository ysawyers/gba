#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include <string>

class Registers;
class CPU;

class Debugger {
    public:
        Debugger(CPU& cpu) : m_cpu(cpu) {};

        struct Instr {
            Instr() : addr(0), instr(0) {};

            std::uint32_t addr;
            std::uint32_t instr;
            std::string desc;
        };

        Registers& view_registers();
        bool thumb_enabled();

        friend class CPU;

    private:
        void translate_arm(Instr& instr);
        void translate_thumb(Instr& instr);

        CPU& m_cpu;
};

#endif

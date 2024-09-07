#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include <span>
#include <vector>
#include <memory>

#include "core/cpu.hpp"

class Debugger {
    public:
        Debugger(std::shared_ptr<CPU> cpu) : m_cpu(cpu) {};

        struct Instr {
            Instr() : desc("????????") {};

            std::uint32_t addr;
            std::uint32_t opcode;
            std::string desc;
        };

        CPU::Registers& view_registers();
        std::uint32_t view_cpsr();
        std::uint32_t view_psr();
        std::uint16_t view_if();
        std::uint16_t view_ie();
        std::uint32_t view_pipeline();
        bool is_pipeline_invalid();

        std::uint32_t current_pc();
        std::array<Instr, 64> view_nearby_instructions();

    private:
        void decompile_arm_instr(Instr& instr);
        void decompile_thumb_instr(Instr& instr);

        void print_branch(Instr& instr);
        void print_branch_x(Instr& instr);
        void print_alu(Instr& instr);
        void print_single_transfer(Instr& instr);
        void print_halfword_transfer(Instr& instr);
        void print_block_transfer(Instr& instr);
        void print_mul(Instr& instr);

        const char* swi_subroutine(std::uint32_t nn);
        const char* shift_type(std::uint8_t opcode);
        const char* condition(std::uint32_t instr);
        const char* amod(std::uint8_t pu);

        std::shared_ptr<CPU> m_cpu;
};

#endif

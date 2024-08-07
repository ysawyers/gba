#ifndef CPU_HPP
#define CPU_HPP

#include <string>

#include "memory.hpp"

struct Registers {
    std::uint32_t r0 = 0;
    std::uint32_t r1 = 0;
    std::uint32_t r2 = 0;
    std::uint32_t r3 = 0;
    std::uint32_t r4 = 0;
    std::uint32_t r5 = 0;
    std::uint32_t r6 = 0;
    std::uint32_t r7 = 0;
    std::uint32_t r8 = 0;
    std::uint32_t r9 = 0;
    std::uint32_t r10 = 0;
    std::uint32_t r11 = 0;
    std::uint32_t r12 = 0;
    std::uint32_t r13 = 0; // SP
    std::uint32_t r14 = 0; // LR
    std::uint32_t r15 = 0; // PC
    std::uint32_t cpsr = 0;
    std::uint32_t spsr = 0;
};

class CPU {
    public:
        CPU(const std::string&& rom_filepath);

        void render_frame() noexcept;

        // used as indices for banked registers
        enum class Mode {
            SYS = 0, FIQ, SVC, ABT, IRQ, UND
        };

        enum class InstrFormat {
            NOP, B, BX, SWP, MRS, SWI, MUL, MSR, ALU, 
            SINGLE_TRANSFER, HALFWORD_TRANSFER, BLOCK_TRANSFER,   
        };

    private:
        std::uint32_t fetch();
        InstrFormat decode(const std::uint32_t instr);
        std::size_t execute();

        std::size_t branch(const std::uint32_t instr);
        std::size_t branch_ex(const std::uint32_t instr);
        std::size_t single_transfer(const std::uint32_t instr);
        std::size_t halfword_transfer(const std::uint32_t instr);
        std::size_t block_transfer(const std::uint32_t instr);
        std::size_t msr(const std::uint32_t instr);
        std::size_t mrs(const std::uint32_t instr);
        std::size_t swi(const std::uint32_t instr);
        std::size_t swp(const std::uint32_t instr);
        std::size_t alu(const std::uint32_t instr);
        std::size_t mul(const std::uint32_t instr);

        bool thumb_enabled();
        void change_cpsr_mode(Mode mode);

        bool m_pipeline_invalid;
        std::uint32_t m_pipeline;
        std::array<Registers, 6> m_banked_regs{};
        Registers& m_regs = m_banked_regs[0];
        Memory m_mem;
};

#endif

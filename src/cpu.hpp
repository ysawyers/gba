#ifndef CPU_HPP
#define CPU_HPP

#include <string>

#include "memory.hpp"

typedef std::array<std::uint32_t, 17> Registers;

struct Flags {
    bool n = false;
    bool z = false;
    bool c = false;
    bool v = false;
};

class CPU {
    public:
        CPU(const std::string&& rom_filepath);

        std::array<std::array<std::uint16_t, 240>, 160>& render_frame();

        // used as indices for banked registers
        enum class Mode {
            SYS = 0, FIQ, SVC, ABT, IRQ, UND
        };

        enum class InstrFormat {
            NOP, B, BX, SWP, MRS, SWI, MUL, MSR, ALU, 
            SINGLE_TRANSFER, HALFWORD_TRANSFER, BLOCK_TRANSFER,   
        };

        enum class ShiftType {
            LSL = 0, LSR, ASR, ROR 
        };

    private:
        std::uint32_t fetch();
        InstrFormat decode(std::uint32_t instr);
        int execute();

        bool barrel_shifter(
            std::uint32_t& operand,
            bool& carry_out,
            ShiftType shift_type,
            std::uint8_t shift_amount, 
            bool reg_imm_shift
        );

        int branch(std::uint32_t instr);
        int branch_ex(std::uint32_t instr);
        int single_transfer(std::uint32_t instr);
        int halfword_transfer(std::uint32_t instr);
        int block_transfer(std::uint32_t instr);
        int msr(std::uint32_t instr);
        int mrs(std::uint32_t instr);
        int swi(std::uint32_t instr);
        int swp(std::uint32_t instr);
        int alu(std::uint32_t instr);
        int mul(std::uint32_t instr);

        bool thumb_enabled();
        bool condition(std::uint32_t instr);
        void change_cpsr_mode(Mode mode);
        std::uint32_t ror(std::uint32_t operand, std::size_t shift_amount);

        std::uint32_t m_pipeline;
        bool m_pipeline_invalid;
        std::array<Registers, 6> m_banked_regs{};
        Registers& m_regs = m_banked_regs[0];
        Flags m_flags;
        Memory m_mem;
};

#endif

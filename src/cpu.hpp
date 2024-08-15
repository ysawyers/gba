#ifndef CPU_HPP
#define CPU_HPP

#ifdef _DEBUG
#define CPU_LOG(x) printf(x)
#else
#define CPU_LOG(x)
#endif

#include <string>

#include "memory.hpp"

// used as indices for banked registers in CPU
enum Mode {
    SYS = 0, FIQ, SVC, ABT, IRQ, UND, UNSET
};

struct Flags {
    Flags() : n(0), z(0), c(0), v(0) {};

    Flags(Flags& flags) {
        n = 1;
        z = 1;
        c = 1;
        v = 1;
    }

    std::uint8_t n, z, c, v;
};

class Registers {
    public:
        Registers() : mode(UNSET) {};

        Registers(Registers& regs) {
            flags = regs.flags;
            mode = regs.mode;
            std::copy(regs.m_list.begin(), regs.m_list.end(), m_list.begin());
        }

        std::uint32_t& operator[](std::uint8_t reg) {
            return m_list[reg];
        }

        Flags flags;
        std::uint8_t mode;

    private:
        std::array<std::uint32_t, 16> m_list;
};

class CPU {
    public:
        CPU(const std::string&& rom_filepath);

        std::array<std::array<std::uint16_t, 240>, 160>& render_frame();

        enum class InstrFormat {
            NOP, B, BX, SWP, MRS, SWI, MUL, MSR, ALU, 
            SINGLE_TRANSFER, HALFWORD_TRANSFER, BLOCK_TRANSFER,
            THUMB_1, THUMB_3, THUMB_5, THUMB_6, THUMB_12,  
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

        std::uint32_t thumb_translate_3(std::uint16_t instr);
        //! assumes that thumb_opcode != 3
        std::uint32_t thumb_translate_5(std::uint16_t instr, std::uint32_t rs, std::uint32_t thumb_opcode);

        // handles r15 edge case when assigned which should force a pipeline flush
        void safe_reg_assign(std::uint8_t reg, std::uint32_t value);

        bool condition(std::uint32_t instr);
        std::uint32_t ror(std::uint32_t operand, std::size_t shift_amount);
        Registers& get_bank(std::uint8_t mode);
        std::uint32_t get_psr();
        std::uint32_t get_cpsr();
        void dump_state();

        std::uint32_t m_pipeline;
        bool m_pipeline_invalid;
        bool m_thumb_enabled;
        std::array<Registers, 6> m_banked_regs{};
        Registers m_regs = m_banked_regs[SYS];
        Memory m_mem;
};

#endif

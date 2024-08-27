#ifndef CPU_HPP
#define CPU_HPP

#include <string>
#include <unordered_map>
#include <map>

#include "../debugger.hpp"

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
        Registers() : control(0), m_list({}) {};

        Registers(Registers& regs) {
            flags = regs.flags;
            control = regs.control;
            std::copy(regs.m_list.begin(), regs.m_list.end(), m_list.begin());
        }

        std::uint32_t& operator[](std::uint8_t reg) {
            return m_list[reg];
        }

        Flags flags;
        std::uint8_t control;

    private:
        std::array<std::uint32_t, 16> m_list{};
};

class CPU {
    public:
        CPU(const std::string& rom_filepath);

        FrameBuffer& render_frame(std::uint16_t key_input, std::uint32_t breakpoint, bool& breakpoint_reached);
        FrameBuffer& view_current_frame();
        void step();

        void reset();

        friend class Debugger;

    private:
        enum class ShiftType {
            LSL = 0, LSR, ASR, ROR 
        };

        enum class InstrFormat : std::uint8_t {
            NOP = 0, B, BX, SWP, MRS, SWI, MUL, MSR, ALU, 
            SINGLE_TRANSFER, HALFWORD_TRANSFER, BLOCK_TRANSFER,
            THUMB_1, THUMB_2, THUMB_3, THUMB_4, THUMB_5_ALU, THUMB_5_BX,
            THUMB_6, THUMB_7, THUMB_8, THUMB_9, THUMB_10, THUMB_11, THUMB_12,
            THUMB_13, THUMB_14, THUMB_15, THUMB_16, THUMB_17, THUMB_18,
            THUMB_19_PREFIX, THUMB_19_SUFFIX
        };

        bool condition(std::uint32_t instr);
        std::uint32_t fetch_arm();
        std::uint16_t fetch_thumb();
        int execute();

        bool barrel_shifter(
            std::uint32_t& op,
            bool& carry_out,
            ShiftType shift_type,
            std::uint8_t shift_amount, 
            bool reg_imm_shift
        );

        void get_alu_operands(
            std::uint32_t instr, 
            std::uint32_t& op1, 
            std::uint32_t& op2, 
            bool& carry_out
        );

        Registers& sys_bank();
        std::uint32_t get_cpsr();
        std::uint32_t get_psr();

        void update_cpsr_mode(std::uint8_t mode_bits);
        void update_cpsr_thumb_status(bool enabled);

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

        std::uint32_t thumb_translate_1(std::uint16_t instr);
        std::uint32_t thumb_translate_2(std::uint16_t instr);
        std::uint32_t thumb_translate_3(std::uint16_t instr);
        std::uint32_t thumb_translate_5_alu(std::uint16_t instr);
        std::uint32_t thumb_translate_5_bx(std::uint16_t instr);
        std::uint32_t thumb_translate_7(std::uint16_t instr);
        std::uint32_t thumb_translate_8(std::uint16_t instr);
        std::uint32_t thumb_translate_9(std::uint16_t instr);
        std::uint32_t thumb_translate_10(std::uint16_t instr);
        std::uint32_t thumb_translate_11(std::uint16_t instr);
        std::uint32_t thumb_translate_13(std::uint16_t instr);
        std::uint32_t thumb_translate_14(std::uint16_t instr);
        std::uint32_t thumb_translate_15(std::uint16_t instr);
        std::uint32_t thumb_translate_16(std::uint16_t instr);
        std::uint32_t thumb_translate_17(std::uint16_t instr);
        std::uint32_t thumb_translate_18(std::uint16_t instr);

        void initialize_registers();
        std::uint32_t ror(std::uint32_t operand, std::size_t shift_amount);
        Registers& get_bank(std::uint8_t mode);
        
        void safe_reg_assign(std::uint8_t reg, std::uint32_t value);

        std::array<InstrFormat, 4096> m_arm_lut = ([]() constexpr -> auto {
            std::array<InstrFormat, 4096> lut{};
            {
                std::uint16_t postfix = 0b1111 << 8;
                for (std::uint16_t mask = 0; mask <= 0b11111111; mask++) {
                    lut[postfix | mask] = InstrFormat::SWI;
                }
            }
            {
                std::uint16_t postfix = 0b101 << 9;
                for (std::uint16_t mask = 0; mask <= 0b111111111; mask++) {
                    lut[postfix | mask] = InstrFormat::B;
                }
            }
            {
                std::uint16_t postfix = 0b100 << 9;
                for (std::uint16_t mask = 0; mask <= 0b111111111; mask++) {
                    lut[postfix | mask] = InstrFormat::BLOCK_TRANSFER;
                }
            }
            {
                std::uint16_t postfix = 0b010 << 9;
                for (std::uint16_t mask = 0; mask <= 0b111111111; mask++) {
                    lut[postfix | mask] = InstrFormat::SINGLE_TRANSFER;
                }
                postfix = 0b011 << 9;
                for (std::uint16_t mask = 0; mask <= 0b11111111; mask++) {
                    lut[postfix | (mask << 1)] = InstrFormat::SINGLE_TRANSFER;
                }
            }
            {
                std::uint16_t postfix = 0b001 << 9;
                for (std::uint16_t mask = 0; mask <= 0b111111111; mask++) {
                    lut[postfix | mask] = InstrFormat::ALU;
                }
                for (std::uint16_t mask = 0; mask <= 0b11111111; mask++) {
                    lut[mask << 1] = InstrFormat::ALU;
                }
                for (std::uint16_t prefix = 0b0001; prefix <= 0b0111; prefix++) {
                    for (std::uint16_t mask = 0; mask <= 0b11111; mask++) {
                        lut[(mask << 4) | prefix] = InstrFormat::ALU;
                    }
                }
            }
            lut[0b000100100001] = InstrFormat::BX;
            {
                lut[0b000100000000] = InstrFormat::MRS;
                lut[0b000101000000] = InstrFormat::MRS;
                lut[0b000100100000] = InstrFormat::MSR;
                lut[0b000101100000] = InstrFormat::MSR;
                std::uint16_t postfix = 0b00110010 << 4;
                for (std::uint16_t mask = 0; mask <= 0b1111; mask++) {
                    lut[postfix | mask] = InstrFormat::MSR;
                }
                postfix = 0b00110110 << 4;
                for (std::uint16_t mask = 0; mask <= 0b1111; mask++) {
                    lut[postfix | mask] = InstrFormat::MSR;
                }
            }
            {
                std::uint16_t prefix = 0b1011;
                for (std::uint16_t mask = 0; mask <= 0b11111; mask++) {
                    lut[(mask << 4) | prefix] = InstrFormat::HALFWORD_TRANSFER;
                }
                prefix = 0b11101;
                for (std::uint16_t mask = 0; mask <= 0b1111; mask++) {
                    lut[(mask << 5) | prefix] = InstrFormat::HALFWORD_TRANSFER;
                }
                prefix = 0b11111;
                for (std::uint16_t mask = 0; mask <= 0b1111; mask++) {
                    lut[(mask << 5) | prefix] = InstrFormat::HALFWORD_TRANSFER;
                }
            }
            lut[0b000100001001] = InstrFormat::SWP;
            lut[0b000101001001] = InstrFormat::SWP;
            {
                std::uint16_t prefix = 0b1001;
                for (std::uint16_t mask = 0; mask <= 0b11; mask++) {
                    lut[(mask << 4) | prefix] = InstrFormat::MUL;
                }
                for (std::uint16_t mask = 0b00001000; mask <= 0b00001111; mask++) {
                    lut[(mask << 4) | prefix] = InstrFormat::MUL;
                }
            }
            return lut;
        })();

        std::array<InstrFormat, 1024> m_thumb_lut = ([]() constexpr -> auto {
            std::array<InstrFormat, 1024> lut{};
            {
                std::uint16_t suffix = 0b11110 << 5;
                for (std::uint16_t mask = 0; mask <= 0b11111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_19_PREFIX;
                }
                suffix = 0b11111 << 5;
                for (std::uint16_t mask = 0; mask <= 0b11111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_19_SUFFIX;
                }
            }
            {
                std::uint16_t suffix = 0b11100 << 5;
                for (std::uint16_t mask = 0; mask <= 0b11111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_18;
                }
            }
            {
                std::uint16_t suffix = 0b1101 << 6;
                for (std::uint16_t mask = 0; mask <= 0b111111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_16;
                }
            }
            {
                std::uint16_t suffix = 0b11011111 << 2;
                for (std::uint16_t mask = 0; mask <= 0b11; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_17;
                }
            }
            {
                std::uint16_t suffix = 0b1100 << 6;
                for (std::uint16_t mask = 0; mask <= 0b111111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_15;
                }
            }
            {
                std::uint16_t suffix = 0b1011010 << 3;
                for (std::uint16_t mask = 0; mask <= 0b111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_14;
                }
                suffix = 0b1011110 << 3;
                for (std::uint16_t mask = 0; mask <= 0b111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_14;
                }
            }
            {
                std::uint16_t suffix = 0b10110000 << 2;
                for (std::uint16_t mask = 0; mask <= 0b11; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_13;
                }
            }
            {
                std::uint16_t suffix = 0b1010 << 6;
                for (std::uint16_t mask = 0; mask <= 0b111111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_12;
                }
            }
            {
                std::uint16_t suffix = 0b1001 << 6;
                for (std::uint16_t mask = 0; mask <= 0b111111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_11;
                }
            }
            {
                std::uint16_t suffix = 0b1000 << 6;
                for (std::uint16_t mask = 0; mask <= 0b111111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_10;
                }
            }
            {
                std::uint16_t suffix = 0b0110 << 6;
                for (std::uint16_t mask = 0; mask <= 0b111111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_9;
                }
                suffix = 0b0111 << 6;
                for (std::uint16_t mask = 0; mask <= 0b111111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_9;
                }
            }
            {
                std::uint16_t suffix = 0b0101000 << 3;
                for (std::uint16_t mask = 0; mask <= 0b111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_7;
                }
                suffix = 0b0101100 << 3;
                for (std::uint16_t mask = 0; mask <= 0b111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_7;
                }
                suffix = 0b0101010 << 3;
                for (std::uint16_t mask = 0; mask <= 0b111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_7;
                }
                suffix = 0b0101110 << 3;
                for (std::uint16_t mask = 0; mask <= 0b111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_7;
                }
            }
            {
                std::uint16_t suffix = 0b0101011 << 3;
                for (std::uint16_t mask = 0; mask <= 0b111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_8;
                }
                suffix = 0b0101111 << 3;
                for (std::uint16_t mask = 0; mask <= 0b111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_8;
                }
                suffix = 0b0101001 << 3;
                for (std::uint16_t mask = 0; mask <= 0b111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_8;
                }
                suffix = 0b0101101 << 3;
                for (std::uint16_t mask = 0; mask <= 0b111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_8;
                }
            }
            {
                std::uint16_t suffix = 0b01001 << 5;
                for (std::uint16_t mask = 0; mask <= 0b11111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_6;
                }
            }
            {
                std::uint16_t suffix = 0b010001 << 4;
                for (std::uint16_t mask = 0; mask <= 0b1111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_5_ALU;
                }
            }
            {
                std::uint16_t suffix = 0b01000111 << 2;
                for (std::uint16_t mask = 0; mask <= 0b11; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_5_BX;
                }
            }
            {
                std::uint16_t suffix = 0b010000 << 4;
                for (std::uint16_t mask = 0; mask <= 0b1111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_4;
                }
            }
            {
                std::uint16_t suffix = 0b001 << 7;
                for (std::uint16_t mask = 0; mask <= 0b1111111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_3;
                }
            }
            {
                for (std::uint16_t mask = 0; mask <= 0b1111111; mask++) {
                    lut[mask] = InstrFormat::THUMB_1;
                }
            }
            {
                std::uint16_t suffix = 0b00011 << 5;
                for (std::uint16_t mask = 0; mask <= 0b11111; mask++) {
                    lut[suffix | mask] = InstrFormat::THUMB_2;
                }
            }
            return lut;
        })();

        std::uint32_t m_pipeline;
        bool m_pipeline_invalid;
        bool m_thumb_enabled;
        std::array<Registers, 6> m_banked_regs{};
        Registers m_regs = m_banked_regs[SYS];
        Memory m_mem;
};

#endif

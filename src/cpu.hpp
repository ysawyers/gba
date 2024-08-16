#ifndef CPU_HPP
#define CPU_HPP

#include <string>
#include <unordered_map>
#include <map>

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

        enum class InstrFormat : std::uint8_t {
            NOP = 0, B, BX, SWP, MRS, SWI, MUL, MSR, ALU, 
            SINGLE_TRANSFER, HALFWORD_TRANSFER, BLOCK_TRANSFER,
            THUMB_1, THUMB_3, THUMB_5, THUMB_6, THUMB_12,  
        };

        enum class ShiftType {
            LSL = 0, LSR, ASR, ROR 
        };

    private:
        std::uint32_t fetch_arm();
        std::uint16_t fetch_thumb();

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

        bool condition(std::uint32_t instr);
        std::uint32_t get_psr();
        std::uint32_t get_cpsr();

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
        std::uint32_t thumb_translate_5(std::uint16_t instr, std::uint32_t rs, std::uint32_t thumb_opcode);

        std::uint32_t ror(std::uint32_t operand, std::size_t shift_amount);
        Registers& get_bank(std::uint8_t mode);
        void change_bank(std::uint8_t new_mode);
        void safe_reg_assign(std::uint8_t reg, std::uint32_t value);
        void dump_state();

        std::array<InstrFormat, 4096> m_arm_lut = ([]() constexpr -> auto {
            std::array<InstrFormat, 4096> lut;

            lut[0b000100100001] = InstrFormat::BX;
            lut[0b000100001001] = InstrFormat::SWP;
            lut[0b000101001001] = InstrFormat::SWP;

            // MSR, MRS
            {
                lut[0b000100000000] = InstrFormat::MRS;
                lut[0b000101000000] = InstrFormat::MRS;
                std::uint16_t postfix = 0b00110010;
                for (std::uint16_t mask = 0; mask <= 0b1111; mask++) {
                    lut[(postfix << 4) | mask] = InstrFormat::MSR;
                }
                postfix = 0b00110110;
                for (std::uint16_t mask = 0; mask <= 0b1111; mask++) {
                    lut[(postfix << 4) | mask] = InstrFormat::MSR;
                }
            }

            // MUL, MLA, MULL, MLAL
            {
                std::uint16_t prefix = 0b1001;
                for (std::uint16_t mask = 0; mask <= 0b11; mask++) {
                    lut[(mask << 4) | prefix] = InstrFormat::MUL;
                }
                for (std::uint16_t mask = 0b00001000; mask <= 0b00001111; mask++) {
                    lut[(mask << 4) | prefix] = InstrFormat::MUL;
                }
            }

            // LDRH, LDRSB, LDRSH, STRH
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

            // B, BL
            {
                std::uint16_t postfix = 0b101 << 9;
                for (std::uint16_t mask = 0; mask <= 0b111111111; mask++) {
                    lut[postfix | mask] = InstrFormat::B;
                }
            }

            // Data Processing (ALU)
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

            // LDR, STR
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

            // LDM, STM
            {
                std::uint16_t postfix = 0b100 << 9;
                for (std::uint16_t mask = 0; mask <= 0b111111111; mask++) {
                    lut[postfix | mask] = InstrFormat::BLOCK_TRANSFER;
                }
            }

            // SWI
            {
                std::uint16_t postfix = 0b1111 << 8;
                for (std::uint16_t mask = 0; mask <= 0b11111111; mask++) {
                    lut[postfix | mask] = InstrFormat::SWI;
                }
            }

            return lut;
        })();

        std::array<InstrFormat, 1024> thumb_lut = ([]() constexpr -> auto {
            std::array<InstrFormat, 1024> lut;
            // TODO
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

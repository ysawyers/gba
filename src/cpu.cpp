#include "cpu.hpp"

#include <iostream>
#include <utility>

constexpr int halfword_access = 2;
constexpr int word_access = 4;
constexpr int cycles_per_frame = 280896;

bool CPU::thumb_enabled() {
    return (m_regs.cpsr >> 5) & 1;
}

void CPU::change_cpsr_mode(Mode mode) {
    m_regs = m_banked_regs[std::to_underlying(mode)];
    bool n = (m_regs.cpsr >> 31) & 1;
    bool z = (m_regs.cpsr >> 30) & 1;
    bool c = (m_regs.cpsr >> 29) & 1;
    bool v = (m_regs.cpsr >> 28) & 1;
    m_flags = Flags{n, z, c, v};
}

bool CPU::condition(std::uint32_t instr) {
    switch ((instr >> 28) & 0xF) {
    case 0x0: return m_flags.z;
    case 0x1: return !m_flags.z;
    case 0x2: return m_flags.c;
    case 0x3: return !m_flags.c;
    case 0x4: return m_flags.n;
    case 0x5: return !m_flags.n;
    case 0x6: return m_flags.v;
    case 0x7: return !m_flags.v;
    case 0x8: return m_flags.c & !m_flags.z;
    case 0x9: return !m_flags.c | m_flags.z;
    case 0xA: return m_flags.n == m_flags.v;
    case 0xB: return m_flags.n ^ m_flags.v;
    case 0xC: return !m_flags.z && (m_flags.n == m_flags.v);
    case 0xD: return m_flags.z || (m_flags.n ^ m_flags.v);
    case 0xE: return true;
    }
    std::unreachable();
}

std::uint32_t CPU::ror(std::uint32_t operand, std::size_t shift_amount) {
    return (operand >> (shift_amount & 31)) | (operand << ((-shift_amount) & 31));
}

CPU::CPU(const std::string&& rom_filepath) : m_pipeline(0), m_pipeline_invalid(true) {
    m_mem.load_rom(std::move(rom_filepath));

    // initializes registers while bios is unimplemented
    m_banked_regs[std::to_underlying(Mode::SVC)].r13 = 0x03007FE0;
    m_banked_regs[std::to_underlying(Mode::IRQ)].r13 = 0x03007FA0;
    m_regs.r13 = 0x03007F00;
    m_regs.r14 = 0x08000000;
    m_regs.r15 = 0x08000000;

    m_banked_regs[std::to_underlying(Mode::SYS)].cpsr = 0b10000;
    m_banked_regs[std::to_underlying(Mode::FIQ)].cpsr = 0b10001;
    m_banked_regs[std::to_underlying(Mode::IRQ)].cpsr = 0b10010;
    m_banked_regs[std::to_underlying(Mode::SVC)].cpsr = 0b10011;
    m_banked_regs[std::to_underlying(Mode::ABT)].cpsr = 0b10111;
    m_banked_regs[std::to_underlying(Mode::UND)].cpsr = 0b11011;
}

std::uint32_t CPU::fetch() {
    std::uint32_t instr = 0;
    if (thumb_enabled()) {
        instr = m_mem.read_halfword(m_regs.r15);
        m_regs.r15 += halfword_access;
    } else {
        instr = m_mem.read_word(m_regs.r15);
        m_regs.r15 += word_access;
    }
    return instr;
}

CPU::InstrFormat CPU::decode(std::uint32_t instr) {
    m_pipeline = fetch();

    if (thumb_enabled()) {
        std::cout << "need to decode thumb" << std::endl;
        std::exit(1);

        // switch ((instr >> 13) & 0x7) {
        // case 0x0:
        //     if (((instr >> 11) & 0x3) == 0x3) 
        //         return thumb_decompress_2(instr, &curr_instr);
        //     return thumb_decompress_1(instr, &curr_instr);
        // case 0x1: return thumb_decompress_3(instr, &curr_instr);
        // case 0x2:
        //     switch ((instr >> 10) & 0x7) {
        //     case 0x0: return thumb_decompress_4(instr, &curr_instr);
        //     case 0x1: return thumb_decompress_5(instr, &curr_instr);
        //     case 0x2:
        //     case 0x3: return THUMB_LOAD_PC_RELATIVE;
        //     }
        //     if ((instr >> 9) & 1)
        //         return thumb_decompress_8(instr, &curr_instr);
        //     return thumb_decompress_7(instr, &curr_instr);
        // case 0x3: return thumb_decompress_9(instr, &curr_instr);
        // case 0x4:
        //     if ((instr >> 12) & 1)
        //         return thumb_decompress_11(instr, &curr_instr);
        //     return thumb_decompress_10(instr, &curr_instr);
        // case 0x5:
        //     if (((instr >> 12) & 1) == 0) 
        //         return THUMB_RELATIVE_ADDRESS;
        //     if (((instr >> 9) & 0x3) == 0x2)
        //         return thumb_decompress_14(instr, &curr_instr);
        //     return thumb_decompress_13(instr, &curr_instr);
        // case 0x6:
        //     switch ((instr >> 12) & 0x3) {
        //     case 0x0:
        //         return thumb_decompress_15(instr, &curr_instr);
        //     case 0x1:
        //         switch ((instr >> 8) & 0xFF) {
        //         case 0b11011111: return thumb_decompress_17(instr, &curr_instr);
        //         case 0b10111110:
        //             fprintf(stderr, "CPU Error [THUMB]: debugging not supported!\n");
        //             exit(1);
        //         }
        //         return thumb_decompress_16(instr, &curr_instr);
        //     }
        //     return THUMB_BAD_INSTR;
        // case 0x7:
        //     switch ((instr >> 11) & 0x3) {
        //     case 0x0: return thumb_decompress_18(instr, &curr_instr);
        //     case 0x2: return THUMB_LONG_BRANCH_1;
        //     case 0x3:
        //     case 0x1: return THUMB_LONG_BRANCH_2;
        //     }
        //     return THUMB_BAD_INSTR;
        // }
        // return THUMB_BAD_INSTR;
    } else {
        switch ((instr >> 25) & 0x7) {
        case 0x0:
            switch ((instr >> 4) & 0xF) {
            case 0x1:
                if (((instr >> 8) & 0xF) == 0xF) return InstrFormat::BX;
                goto psr_or_alu;
            case 0x9:
                switch ((instr >> 23) & 0x3) {
                case 0x0:
                case 0x1: return InstrFormat::MUL;
                case 0x2: return InstrFormat::SWP;
                }
            case 0xB:
            case 0xD: 
            case 0xF: return InstrFormat::HALFWORD_TRANSFER;
            default: goto psr_or_alu;
            }
        case 0x1: goto psr_or_alu;
        case 0x2:
        case 0x3: return InstrFormat::SINGLE_TRANSFER;
        case 0x4: return InstrFormat::BLOCK_TRANSFER;
        case 0x5: return InstrFormat::B;
        case 0x7: return InstrFormat::SWI;
        }
    }

    psr_or_alu: {
        std::uint8_t opcode = (instr >> 21) & 0xF;
        if ((((instr >> 20) & 1) == 0) && ((opcode >> 2) == 0b10)) [[unlikely]] {
            if (opcode & 1) return InstrFormat::MSR;
            return InstrFormat::MRS;
        }
        return InstrFormat::ALU;
    }
}

bool CPU::barrel_shifter(
    std::uint32_t& operand,
    bool& carry_out,
    ShiftType shift_type,
    std::uint8_t shift_amount, 
    bool reg_imm_shift
) {
    // Rs=00h flag is unmodified
    if (!reg_imm_shift && !shift_amount) return false;

    switch (shift_type) {
    case ShiftType::LSL:
        std::cout << "LSL UNIMPLEMENTED" << std::endl;
        std::exit(1);
    case ShiftType::LSR:
        std::cout << "LSR UNIMPLEMENETED" << std::endl;
        std::exit(1);
    case ShiftType::ASR:
        std::cout << "ASR UNIMPLEMEENTED" << std::endl;
        std::exit(1);
    case ShiftType::ROR:
        if (reg_imm_shift && !shift_amount) {
            // ROR#0 (shift by immediate): Interpreted as RRX#1 (RCR), like ROR#1, but Op2 Bit 31 set to old C.
            carry_out = operand & 1;
            operand = (static_cast<std::uint32_t>(m_flags.c) << 31) | (operand >> 1);
        } else {
            operand = ror(operand, shift_amount);
            carry_out = operand >> 31;
        }
        return true;
    }
}

int CPU::branch(std::uint32_t instr) {
    if (condition(instr)) {
        bool bl = (instr >> 24) & 1;
        std::int32_t nn = (static_cast<std::int32_t>((instr & 0xFFFFFF) << 8) >> 8) * 4;
        if (bl) m_regs.r14 = m_regs.r15 - 4;
        m_regs.r15 += nn;
        m_pipeline_invalid = true;
    }
    return 1;
}

int CPU::branch_ex(std::uint32_t instr) {
    if (condition(instr)) {
        std::cout << "branch ex" << std::endl;
    }
    return 1;
}

int CPU::single_transfer(std::uint32_t instr) {
    if (condition(instr)) {
        std::cout << "single transfer" << std::endl;
    }
    return 1;
}

int CPU::halfword_transfer(std::uint32_t instr) {
    if (condition(instr)) {
        bool p = (instr >> 24) & 1;
        bool u = (instr >> 23) & 1;
        bool i = (instr >> 22) & 1;
        bool w = (instr >> 21) & 1;
        bool l = (instr >> 20) & 1;
        auto rn = (instr >> 16) & 0xF;
        auto rd = (instr >> 12) & 0xF;

        std::int32_t offset = (i ? ((((instr >> 8) & 0xF) << 4) | (instr & 0xF)) : 
            m_regs[instr & 0xF]) * -(u & 0);

        auto opcode = (instr >> 5) & 0x3;
        auto addr = m_regs[rn] + (p * offset);
        bool writeback = (p && w) || !p;

        if (l) {
            // https://problemkaputt.de/gbatek.htm#armcpumemoryalignments
            // LDRH and LDRSH alignments are weird.
            switch (opcode) {
            case 0x1: {
                m_regs[rd] = addr & 1 ? ror(m_mem.read_halfword(addr - 1), 8) 
                    : m_mem.read_halfword(addr);
                break;
            }
            case 0x2: {
                std::cout << "LDRSB" << std::endl;
                std::exit(1);
            }
            case 0x3: {
                std::cout << "LDRSH" << std::endl;
                std::exit(1);
            }
            }
        } else {
            switch (opcode) {
            case 0x1: {
                m_mem.write_halfword(addr, m_regs[rd]);
                break;
            }
            case 0x2: {
                std::cout << "LDRD" << std::endl;
                std::exit(1);
            }
            case 0x3: {
                std::cout << "STRD" << std::endl;
                std::exit(1);
            }
            }
        }

        if (writeback && (!l || !(rn == rd)))
            m_regs[rn] += (offset + ((rn == 0xF) << 2));
    }
    return 1;
}

int CPU::block_transfer(std::uint32_t instr) {
    if (condition(instr)) {
        std::cout << "block transfer" << std::endl;
    }
    return 1;
}

int CPU::mrs(std::uint32_t instr) {
    if (condition(instr)) {
        std::cout << "mrs" << std::endl;
    }
    return 1;
}

int CPU::msr(std::uint32_t instr) {
    if (condition(instr)) {
        std::cout << "msr" << std::endl;
    }
    return 1;
}

int CPU::swi(std::uint32_t instr) {
    if (condition(instr)) {
        std::cout << "swi" << std::endl;
    }
    return 1;
}

int CPU::swp(std::uint32_t instr) {
    if (condition(instr)) {
        std::cout << "swp" << std::endl;
    }
    return 1;
}

int CPU::alu(std::uint32_t instr) {
    if (condition(instr)) {
        bool imm = (instr >> 25) & 1;
        bool set_cc = (instr >> 20) & 1;
        auto rn = (instr >> 16) & 0xF;
        auto rd = (instr >> 12) & 0xF;

        auto operand_1 = m_regs[rn];
        std::uint32_t operand_2 = 0;
        bool carry_out = false;
        bool carry_out_modify = false;

        if (imm) {
            int imm_shift = ((instr >> 8) & 0xF) * 2; 
            operand_2 = instr & 0xFF;
            carry_out_modify = barrel_shifter(operand_2, carry_out, ShiftType::ROR, imm_shift, false);
        } else {
            std::cout << "!imm not implemented\n";
            std::exit(1);
        }

        switch ((instr >> 21) & 0xF) {
        case 0x0:
            std::cout << "AND" << std::endl;
            std::exit(1);
        case 0x1:
            std::cout << "EOR" << std::endl;
            std::exit(1);
        case 0x2:
            std::cout << "SUB" << std::endl;
            std::exit(1);
        case 0x3:
            std::cout << "RSB" << std::endl;
            std::exit(1);
        case 0x4: {
            std::uint32_t result = operand_1 + operand_2;
            if (set_cc) {
                m_flags.n = result >> 31;
                m_flags.z = result == 0;
                m_flags.c = ((operand_1 >> 31) + (operand_2 >> 31) > (result >> 31));
                m_flags.v = ((operand_1 >> 31) == (operand_2 >> 31)) && ((operand_1 >> 31) != (result >> 31));
            }
            m_regs[rd] = result;
            break;
        }
        case 0x5:
            std::cout << "ADC" << std::endl;
            std::exit(1);
        case 0x6:
            std::cout << "SBC" << std::endl;
            std::exit(1);
        case 0x7:
            std::cout << "RSC" << std::endl;
            std::exit(1);
        case 0x8:
            std::cout << "TST" << std::endl;
            std::exit(1);
        case 0x9:
            std::cout << "TEQ" << std::endl;
            std::exit(1);
        case 0xA:
            std::cout << "CMP" << std::endl;
            std::exit(1);
        case 0xB:
            std::cout << "CMN" << std::endl;
            std::exit(1);
        case 0xC:
            std::cout << "ORR" << std::endl;
            std::exit(1);
        case 0xD:
            if (set_cc) {
                m_flags.n = operand_2 >> 31;
                m_flags.z = operand_2 == 0;
                m_flags.c = carry_out_modify ? carry_out : m_flags.c;
            }
            m_regs[rd] = operand_2;
            break;
        case 0xF:
            std::cout << "MVN" << std::endl;
            std::exit(1);
        }
    }
    return 1;
}

int CPU::mul(std::uint32_t instr) {
    if (condition(instr)) {
        std::cout << "mul" << std::endl;
    }
    return 1;
}

int CPU::execute() {
    std::uint32_t instr = m_pipeline_invalid ? fetch() : m_pipeline;
    m_pipeline_invalid = false;

    switch (decode(instr)) {
    case InstrFormat::B: return branch(instr);
    case InstrFormat::BX: return branch_ex(instr);
    case InstrFormat::SINGLE_TRANSFER: return single_transfer(instr);
    case InstrFormat::HALFWORD_TRANSFER: return halfword_transfer(instr);
    case InstrFormat::BLOCK_TRANSFER: return block_transfer(instr);
    case InstrFormat::MRS: return mrs(instr);
    case InstrFormat::MSR: return msr(instr);
    case InstrFormat::SWI: return swi(instr);
    case InstrFormat::SWP: return swp(instr);
    case InstrFormat::ALU: return alu(instr);
    case InstrFormat::MUL: return mul(instr);
    case InstrFormat::NOP: return 1;
    }
}

void CPU::render_frame() {
    int cycles = 0;
    while (cycles < cycles_per_frame) {
        int instr_cycles = execute();
        m_mem.tick_components(instr_cycles);
        cycles += instr_cycles;
    }
}

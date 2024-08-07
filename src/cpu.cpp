#include "cpu.hpp"

#include <iostream>
#include <utility>

constexpr int halfword_access = 2;
constexpr int word_access = 4;
// constexpr int cycles_per_frame = 280896;

bool CPU::thumb_enabled() {
    return (m_regs.cpsr >> 5) & 1;
}

void CPU::change_cpsr_mode(Mode mode) {
    m_regs = m_banked_regs[std::to_underlying(mode)];
    bool n = (m_regs.cpsr >> 31) & 1;
    bool z = (m_regs.cpsr >> 30) & 1;
    bool c = (m_regs.cpsr >> 29) & 1;
    bool v = (m_regs.cpsr >> 28) & 1;
    flags = Flags{n, z, c, v};
}

bool CPU::condition(const std::uint32_t instr) {
    switch ((instr >> 28) & 0xF) {
    case 0x0: return flags.z;
    case 0x1: return !flags.z;
    case 0x2: return flags.c;
    case 0x3: return !flags.c;
    case 0x4: return flags.n;
    case 0x5: return !flags.n;
    case 0x6: return flags.v;
    case 0x7: return !flags.v;
    case 0x8: return flags.c & !flags.z;
    case 0x9: return !flags.c | flags.z;
    case 0xA: return flags.n == flags.v;
    case 0xB: return flags.n ^ flags.v;
    case 0xC: return !flags.z && (flags.n == flags.v);
    case 0xD: return flags.z || (flags.n ^ flags.v);
    case 0xE: return true;
    }
    std::unreachable();
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

CPU::InstrFormat CPU::decode(const std::uint32_t instr) {
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
        if ((opcode >> 2) == 0b10) {
            if (opcode & 1) {
                return InstrFormat::MSR;
            }
            return InstrFormat::MRS;
        }
        return InstrFormat::ALU;
    }
}

std::uint32_t CPU::barrel_shifter(
    ShiftType shift_type, 
    std::uint32_t value, 
    std::uint8_t shift_amount, 
    bool reg_imm_shift
) {
    return 0;
}

std::size_t CPU::branch(const std::uint32_t instr) {
    if (condition(instr)) {
        bool bl = (instr >> 24) & 1;
        std::int32_t nn = (static_cast<std::int32_t>((instr & 0xFFFFFF) << 8) >> 8) * 4;
        if (bl) m_regs.r14 = m_regs.r15 - 4;
        m_regs.r15 += nn;
        m_pipeline_invalid = true;
    }
    return 1;
}

std::size_t CPU::branch_ex(const std::uint32_t instr) {
    std::cout << "branch ex" << std::endl;

    if (condition(instr)) {
        std::cout << "branch ex" << std::endl;
    }
    return 1;
}

std::size_t CPU::single_transfer(const std::uint32_t instr) {
    std::cout << "single transfer" << std::endl;

    if (condition(instr)) {
        std::cout << "single transfer" << std::endl;
    }
    return 1;
}

std::size_t CPU::halfword_transfer(const std::uint32_t instr) {
    std::cout << "halfword transfer" << std::endl;

    if (condition(instr)) {
        std::cout << "halfword transfer" << std::endl;
    }
    return 1;
}

std::size_t CPU::block_transfer(const std::uint32_t instr) {
    std::cout << "block transfer" << std::endl;

    if (condition(instr)) {
        std::cout << "block transfer" << std::endl;
    }
    return 1;
}

std::size_t CPU::mrs(const std::uint32_t instr) {
    std::cout << "mrs 1" << std::endl;

    if (condition(instr)) {
        std::cout << "mrs 2" << std::endl;
    }
    return 1;
}

std::size_t CPU::msr(const std::uint32_t instr) {
    std::cout << "msr" << std::endl;

    if (condition(instr)) {
        std::cout << "msr" << std::endl;
    }
    return 1;
}

std::size_t CPU::swi(const std::uint32_t instr) {
    std::cout << "swi" << std::endl;

    if (condition(instr)) {
        std::cout << "swi" << std::endl;
    }
    return 1;
}

std::size_t CPU::swp(const std::uint32_t instr) {
    std::cout << "swp" << std::endl;

    if (condition(instr)) {
        std::cout << "swp" << std::endl;
    }
    return 1;
}

std::size_t CPU::alu(const std::uint32_t instr) {
    if (condition(instr)) {
        bool imm = (instr >> 25) & 1;
        // bool set_cc = (instr >> 20) & 1;
        // std::uint32_t rn = (instr >> 16) & 0xF;
        // std::uint32_t rd = (instr >> 12) & 0xF;

        std::uint32_t operand_2 = 0;

        if (imm) {
            std::size_t imm_shift = ((instr >> 8) & 0xF) * 2; 
            std::uint8_t nn = instr & 0xFF;
            operand_2 = barrel_shifter(ShiftType::ROR, nn, imm_shift, false);
        } else {
            std::cout << "!imm not implemented\n";
            std::exit(1);
        }

        std::cout << std::hex << operand_2 << std::endl;

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
        case 0x4:
            std::cout << "ADD" << std::endl;
            std::exit(1);
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
            std::cout << "MOV" << std::endl;
            std::exit(1);
        case 0xF:
            std::cout << "MVN" << std::endl;
            std::exit(1);
        }
    }
    return 1;
}

std::size_t CPU::mul(const std::uint32_t instr) {
    std::cout << "mul" << std::endl;

    if (condition(instr)) {
        std::cout << "mul" << std::endl;
    }
    return 1;
}

std::size_t CPU::execute() {
    const std::uint32_t instr = m_pipeline_invalid ? fetch() : m_pipeline;
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

void CPU::render_frame() noexcept {
    // int cycles = 0;

    execute();
    execute();

    // while (cycles < cycles_per_frame) {
    //     execute();
    //     // tick components n number of cycles
    // }
}

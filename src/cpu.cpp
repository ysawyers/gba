#include "cpu.hpp"

#include <iostream>
#include <utility>

constexpr int halfword_access = 2;
constexpr int word_access = 4;
constexpr int cycles_per_frame = 280896;

CPU::CPU(const std::string&& rom_filepath) 
    : m_pipeline(0), m_pipeline_invalid(true), m_thumb_enabled(false) 
{
    m_mem.load_bios();
    m_mem.load_rom(std::move(rom_filepath));
    m_banked_regs[SYS].mode = 0x1F;
    m_regs.mode = 0x1F;

    // initializes registers while bios is unimplemented
    m_banked_regs[SVC][13] = 0x03007FE0;
    m_banked_regs[IRQ][13] = 0x03007FA0;
    m_regs[13] = 0x03007F00;
    m_regs[14] = 0x08000000;
    m_regs[15] = 0x08000000;
}

Registers& CPU::get_bank(std::uint8_t mode) {
    switch (mode) {
    case 0b10000:
    case 0b11111: return m_banked_regs[SYS];
    case 0b10001: return m_banked_regs[FIQ];
    case 0b10010: return m_banked_regs[IRQ];
    case 0b10011: return m_banked_regs[SVC];
    case 0b10111: return m_banked_regs[ABT];
    case 0b11011: return m_banked_regs[UND];
    default: std::unreachable();
    }
}

void CPU::change_bank(std::uint8_t new_mode) {
    get_bank(m_banked_regs[SYS].mode) = m_regs;
    switch (new_mode) {
    case 0b11011:
    case 0b10111:
    case 0b10011:
    case 0b10010:
    case 0b10000: 
    case 0b11111: {
        auto& bank = get_bank(new_mode);
        for (int i = 8; i < 15; i++) {
            m_regs[i] = bank[i];
        }
        break;
    }
    case 0b10001: {
        for (int i = 0; i < 6; i++) {
            if (i == FIQ) continue;
            for (int j = 8; j < 13; j++) {
                m_banked_regs[i][j] = m_regs[j];
            }
        }
        for (int i = 8; i < 15; i++) {
            m_regs[i] = m_banked_regs[FIQ][i];
        }
        break;
    }
    default: std::unreachable();
    }
    m_banked_regs[SYS].mode = new_mode;
}

std::uint32_t CPU::get_psr() {
    std::uint8_t flags = (m_regs.flags.n << 3) | (m_regs.flags.z << 2) | (m_regs.flags.c << 1) | m_regs.flags.v;
    return static_cast<std::uint32_t>(flags << 28) | m_regs.mode;
}

std::uint32_t CPU::get_cpsr() {
    auto& sys = m_banked_regs[SYS];
    std::uint8_t flags = (sys.flags.n << 3) | (sys.flags.z << 2) | (sys.flags.c << 1) | sys.flags.v;
    return static_cast<std::uint32_t>(flags << 28) | sys.mode;
}

std::uint32_t CPU::ror(std::uint32_t operand, std::size_t shift_amount) {
    return (operand >> (shift_amount & 31)) | (operand << ((-shift_amount) & 31));
}

void CPU::safe_reg_assign(std::uint8_t reg, std::uint32_t value) {
    m_regs[reg] = value;
    m_pipeline_invalid |= (reg == 0xF);
}

bool CPU::condition(std::uint32_t instr) {
    switch ((instr >> 28) & 0xF) {
    case 0x0: return m_regs.flags.z;
    case 0x1: return !m_regs.flags.z;
    case 0x2: return m_regs.flags.c;
    case 0x3: return !m_regs.flags.c;
    case 0x4: return m_regs.flags.n;
    case 0x5: return !m_regs.flags.n;
    case 0x6: return m_regs.flags.v;
    case 0x7: return !m_regs.flags.v;
    case 0x8: return m_regs.flags.c & !m_regs.flags.z;
    case 0x9: return !m_regs.flags.c | m_regs.flags.z;
    case 0xA: return m_regs.flags.n == m_regs.flags.v;
    case 0xB: return m_regs.flags.n ^ m_regs.flags.v;
    case 0xC: return !m_regs.flags.z && (m_regs.flags.n == m_regs.flags.v);
    case 0xD: return m_regs.flags.z || (m_regs.flags.n ^ m_regs.flags.v);
    case 0xE: return true;
    default: std::unreachable();
    }
}

std::uint32_t CPU::fetch_arm() {
    std::uint32_t instr = m_mem.read_word(m_regs[15]);
    m_regs[15] += word_access;
    return instr;
}

std::uint32_t CPU::fetch() {
    std::uint32_t instr = 0;
    if (m_thumb_enabled) {
        instr = m_mem.read_halfword(m_regs[15]);
        m_regs[15] += halfword_access;
    } else {
        instr = m_mem.read_word(m_regs[15]);
        m_regs[15] += word_access;
    }
    return instr;
}

// TODO: convert to LUT
CPU::InstrFormat CPU::decode(std::uint32_t instr) {
    if (m_thumb_enabled) {
        switch ((instr >> 13) & 0x7) {
        case 0x0:
            if (((instr >> 11) & 0x3) == 0x3)  {
                std::cout << "thumb 2" << std::endl;
                std::exit(1);
            }
            std::cout << "thumb 1" << std::endl;
            std::exit(1);
        case 0x1: return InstrFormat::THUMB_3;
        case 0x2:
            switch ((instr >> 10) & 0x7) {
            case 0x0:
                std::cout << "thumb 4" << std::endl;
                std::exit(1);
            case 0x1: return InstrFormat::THUMB_5;
            case 0x2:
            case 0x3: 
                std::cout << "thumb 6" << std::endl;
                std::exit(1);
            }
            if ((instr >> 9) & 1) {
                std::cout << "thumb 8" << std::endl;
                std::exit(1);
            }
            std::cout << "thumb 7" << std::endl;
            std::exit(1);
        case 0x3:
            std::cout << "thumb 9" << std::endl;
            std::exit(1);
        case 0x4:
            if ((instr >> 12) & 1) {
                std::cout << "thumb 11" << std::endl;
                std::exit(1);
            }
            std::cout << "thumb 10" << std::endl;
            std::exit(1);
        case 0x5:
            if (((instr >> 12) & 1) == 0) return InstrFormat::THUMB_12;
            if (((instr >> 9) & 0x3) == 0x2) {
                std::cout << "thumb 14" << std::endl;
                std::exit(1);
            }
            std::cout << "thumb 13" << std::endl;
            std::exit(1);
        case 0x6:
            switch ((instr >> 12) & 0x3) {
            case 0x0:
                std::cout << "thumb 15" << std::endl;
                std::exit(1);
            case 0x1:
                switch ((instr >> 8) & 0xFF) {
                case 0b11011111: {
                    std::cout << "thumb 17" << std::endl;
                    std::exit(1);
                }
                case 0b10111110:
                    fprintf(stderr, "CPU Error [THUMB]: debugging not supported!\n");
                    exit(1);
                }
                std::cout << "thumb 16" << std::endl;
                std::exit(1);
            }
            std::unreachable();
        case 0x7:
            switch ((instr >> 11) & 0x3) {
            case 0x0:
                std::cout << "thumb 18" << std::endl;
                std::exit(1);
            case 0x2:
                std::cout << "thumb long branch 1" << std::endl;
                std::exit(1);
            case 0x3:
            case 0x1:
                std::cout << "thumb long branch 2" << std::endl;
                std::exit(1);
            }
        }
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
    switch (shift_type) {
    case ShiftType::LSL: {
        bool nonzero_shift = shift_amount != 0;
        carry_out = (operand >> (nonzero_shift * (32 - (shift_amount | !nonzero_shift)))) & 1;
        operand = !(shift_amount > 31) * (operand << (shift_amount & 31));
        return nonzero_shift;
    }
    case ShiftType::LSR: {
        std::uint32_t adjusted_shift = (((shift_amount != 0) * (shift_amount - 1)) + ((shift_amount == 0) * 31));
        carry_out = ((operand >> (adjusted_shift & 31)) & 1) * !(shift_amount > 32);
        operand = ((operand >> adjusted_shift) * !(shift_amount > 32)) >> 1;
        return true;
    }
    case ShiftType::ASR: {
        bool toggle = (shift_amount == 0) | (shift_amount > 31);
        std::uint32_t adjusted_shift = ((!toggle * (shift_amount - 1)) + (toggle * 31));
        carry_out = (operand >> adjusted_shift) & 1;
        operand = (static_cast<std::int32_t>(operand) >> adjusted_shift) >> !toggle;
        return true;
    }
    case ShiftType::ROR:
        if (reg_imm_shift && !shift_amount) {
            // ROR#0 (shift by immediate): Interpreted as RRX#1 (RCR), like ROR#1, but Op2 Bit 31 set to old C.
            carry_out = operand & 1;
            operand = (static_cast<std::uint32_t>(m_regs.flags.c) << 31) | (operand >> 1);
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
        if (bl) m_regs[14] = m_regs[15] - 4;
        m_regs[15] += nn;
        m_pipeline_invalid = true;
    }
    return 1;
}

int CPU::branch_ex(std::uint32_t instr) {
    if (condition(instr)) {
        std::uint8_t rn = instr & 0xF;
        if (((instr >> 4) & 0xF) == 0x1) {
            if (m_regs[rn] & 1) {
                m_thumb_enabled = true;
                m_regs[15] = m_regs[rn] & ~0x1;
            } else {
                m_thumb_enabled = false;
                m_regs[15] = m_regs[rn] & ~0x3;
            }
        } else {
            std::cout << "BLX" << std::endl;
            std::exit(1);
        }
        m_pipeline_invalid = true;
    }
    return 1;
}

int CPU::single_transfer(std::uint32_t instr) {
    if (condition(instr)) {
        bool i = (instr >> 25) & 1;
        bool p = (instr >> 24) & 1;
        bool u = (instr >> 23) & 1;
        bool b = (instr >> 22) & 1;
        bool w = (instr >> 21) & 1;
        bool l = (instr >> 20) & 1;
        std::uint8_t rn = (instr >> 16) & 0xF;
        std::uint8_t rd = (instr >> 12) & 0xF;
        std::uint32_t offset = 0;

        if (i) {
            std::uint8_t rm = instr & 0xF;
            std::uint32_t operand = m_regs[rm];
            std::uint8_t shift_amount = (instr >> 7) & 0x1F;
            ShiftType shift_type = static_cast<ShiftType>((instr >> 5) & 3);

            bool carry_out = false;
            barrel_shifter(operand, carry_out, shift_type, shift_amount, true);
            offset = operand;
        } else {
            offset = instr & 0xFFF;
        }
        offset *= (-1 + (u * 2));

        std::uint32_t addr = m_regs[rn] + (p ? offset : 0);
        bool writeback = !p || (p && w);

        if (l) {
            // https://problemkaputt.de/gbatek.htm#armcpumemoryalignments
            // LDR alignments are weird.
            safe_reg_assign(rd, b ? m_mem.read_byte(addr) : ror(m_mem.read_word(addr), (addr & 0x3) * 8));
        } else {
            std::uint32_t value = m_regs[rd] + ((rd == 0xF) * 4);
            if (b) {
                m_mem.write_byte(addr, value);
            } else {
                m_mem.write_word(addr, value);
            }
        }

        if (writeback && (!l || !(rn == rd))) {
            safe_reg_assign(rn, m_regs[rn] + (((rd == 0xF) * 4) + offset));
        }
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
            m_regs[instr & 0xF]) * (-1 + (u * 2));

        auto opcode = (instr >> 5) & 0x3;
        auto addr = m_regs[rn] + (p * offset);
        bool writeback = (p && w) || !p;

        if (l) {
            bool misaligned_read = addr & 1;
            // https://problemkaputt.de/gbatek.htm#armcpumemoryalignments
            // LDRH and LDRSH alignments are weird.
            switch (opcode) {
            case 0x1: {
                safe_reg_assign(rd, misaligned_read ? 
                    ror(m_mem.read_halfword(addr - 1), 8) : m_mem.read_halfword(addr));
                break;
            }
            case 0x2: {
                safe_reg_assign(rd, static_cast<std::int32_t>(static_cast<std::int8_t>(m_mem.read_byte(addr))));
                break;
            }
            case 0x3: {
                std::uint32_t value = misaligned_read ? static_cast<std::int32_t>(static_cast<std::int8_t>(m_mem.read_byte(addr)))
                    : static_cast<std::int32_t>(static_cast<std::int16_t>(m_mem.read_halfword(addr)));
                safe_reg_assign(rd, value);
                break;
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

        if (writeback && (!l || !(rn == rd))) {
            safe_reg_assign(rn, m_regs[rn] + (offset + ((rn == 0xF) * 4)));
        }
    }
    return 1;
}

int CPU::block_transfer(std::uint32_t instr) {
    if (condition(instr)) {
        bool p = (instr >> 24) & 1;
        bool u = (instr >> 23) & 1;
        bool s = (instr >> 22) & 1;
        bool w = (instr >> 21) & 1;
        bool l = (instr >> 20) & 1;
        std::uint8_t rn = (instr >> 16) & 0xF;
        std::uint16_t reg_list = instr & 0xFFFF;

        int transfers = __builtin_popcount(reg_list);
        std::uint32_t transfer_base_addr = m_regs[rn];
        std::uint32_t direction = 4 - (!u * 8);

        std::uint8_t prev_mode = 0;
        if (s) {
            if (l && ((reg_list >> 15) & 1)) {
                change_bank(m_regs.mode);
            } else {
                prev_mode = m_banked_regs[SYS].mode;
                change_bank(0x1F);
            }
        }

        // (ARMv4 edge case): for an empty register list r15 is loaded/stored and base register is
        // written back to +/-40h since the register count is 16 (even though only 1 transfer occurs)
        if (transfers == 0) {
            std::uint32_t addr = u ? transfer_base_addr + (p * 4) : ((transfer_base_addr + (16 * direction)) + (!p * 4));
            if (l) {
                m_regs[15] = m_mem.read_word(addr);
                m_pipeline_invalid = true;
            } else {
                m_mem.write_word(addr, m_regs[15] + (m_thumb_enabled ? 2 : 4));
            }
            m_regs[rn] += (16 * direction);
            return 1;
        } else if (w) {
            m_regs[rn] += (transfers * direction);
        }

        // transfers from lowest memory address to highest
        int reg_start, reg_end, step;
        if (u) {
            reg_start = __builtin_ffs(reg_list) - 1;
            reg_end = 0x10;
            step = 1;
        } else {
            reg_start = 31 - __builtin_clz(reg_list);
            reg_end = -1;
            step = -1;
        }

        if (l) {
            for (int i = reg_start; i != reg_end; i += step)
                if ((reg_list >> i) & 1) {
                    std::uint32_t addr = transfer_base_addr + (p * direction);
                    safe_reg_assign(i, m_mem.read_word(addr));
                    transfer_base_addr += direction;
                }
        } else {
            bool base_transferred = (reg_list >> rn) & 1;
            if (base_transferred && ((__builtin_ffs(reg_list) - 1) == rn)) {
                std::uint32_t addr = transfer_base_addr + (u * -4) + (direction * (!p + (p * transfers)));
                m_mem.write_word(addr, transfer_base_addr);
                reg_list &= ~(1 << rn);
                transfer_base_addr += (direction * u);
            }

            std::uint32_t pc_offset = m_thumb_enabled ? 2 : 4;
            for (int i = reg_start; i != reg_end; i += step)
                if ((reg_list >> i) & 1) {
                    std::uint32_t addr = transfer_base_addr + (p * direction);
                    m_mem.write_word(addr, m_regs[i] + ((i == 0xF) * pc_offset));
                    transfer_base_addr += direction;
                }
        }

        if (prev_mode) {
            change_bank(prev_mode);
        }
    }
    return 1;
}

int CPU::mrs(std::uint32_t instr) {
    if (condition(instr)) {
        bool psr = (instr >> 22) & 1;
        uint8_t rd = (instr >> 12) & 0xF;
        if (psr) {
            m_regs[rd] = get_psr();
        } else {
            m_regs[rd] = get_cpsr();
        }
    }
    return 1;
}

int CPU::msr(std::uint32_t instr) {
    if (condition(instr)) {
        bool i = (instr >> 25) & 1;
        bool psr = (instr >> 22) & 1;
        bool f = (instr >> 19) & 1;
        bool c = (instr >> 16) & 1;

        std::uint8_t shift_amount = ((instr >> 8) & 0xF) * 2;
        auto imm = instr & 0xFF;
        
        std::uint32_t operand = i ? ror(imm, shift_amount) : m_regs[instr & 0xF];
        std::uint32_t flags = operand & 0xFF000000;
        std::uint32_t mode = operand & 0x000000FF;

        if (psr) {
            if (f) {
                m_regs.flags.n = flags >> 31;
                m_regs.flags.z = (flags >> 30) & 1;
                m_regs.flags.c = (flags >> 29) & 1;
                m_regs.flags.v = (flags >> 28) & 1;
            }
            if (c) {
                if (m_banked_regs[SYS].mode == 0x1F || m_banked_regs[SYS].mode == 0x10) {
                    change_bank(mode);
                } else {
                    m_regs.mode = mode;
                }
            }
        } else {
            Registers& sys_bank = m_banked_regs[SYS];
            if (f) {
                sys_bank.flags.n = flags >> 31;
                sys_bank.flags.z = (flags >> 30) & 1;
                sys_bank.flags.c = (flags >> 29) & 1;
                sys_bank.flags.v = (flags >> 28) & 1;
                if (sys_bank.mode == 0x1F || sys_bank.mode == 0x10) {
                    m_regs.flags = sys_bank.flags;
                }
            }
            if (c) {
                change_bank(mode);
            }
        }
    }
    return 1;
}

int CPU::swi(std::uint32_t instr) {
    if (condition(instr)) {
        std::cout << "swi" << std::endl;
        std::exit(1);
    }
    return 1;
}

int CPU::swp(std::uint32_t instr) {
    if (condition(instr)) {
        bool b = (instr >> 22) & 1;
        std::uint8_t rn = (instr >> 16) & 0xF;
        std::uint8_t rd = (instr >> 12) & 0xF;
        std::uint8_t rm = instr & 0xF;

        if (b) {
            std::uint32_t value = m_mem.read_byte(m_regs[rn]);
            m_mem.write_byte(m_regs[rn], m_regs[rm]);
            safe_reg_assign(rd, value);
        } else {
            std::uint32_t value = ror(m_mem.read_word(m_regs[rn]), (m_regs[rn] & 0x3) * 8);
            m_mem.write_word(m_regs[rn], m_regs[rm]);
            safe_reg_assign(rd, value);
        }
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
            std::uint8_t imm_shift = ((instr >> 8) & 0xF) * 2; 
            operand_2 = instr & 0xFF;
            if (imm_shift) {
                carry_out_modify = barrel_shifter(operand_2, carry_out, ShiftType::ROR, imm_shift, false);
            }
        } else {
            bool r = (instr >> 4) & 1;
            ShiftType shift_type = static_cast<ShiftType>((instr >> 5) & 0x3);
            std::uint8_t rm = instr & 0xF;

            operand_2 = m_regs[rm];
            if (r) {
                std::uint32_t pc_offset = m_thumb_enabled ? 2 : 4;
                if (rn == 0xF) operand_1 = m_regs[15] + pc_offset;
                if (rm == 0xF) operand_2 = m_regs[15] + pc_offset;
                std::uint8_t shift_amount = m_regs[(instr >> 8) & 0xF] & 0xFF;
                if (shift_amount) {
                    carry_out_modify = barrel_shifter(operand_2, carry_out, shift_type, shift_amount, false);
                }
            } else {
                std::uint8_t shift_amount = (instr >> 7) & 0x1F;
                carry_out_modify = barrel_shifter(operand_2, carry_out, shift_type, shift_amount, true);
            }
        }

        switch ((instr >> 21) & 0xF) {
        case 0x0: {
            std::uint32_t result = operand_1 & operand_2;
            if (set_cc) {
                m_regs.flags.n = result >> 31;
                m_regs.flags.z = !result;
                if (carry_out_modify) {
                    m_regs.flags.c = carry_out;
                }
            }
            safe_reg_assign(rd, result);
            break;
        }
        case 0x1: {
            std::uint32_t result = operand_1 ^ operand_2;
            if (set_cc) {
                m_regs.flags.n = result >> 31;
                m_regs.flags.z = !result;
                if (carry_out_modify) {
                    m_regs.flags.c = carry_out;
                }
            }
            safe_reg_assign(rd, result);
            break;
        }
        case 0x3: { // RSB
            std::uint32_t temp = operand_1;
            operand_1 = operand_2;
            operand_2 = temp;
        }
        case 0x2: { // SUB
            std::uint32_t result = operand_1 - operand_2;
            if (set_cc) {
                m_regs.flags.n = result >> 31;
                m_regs.flags.z = !result;
                m_regs.flags.c = operand_1 >= operand_2;
                m_regs.flags.v = ((operand_1 >> 31) != (operand_2 >> 31)) && ((operand_1 >> 31) != (result >> 31));
            }
            safe_reg_assign(rd, result);
            break;
        }
        case 0x4: { // ADD
            std::uint32_t result = operand_1 + operand_2;
            if (set_cc) {
                m_regs.flags.n = result >> 31;
                m_regs.flags.z = result == 0;
                m_regs.flags.c = ((operand_1 >> 31) + (operand_2 >> 31) > (result >> 31));
                m_regs.flags.v = ((operand_1 >> 31) == (operand_2 >> 31)) && ((operand_1 >> 31) != (result >> 31));
            }
            safe_reg_assign(rd, result);
            break;
        }
        case 0x5: {
            std::uint32_t result = operand_1 + operand_2 + m_regs.flags.c;
            if (set_cc) {
                m_regs.flags.n = result >> 31;
                m_regs.flags.z = !result;
                m_regs.flags.c = ((operand_1 >> 31) + (operand_2 >> 31) > (result >> 31));
                m_regs.flags.v = ((operand_1 >> 31) == (operand_2 >> 31)) && ((operand_1 >> 31) != (result >> 31));
            }
            safe_reg_assign(rd, result);
            break;
        }
        case 0x7: { // RSC
            std::uint32_t temp = operand_1;
            operand_1 = operand_2;
            operand_2 = temp;
        }
        case 0x6: {
            std::uint32_t result = operand_1 - operand_2 - !m_regs.flags.c;
            if (set_cc) {
                m_regs.flags.n = result >> 31;
                m_regs.flags.z = !result;
                m_regs.flags.c = static_cast<std::uint64_t>(operand_1) >= (static_cast<std::uint64_t>(operand_2) + !m_regs.flags.c);
                m_regs.flags.v = ((operand_1 >> 31) != (operand_2 >> 31)) && ((operand_1 >> 31) != (result >> 31));
            }
            safe_reg_assign(rd, result);
            break;
        }
        case 0x8: {
            std::uint32_t result = operand_1 & operand_2;
            m_regs.flags.n = result >> 31;
            m_regs.flags.z = !result;
            if (carry_out_modify) {
                m_regs.flags.c = carry_out;
            }
            break;
        }
        case 0x9: { // TEQ
            std::uint32_t result = operand_1 ^ operand_2;
            m_regs.flags.n = result >> 31;
            m_regs.flags.z = !result;
            if (carry_out_modify) {
                m_regs.flags.c = carry_out;
            }
            break;
        }
        case 0xA: { // CMP
            std::uint32_t result = operand_1 - operand_2;
            m_regs.flags.n = result >> 31;
            m_regs.flags.z = result == 0;
            m_regs.flags.c = operand_1 >= operand_2;
            m_regs.flags.v = ((operand_1 >> 31) != (operand_2 >> 31)) && ((operand_1 >> 31) != (result >> 31));
            break;
        }
        case 0xB: { // CMN
            std::uint32_t result = operand_1 + operand_2;
            m_regs.flags.n = result >> 31;
            m_regs.flags.z = !result;
            m_regs.flags.c = ((operand_1 >> 31) + (operand_2 >> 31) > (result >> 31));
            m_regs.flags.v = ((operand_1 >> 31) == (operand_2 >> 31)) && ((operand_1 >> 31) != (result >> 31));
            break;
        }
        case 0xC: { // ORR
            std::uint32_t result = operand_1 | operand_2;
            if (set_cc) {
                m_regs.flags.n = result >> 31;
                m_regs.flags.z = !result;
                if (carry_out_modify) {
                    m_regs.flags.c = carry_out;
                }
            }
            safe_reg_assign(rd, result);
            break;
        }
        case 0xE: { // BIC
            std::uint32_t result = operand_1 & ~operand_2;
            if (set_cc) {
                m_regs.flags.n = result >> 31;
                m_regs.flags.z = !result;
                if (carry_out_modify) {
                    m_regs.flags.c = carry_out;
                }
            } 
            safe_reg_assign(rd, result);
            break;
        }
        case 0xF: // MVN
            operand_2 = ~operand_2;
        case 0xD: // MOV
            if (set_cc) {
                m_regs.flags.n = operand_2 >> 31;
                m_regs.flags.z = !operand_2;
                if (carry_out_modify) {
                    m_regs.flags.c = carry_out;
                }
            }
            safe_reg_assign(rd, operand_2);
            break;
        }

        if ((rd == 0xF) && set_cc) [[unlikely]] {
            change_bank(m_regs.mode);
        }
    }
    return 1;
}

int CPU::mul(std::uint32_t instr) {
    if (condition(instr)) {
        bool s = (instr >> 20) & 1;
        std::uint8_t rd = (instr >> 16) & 0xF;
        std::uint8_t rn = (instr >> 12) & 0xF;
        std::uint8_t rs = (instr >> 8) & 0xF;
        std::uint8_t rm = instr & 0xF;

        switch ((instr >> 21) & 0xF) {
        case 0x0: {
            std::uint32_t result = m_regs[rm] * m_regs[rs];
            if (s) {
                m_regs.flags.n = result >> 31;
                m_regs.flags.z = !result;
            }
            safe_reg_assign(rd, result);
            break;
        }
        case 0x1: {
            std::uint32_t result = (m_regs[rm] * m_regs[rs]) + m_regs[rn];
            if (s) {
                m_regs.flags.n = result >> 31;
                m_regs.flags.z = !result;
            }
            safe_reg_assign(rd, result);
            break;
        }
        case 0x2: {
            printf("3");
            std::exit(1);
        }
        case 0x4: {
            std::uint64_t result = static_cast<std::uint64_t>(m_regs[rm]) * m_regs[rs];
            if (s) {
                m_regs.flags.n = result >> 63;
                m_regs.flags.z = !result;
            }
            safe_reg_assign(rn, result);
            safe_reg_assign(rd, result >> 32);
            break;
        }
        case 0x5: {
            std::uint64_t result = static_cast<std::uint64_t>(m_regs[rm]) * m_regs[rs] 
                + ((static_cast<std::uint64_t>(m_regs[rd]) << 32) | m_regs[rn]);
            if (s) {
                m_regs.flags.n = result >> 63;
                m_regs.flags.z = !result;
            }
            safe_reg_assign(rn, result);
            safe_reg_assign(rd, result >> 32);
            break;
        }
        case 0x6: {
            std::int64_t result = static_cast<std::int64_t>(static_cast<std::int32_t>(m_regs[rm])) 
                * static_cast<std::int64_t>(static_cast<std::int32_t>(m_regs[rs]));
            if (s) {
                m_regs.flags.n = result >> 63;
                m_regs.flags.z = !result;
            }
            safe_reg_assign(rn, result);
            safe_reg_assign(rd, result >> 32);
            break;
        }
        case 0x7: {
            std::int64_t result = static_cast<std::int64_t>(static_cast<std::int32_t>(m_regs[rm])) 
                * static_cast<std::int64_t>(static_cast<std::int32_t>(m_regs[rs]))
                    + (static_cast<std::int64_t>((static_cast<std::uint64_t>(m_regs[rd]) << 32) ) | m_regs[rn]);
            if (s) {
                m_regs.flags.n = result >> 63;
                m_regs.flags.z = !result;
            }
            safe_reg_assign(rn, result);
            safe_reg_assign(rd, result >> 32);
            break;
        }
        default: std::unreachable();
        }
    }
    return 1;
}

std::uint32_t CPU::thumb_translate_3(std::uint16_t instr) {
    std::uint32_t translation = 0b11100010000100000000000000000000;
    std::uint32_t rd = (instr >> 8) & 0x7;
    std::uint32_t nn = instr & 0xFF;
    std::uint32_t thumb_opcode = (instr >> 11) & 0x3;
    std::uint32_t arm_opcode = 0b1101;

    arm_opcode = (arm_opcode << thumb_opcode) & 0xF;
    arm_opcode >>= (!(~thumb_opcode & 0x3) << 1);
    translation |= (arm_opcode << 21);
    translation |= (rd << 16);
    translation |= (rd << 12);
    translation |= nn;

    return translation;
}

std::uint32_t CPU::thumb_translate_5(std::uint16_t instr, std::uint32_t rs, std::uint32_t thumb_opcode) {
    std::uint32_t translation = 0b11100000000000000000000000000000;
    std::uint32_t rd = (((instr >> 7) & 1) << 3) | (instr & 0x7); // MSBd added (r0-r15)
    std::uint32_t arm_opcode = 0b110100;

    arm_opcode = (arm_opcode >> thumb_opcode) & 0xF;
    translation |= ((thumb_opcode == 0x1) << 20);
    translation |= (arm_opcode << 21);
    translation |= (rd << 16);
    translation |= (rd << 12);
    translation |= rs;

    return translation;
}

int CPU::execute() {
    if (m_thumb_enabled) {
        // case InstrFormat::THUMB_1: {
        //     std::cout << "thumb 1" << std::endl;
        //     std::exit(1);
        // }
        // case InstrFormat::THUMB_6: {
        //     std::cout << "thumb 2" << std::endl;
        //     std::exit(1);
        // }
        // case InstrFormat::THUMB_3: return alu(thumb_translate_3(instr));
        // case InstrFormat::THUMB_5: {
        //     std::uint32_t thumb_opcode = (instr >> 8) & 0x3;
        //     std::uint32_t rs = (((instr >> 6) & 1) << 3) | ((instr >> 3) & 0x7);
        //     if (thumb_opcode == 0x3) {
        //         std::uint32_t translation = 0b11100001001011111111111100010000;
        //         translation |= rs;
        //         return branch_ex(translation);
        //     }
        //     return alu(thumb_translate_5(instr, rs, thumb_opcode));
        // }
        // case InstrFormat::THUMB_12: {
        //     std::uint8_t rd = (instr >> 8) & 0x7;
        //     std::uint32_t nn = (instr & 0xFF) << 2;
        //     if ((instr >> 11) & 1) {
        //         m_regs[rd] = m_regs[13] + nn;
        //     } else {
        //         m_regs[rd] = (m_regs[15] & ~2) + nn;
        //     }
        //     return 1;
        // }
        printf("thumb unhandled\n");
        std::exit(1);
    } else {
        std::uint32_t instr = m_pipeline_invalid ? fetch_arm() : m_pipeline;
        m_pipeline_invalid = false;
        m_pipeline = fetch_arm();

        if (condition(instr)) [[likely]] {
            std::uint16_t opcode = (((instr >> 20) & 0xFF) << 4) | ((instr >> 4) & 0xF);
            switch (m_arm_lut[opcode]) {
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
            case InstrFormat::NOP: {
                printf("found: %08X\n", opcode);
                std::exit(1);
                return 1;
            }
            default: std::unreachable();
            }
        }
    }
    return 1;
}

void CPU::dump_state() {
    for (int i = 0; i < 16; i++) {
        if (m_regs[i]) {
            printf("%d: 0x%08X\n", i, m_regs[i]);
        }
    }
    printf("cpsr: %08X\n", get_cpsr());
    printf("psr: %08X\n", get_psr());
}

std::array<std::array<std::uint16_t, 240>, 160>& CPU::render_frame() {
    int cycles = 0;
    while (cycles < cycles_per_frame) {
        int instr_cycles = execute();
        m_mem.tick_components(instr_cycles);
        cycles += instr_cycles;
    }
    return m_mem.get_frame();
}

#include "cpu.hpp"

#include <algorithm>
#include <iostream>
#include <utility>

static const int CYCLES_PER_FRAME = 280896;

CPU::CPU(const std::string& rom_filepath) 
    : m_pipeline(0), m_pipeline_invalid(true), m_mode(SYS)
{
    m_mem.load_bios();
    m_mem.load_rom(rom_filepath);
    initialize_registers();
}

void CPU::initialize_registers() 
{
    m_banked_regs[SYS].m_control = 0x1F;
    m_banked_regs[SYS][13] = 0x03007F00;
    m_banked_regs[SYS][14] = 0x08000000;
    m_banked_regs[SYS][15] = 0x08000000;
    m_banked_regs[SVC][13] = 0x03007FE0;
    m_banked_regs[IRQ][13] = 0x03007FA0;
}

CPU::Registers& CPU::get_bank_from_mode_bits(std::uint8_t mode_bits) 
{
    switch (mode_bits) 
    {
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

bool CPU::in_user_mode()
{
    return (m_banked_regs[SYS].m_control & 0x1F) == 0b010000;
}

bool CPU::is_irq_disabled()
{
    return (m_banked_regs[SYS].m_control >> 7) & 1;
}

bool CPU::is_thumb_enabled()
{
    return (m_banked_regs[SYS].m_control >> 5) & 1;
}

void CPU::update_cpsr_thumb_status(bool is_enabled)
{
    m_banked_regs[SYS].m_control = (m_banked_regs[SYS].m_control & ~(1 << 5)) | (static_cast<std::uint8_t>(is_enabled) << 5);
}

void CPU::update_cpsr_irq_disable(bool is_disabled)
{
    m_banked_regs[SYS].m_control = (m_banked_regs[SYS].m_control & ~(1 << 7)) | (static_cast<std::uint8_t>(is_disabled) << 7);
}

std::uint32_t CPU::get_psr()
{
    const auto& bank = m_banked_regs[m_mode];
    std::uint8_t flags = (bank.m_flags.n << 3) | (bank.m_flags.z << 2) | (bank.m_flags.c << 1) | bank.m_flags.v;
    return static_cast<std::uint32_t>(flags << 28) | bank.m_control;
}

std::uint32_t CPU::get_cpsr()
{
    const auto& bank = m_banked_regs[SYS];
    std::uint8_t flags = (bank.m_flags.n << 3) | (bank.m_flags.z << 2) | (bank.m_flags.c << 1) | bank.m_flags.v;
    return static_cast<std::uint32_t>(flags << 28) | bank.m_control;
}

void CPU::bank_transfer(std::uint8_t mode_bits)
{
    m_banked_regs[SYS].m_control = (m_banked_regs[SYS].m_control & ~0x1F) | mode_bits;

    for (int bank = 0; bank < m_banked_regs.size(); bank++)
    {
        for (int reg = 0; reg <= 7; reg++)
        {
            m_banked_regs[bank][reg] = m_banked_regs[m_mode][reg];
        }
        m_banked_regs[bank][15] = m_banked_regs[m_mode][15];
    }

    if (m_mode != FIQ)
    {
        for (int bank = 0; bank < m_banked_regs.size(); bank++)
        {
            if (bank != FIQ)
            {
                for (int reg = 8; reg <= 12; reg++)
                {
                    m_banked_regs[bank][reg] = m_banked_regs[m_mode][reg];
                }
            }
        }
    }

    switch (mode_bits)
    {
    case 0b10000:
    case 0b11111:
        m_mode = SYS;
        break;
    case 0b10001:
        m_mode = FIQ;
        break;
    case 0b10010:
        m_mode = IRQ;
        break;
    case 0b10011:
        m_mode = SVC;
        break;
    case 0b10111:
        m_mode = ABT;
        break;
    case 0b11011:
        m_mode = UND;
        break;
    default: std::unreachable();
    }
}

std::uint32_t CPU::ror(std::uint32_t operand, std::size_t shift_amount) 
{
    return (operand >> (shift_amount & 31)) | (operand << ((-shift_amount) & 31));
}

void CPU::safe_reg_assign(std::uint8_t reg, std::uint32_t value) 
{
    m_banked_regs[m_mode][reg] = value;

    if (reg == 15)
    {
        m_pipeline_invalid = true;
        m_banked_regs[m_mode][15] &= ~(0b01 | (0b10 * !is_thumb_enabled()));
    }
}

bool CPU::condition(std::uint32_t instr) 
{
    const auto& bank = m_banked_regs[m_mode];    
    switch ((instr >> 28) & 0xF) 
    {
    case 0x0: return bank.m_flags.z;
    case 0x1: return !bank.m_flags.z;
    case 0x2: return bank.m_flags.c;
    case 0x3: return !bank.m_flags.c;
    case 0x4: return bank.m_flags.n;
    case 0x5: return !bank.m_flags.n;
    case 0x6: return bank.m_flags.v;
    case 0x7: return !bank.m_flags.v;
    case 0x8: return bank.m_flags.c & !bank.m_flags.z;
    case 0x9: return !bank.m_flags.c | bank.m_flags.z;
    case 0xA: return bank.m_flags.n == bank.m_flags.v;
    case 0xB: return bank.m_flags.n ^ bank.m_flags.v;
    case 0xC: return !bank.m_flags.z && (bank.m_flags.n == bank.m_flags.v);
    case 0xD: return bank.m_flags.z || (bank.m_flags.n ^ bank.m_flags.v);
    case 0xE: return true;
    default: std::unreachable();
    }
}

std::uint32_t CPU::fetch_arm() 
{
    auto instr = m_mem.read<std::uint32_t>(m_banked_regs[m_mode][15]);
    m_banked_regs[m_mode][15] += 4;
    return instr;
}

std::uint16_t CPU::fetch_thumb() 
{
    auto instr = m_mem.read<std::uint16_t>(m_banked_regs[m_mode][15]);
    m_banked_regs[m_mode][15] += 2;
    return instr;
}

void CPU::barrel_shifter(
    std::uint32_t& operand,
    bool& carry_out,
    ShiftType shift_type,
    std::uint8_t shift_amount, 
    bool reg_imm_shift
) 
{
    switch (shift_type) 
    {
    case ShiftType::LSL: 
    {
        bool nonzero_shift = shift_amount != 0;
        bool computed_carry = (operand >> (nonzero_shift * (32 - (shift_amount | !nonzero_shift)))) & 1;
        carry_out = (nonzero_shift * computed_carry) + (!nonzero_shift * carry_out);
        operand = !(shift_amount > 31) * (operand << (shift_amount & 31));
        break;
    }
    case ShiftType::LSR: 
    {
        std::uint32_t adjusted_shift = (((shift_amount != 0) * (shift_amount - 1)) + ((shift_amount == 0) * 31));
        carry_out = ((operand >> (adjusted_shift & 31)) & 1) * !(shift_amount > 32);
        operand = ((operand >> adjusted_shift) * !(shift_amount > 32)) >> 1;
        break;
    }
    case ShiftType::ASR: 
    {
        bool toggle = (shift_amount == 0) | (shift_amount > 31);
        std::uint32_t adjusted_shift = ((!toggle * (shift_amount - 1)) + (toggle * 31));
        carry_out = (operand >> adjusted_shift) & 1;
        operand = (static_cast<std::int32_t>(operand) >> adjusted_shift) >> !toggle;
        break;
    }
    case ShiftType::ROR:
        if (reg_imm_shift && !shift_amount) 
        {
            carry_out = operand & 1;
            operand = (static_cast<std::uint32_t>(m_banked_regs[m_mode].m_flags.c) << 31) | (operand >> 1);
        } 
        else 
        {
            operand = ror(operand, shift_amount);
            carry_out = operand >> 31;
        }
        break;
    }
}

int CPU::branch(std::uint32_t instr) 
{
    bool bl = (instr >> 24) & 1;
    if (bl) 
    {
        m_banked_regs[m_mode][14] = m_banked_regs[m_mode][15] - 4;
    }

    std::int32_t nn = ((static_cast<std::int32_t>((instr & 0xFFFFFF) << 8) >> 8) << 2) >> is_thumb_enabled();
    m_banked_regs[m_mode][15] += nn;
    m_pipeline_invalid = true;
    return 1;
}

int CPU::branch_ex(std::uint32_t instr) 
{
    std::uint8_t rn = instr & 0xF;
    update_cpsr_thumb_status(m_banked_regs[m_mode][rn] & 1);
    m_banked_regs[m_mode][15] = m_banked_regs[m_mode][rn] & ~(0b1 | (!is_thumb_enabled() * 0b10));
    m_pipeline_invalid = true;
    return 1;
}

int CPU::single_transfer(std::uint32_t instr) 
{
    bool i = (instr >> 25) & 1;
    bool p = (instr >> 24) & 1;
    bool u = (instr >> 23) & 1;
    bool b = (instr >> 22) & 1;
    bool w = (instr >> 21) & 1;
    bool l = (instr >> 20) & 1;
    std::uint8_t rn = (instr >> 16) & 0xF;
    std::uint8_t rd = (instr >> 12) & 0xF;
    std::uint32_t offset = 0;

    if (i) 
    {
        std::uint8_t rm = instr & 0xF;
        std::uint32_t operand = m_banked_regs[m_mode][rm];
        std::uint8_t shift_amount = (instr >> 7) & 0x1F;
        ShiftType shift_type = static_cast<ShiftType>((instr >> 5) & 3);

        bool carry_out = false;
        barrel_shifter(operand, carry_out, shift_type, shift_amount, true);
        offset = operand;
    } 
    else 
    {
        offset = instr & 0xFFF;
    }
    offset *= (-1 + (u * 2));

    std::uint32_t addr = m_banked_regs[m_mode][rn] + (p ? offset : 0);
    bool writeback = !p || (p && w);

    if (l) 
    {
        safe_reg_assign(rd, b ? m_mem.read<std::uint8_t>(addr) : ror(m_mem.read<std::uint32_t>(addr), (addr & 0x3) * 8));
    } 
    else 
    {
        std::uint32_t value = m_banked_regs[m_mode][rd] + ((rd == 0xF) * 4);
        if (b) 
        {
            m_mem.write<std::uint8_t>(addr, value);
        } 
        else 
        {
            m_mem.write<std::uint32_t>(addr, value);
        }
    }

    if (writeback && (!l || !(rn == rd))) 
    {
        safe_reg_assign(rn, m_banked_regs[m_mode][rn] + (((rd == 0xF) * 4) + offset));
    }
    return 1;
}

int CPU::halfword_transfer(std::uint32_t instr) 
{
    bool p = (instr >> 24) & 1;
    bool u = (instr >> 23) & 1;
    bool i = (instr >> 22) & 1;
    bool w = (instr >> 21) & 1;
    bool l = (instr >> 20) & 1;
    auto rn = (instr >> 16) & 0xF;
    auto rd = (instr >> 12) & 0xF;

    std::int32_t offset = (i ? ((((instr >> 8) & 0xF) << 4) | (instr & 0xF)) : 
        m_banked_regs[m_mode][instr & 0xF]) * (-1 + (u * 2));

    auto opcode = (instr >> 5) & 0x3;
    auto addr = m_banked_regs[m_mode][rn] + (p * offset);
    bool writeback = (p && w) || !p;

    if (l) 
    {
        bool misaligned_read = addr & 1;
        switch (opcode) 
        {
        case 1: 
        {
            safe_reg_assign(rd, misaligned_read ? 
                ror(m_mem.read<std::uint16_t>(addr - 1), 8) : m_mem.read<std::uint16_t>(addr));
            break;
        }
        case 2: 
        {
            safe_reg_assign(rd, static_cast<std::int32_t>(static_cast<std::int8_t>(m_mem.read<std::uint8_t>(addr))));
            break;
        }
        case 3: 
        {
            std::uint32_t value = misaligned_read ? static_cast<std::int32_t>(static_cast<std::int8_t>(m_mem.read<std::uint8_t>(addr)))
                : static_cast<std::int32_t>(static_cast<std::int16_t>(m_mem.read<std::uint16_t>(addr)));
            safe_reg_assign(rd, value);
            break;
        }
        }
    } 
    else 
    {
        m_mem.write<std::uint16_t>(addr, m_banked_regs[m_mode][rd]);
    }

    if (writeback && (!l || !(rn == rd))) 
    {
        safe_reg_assign(rn, m_banked_regs[m_mode][rn] + (offset + ((rn == 0xF) * 4)));
    }
    return 1;
}

int CPU::block_transfer(std::uint32_t instr) 
{
    bool p = (instr >> 24) & 1;
    bool u = (instr >> 23) & 1;
    bool s = (instr >> 22) & 1;
    bool w = (instr >> 21) & 1;
    bool l = (instr >> 20) & 1;
    std::uint8_t rn = (instr >> 16) & 0xF;
    std::uint16_t reg_list = instr & 0xFFFF;

    int first_transfer = __builtin_ffs(reg_list) - 1;
    int transfers = __builtin_popcount(reg_list);
    std::uint32_t transfer_base_addr = m_banked_regs[m_mode][rn];
    std::uint32_t direction = 4 - (!u * 8);

    std::uint8_t prev_mode = 0;
    if (s)
    {
        if (l && ((reg_list >> 15) & 1)) 
        {
            m_banked_regs[SYS].m_flags = m_banked_regs[m_mode].m_flags;
            update_cpsr_thumb_status((m_banked_regs[m_mode].m_control >> 5) & 1);
            update_cpsr_irq_disable((m_banked_regs[m_mode].m_control >> 7) & 1);
            bank_transfer(m_banked_regs[m_mode].m_control & 0x1F);
        }
        else
        {
            prev_mode = m_banked_regs[SYS].m_control & 0x1F;
            bank_transfer(0b11111);
        }
    }

    if (transfers == 0) 
    {
        if (l) 
        {
            m_banked_regs[m_mode][15] = m_mem.read<std::uint32_t>(transfer_base_addr);
            m_pipeline_invalid = true;
        } 
        else 
        {
            std::uint32_t addr = u ? transfer_base_addr + (p * 4) : ((transfer_base_addr + (16 * direction)) + (!p * 4));
            m_mem.write<std::uint32_t>(addr, m_banked_regs[m_mode][15] + (4 >> is_thumb_enabled()));
        }
        m_banked_regs[m_mode][rn] += (16 * direction);
        return 1;
    } 
    else if (w) 
    {
        m_banked_regs[m_mode][rn] += (transfers * direction);
    }

    int reg_start, reg_end, step;
    if (u) 
    {
        reg_start = first_transfer;
        reg_end = 0x10;
        step = 1;
    } 
    else 
    {
        reg_start = 31 - __builtin_clz(reg_list);
        reg_end = -1;
        step = -1;
    }

    if (l) 
    {
        for (int i = reg_start; i != reg_end; i += step)
            if ((reg_list >> i) & 1) 
            {
                std::uint32_t addr = transfer_base_addr + (p * direction);
                safe_reg_assign(i, m_mem.read<std::uint32_t>(addr));
                transfer_base_addr += direction;
            }
    } 
    else
    {
        std::uint32_t transfer_base_addr_copy = transfer_base_addr;
        for (int i = reg_start; i != reg_end; i += step)
            if ((reg_list >> i) & 1) 
            {
                std::uint32_t addr = transfer_base_addr + (p * direction);
                if ((first_transfer == i) && (i == rn)) {
                    m_mem.write<std::uint32_t>(addr, transfer_base_addr_copy);
                } else {
                    m_mem.write<std::uint32_t>(addr, m_banked_regs[m_mode][i] + ((i == 0xF) * (4 >> is_thumb_enabled())));
                }
                transfer_base_addr += direction;
            }
    }

    if (prev_mode) 
    {

        bank_transfer(prev_mode);
    }
    return 1;
}

int CPU::mrs(std::uint32_t instr) 
{
    std::uint8_t rd = (instr >> 12) & 0xF;
    bool psr = (instr >> 22) & 1;
    if (psr) 
    {
        m_banked_regs[m_mode][rd] = get_psr();
    } 
    else 
    {
        m_banked_regs[m_mode][rd] = get_cpsr();
    }
    return 1;
}

int CPU::msr(std::uint32_t instr) 
{
    bool i = (instr >> 25) & 1;
    bool psr = (instr >> 22) & 1;
    bool f = (instr >> 19) & 1;
    bool c = (instr >> 16) & 1;

    std::uint8_t shift_amount = ((instr >> 8) & 0xF) * 2;
    auto imm = instr & 0xFF;
    
    std::uint32_t operand = i ? ror(imm, shift_amount) : m_banked_regs[m_mode][instr & 0xF];
    std::uint32_t flags = operand & 0xFF000000;
    std::uint32_t control = operand & 0x000000FF;

    if (psr) 
    {
        if (f) 
        {
            m_banked_regs[m_mode].m_flags.n = flags >> 31;
            m_banked_regs[m_mode].m_flags.z = (flags >> 30) & 1;
            m_banked_regs[m_mode].m_flags.c = (flags >> 29) & 1;
            m_banked_regs[m_mode].m_flags.v = (flags >> 28) & 1;
        }
        if (c && !in_user_mode())
        {
            if (m_mode == SYS)
            {
                update_cpsr_irq_disable((control >> 7) & 1);
                bank_transfer(control & 0x1F);
            }
            else
            {
                m_banked_regs[m_mode].m_control = control;
            }
        }
    }
    else
    {
        if (f)
        {
            m_banked_regs[SYS].m_flags.n = flags >> 31;
            m_banked_regs[SYS].m_flags.z = (flags >> 30) & 1;
            m_banked_regs[SYS].m_flags.c = (flags >> 29) & 1;
            m_banked_regs[SYS].m_flags.v = (flags >> 28) & 1;
        }
        if (c && !in_user_mode()) 
        {
            update_cpsr_irq_disable((control >> 7) & 1);
            bank_transfer(control & 0x1F);
        }
    }
    return 1;
}

int CPU::swi(std::uint32_t instr)
{
    m_banked_regs[SVC][14] = m_banked_regs[m_mode][15] - (4 >> is_thumb_enabled());
    m_banked_regs[SVC].m_flags = m_banked_regs[SYS].m_flags;
    m_banked_regs[SVC].m_control = m_banked_regs[SYS].m_control;
    update_cpsr_thumb_status(false);
    update_cpsr_irq_disable(true);
    bank_transfer(0b10011);
    m_banked_regs[m_mode][15] = 0x00000008;
    m_pipeline_invalid = true;
    return 1;
}

int CPU::swp(std::uint32_t instr) 
{
    bool b = (instr >> 22) & 1;
    std::uint8_t rn = (instr >> 16) & 0xF;
    std::uint8_t rd = (instr >> 12) & 0xF;
    std::uint8_t rm = instr & 0xF;

    if (b) 
    {
        std::uint32_t value = m_mem.read<std::uint8_t>(m_banked_regs[m_mode][rn]);
        m_mem.write<std::uint8_t>(m_banked_regs[m_mode][rn], m_banked_regs[m_mode][rm]);
        safe_reg_assign(rd, value);
    } 
    else 
    {
        std::uint32_t value = ror(m_mem.read<std::uint32_t>(m_banked_regs[m_mode][rn]), (m_banked_regs[m_mode][rn] & 0x3) * 8);
        m_mem.write<std::uint32_t>(m_banked_regs[m_mode][rn], m_banked_regs[m_mode][rm]);
        safe_reg_assign(rd, value);
    }
    return 1;
}

void CPU::get_alu_operands(
    std::uint32_t instr, 
    std::uint32_t& op1, 
    std::uint32_t& op2, 
    bool& carry_out
) 
{
    bool imm = (instr >> 25) & 1;
    auto rn = (instr >> 16) & 0xF;
    op1 = m_banked_regs[m_mode][rn];

    if (imm) 
    {
        std::uint8_t imm_shift = ((instr >> 8) & 0xF) * 2; 
        op2 = instr & 0xFF;
        if (imm_shift)
            barrel_shifter(op2, carry_out, ShiftType::ROR, imm_shift, false);
    } 
    else 
    {
        bool r = (instr >> 4) & 1;
        ShiftType shift_type = static_cast<ShiftType>((instr >> 5) & 0x3);
        std::uint8_t rm = instr & 0xF;

        op2 = m_banked_regs[m_mode][rm];
        if (r) {
            if (rn == 0xF) op1 = m_banked_regs[m_mode][15] + (4 >> is_thumb_enabled());
            if (rm == 0xF) op2 = m_banked_regs[m_mode][15] + (4 >> is_thumb_enabled());
            std::uint8_t shift_amount = m_banked_regs[m_mode][(instr >> 8) & 0xF] & 0xFF;
            if (shift_amount) 
                barrel_shifter(op2, carry_out, shift_type, shift_amount, false);
        } else {
            std::uint8_t shift_amount = (instr >> 7) & 0x1F;
            barrel_shifter(op2, carry_out, shift_type, shift_amount, true);
        }
    }
}

int CPU::alu(std::uint32_t instr) 
{
    bool set_cc = (instr >> 20) & 1;
    auto rd = (instr >> 12) & 0xF;

    std::uint32_t op1 = 0;
    std::uint32_t op2 = 0;
    bool carry_out = m_banked_regs[m_mode].m_flags.c;
    get_alu_operands(instr, op1, op2, carry_out);

    if ((rd == 15) && set_cc) 
    {
        m_banked_regs[SYS].m_flags = m_banked_regs[m_mode].m_flags;
        update_cpsr_thumb_status((m_banked_regs[m_mode].m_control >> 5) & 1);
        update_cpsr_irq_disable((m_banked_regs[m_mode].m_control >> 7) & 1);
        bank_transfer(m_banked_regs[m_mode].m_control & 0x1F);
    }

    switch ((instr >> 21) & 0xF) 
    {
    case 0x0: 
    {
        std::uint32_t result = op1 & op2;
        if (set_cc) 
        {
            m_banked_regs[m_mode].m_flags.n = result >> 31;
            m_banked_regs[m_mode].m_flags.z = !result;
            m_banked_regs[m_mode].m_flags.c = carry_out;
        }
        safe_reg_assign(rd, result);
        break;
    }
    case 0x1: 
    {
        std::uint32_t result = op1 ^ op2;
        if (set_cc) 
        {
            m_banked_regs[m_mode].m_flags.n = result >> 31;
            m_banked_regs[m_mode].m_flags.z = !result;
            m_banked_regs[m_mode].m_flags.c = carry_out;
        }
        safe_reg_assign(rd, result);
        break;
    }
    case 0x3: // RSB
    {
        std::uint32_t temp = op1;
        op1 = op2;
        op2 = temp;
    }
    case 0x2: // SUB
    {
        std::uint32_t result = op1 - op2;
        if (set_cc) 
        {
            m_banked_regs[m_mode].m_flags.n = result >> 31;
            m_banked_regs[m_mode].m_flags.z = !result;
            m_banked_regs[m_mode].m_flags.c = op1 >= op2;
            m_banked_regs[m_mode].m_flags.v = ((op1 >> 31) != (op2 >> 31)) && ((op1 >> 31) != (result >> 31));
        }
        safe_reg_assign(rd, result);
        break;
    }
    case 0x4: // ADD
    {
        std::uint32_t result = op1 + op2;
        if (set_cc) 
        {
            m_banked_regs[m_mode].m_flags.n = result >> 31;
            m_banked_regs[m_mode].m_flags.z = result == 0;
            m_banked_regs[m_mode].m_flags.c = ((op1 >> 31) + (op2 >> 31) > (result >> 31));
            m_banked_regs[m_mode].m_flags.v = ((op1 >> 31) == (op2 >> 31)) && ((op1 >> 31) != (result >> 31));
        }
        safe_reg_assign(rd, result);
        break;
    }
    case 0x5: 
    {
        std::uint32_t result = op1 + op2 + m_banked_regs[m_mode].m_flags.c;
        if (set_cc) 
        {
            m_banked_regs[m_mode].m_flags.n = result >> 31;
            m_banked_regs[m_mode].m_flags.z = !result;
            m_banked_regs[m_mode].m_flags.c = ((op1 >> 31) + (op2 >> 31) > (result >> 31));
            m_banked_regs[m_mode].m_flags.v = ((op1 >> 31) == (op2 >> 31)) && ((op1 >> 31) != (result >> 31));
        }
        safe_reg_assign(rd, result);
        break;
    }
    case 0x7: // RSC
    { 
        std::uint32_t temp = op1;
        op1 = op2;
        op2 = temp;
    }
    case 0x6: 
    {
        std::uint32_t result = op1 - op2 - !m_banked_regs[m_mode].m_flags.c;
        if (set_cc) 
        {
            m_banked_regs[m_mode].m_flags.n = result >> 31;
            m_banked_regs[m_mode].m_flags.z = !result;
            m_banked_regs[m_mode].m_flags.c = static_cast<std::uint64_t>(op1) >= (static_cast<std::uint64_t>(op2) + !m_banked_regs[m_mode].m_flags.c);
            m_banked_regs[m_mode].m_flags.v = ((op1 >> 31) != (op2 >> 31)) && ((op1 >> 31) != (result >> 31));
        }
        safe_reg_assign(rd, result);
        break;
    }
    case 0x8: 
    {
        std::uint32_t result = op1 & op2;
        m_banked_regs[m_mode].m_flags.n = result >> 31;
        m_banked_regs[m_mode].m_flags.z = !result;
        m_banked_regs[m_mode].m_flags.c = carry_out;
        break;
    }
    case 0x9: // TEQ
    { 
        std::uint32_t result = op1 ^ op2;
        m_banked_regs[m_mode].m_flags.n = result >> 31;
        m_banked_regs[m_mode].m_flags.z = !result;
        m_banked_regs[m_mode].m_flags.c = carry_out;
        break;
    }
    case 0xA: // CMP
    { 
        std::uint32_t result = op1 - op2;
        m_banked_regs[m_mode].m_flags.n = result >> 31;
        m_banked_regs[m_mode].m_flags.z = result == 0;
        m_banked_regs[m_mode].m_flags.c = op1 >= op2;
        m_banked_regs[m_mode].m_flags.v = ((op1 >> 31) != (op2 >> 31)) && ((op1 >> 31) != (result >> 31));
        break;
    }
    case 0xB: // CMN
    { 
        std::uint32_t result = op1 + op2;
        m_banked_regs[m_mode].m_flags.n = result >> 31;
        m_banked_regs[m_mode].m_flags.z = !result;
        m_banked_regs[m_mode].m_flags.c = ((op1 >> 31) + (op2 >> 31) > (result >> 31));
        m_banked_regs[m_mode].m_flags.v = ((op1 >> 31) == (op2 >> 31)) && ((op1 >> 31) != (result >> 31));
        break;
    }
    case 0xC: // ORR
    { 
        std::uint32_t result = op1 | op2;
        if (set_cc) 
        {
            m_banked_regs[m_mode].m_flags.n = result >> 31;
            m_banked_regs[m_mode].m_flags.z = !result;
            m_banked_regs[m_mode].m_flags.c = carry_out;
        }
        safe_reg_assign(rd, result);
        break;
    }
    case 0xE: // BIC
    { 
        std::uint32_t result = op1 & ~op2;
        if (set_cc) 
        {
            m_banked_regs[m_mode].m_flags.n = result >> 31;
            m_banked_regs[m_mode].m_flags.z = !result;
            m_banked_regs[m_mode].m_flags.c = carry_out;
        } 
        safe_reg_assign(rd, result);
        break;
    }
    case 0xF: // MVN
        op2 = ~op2;
    case 0xD: // MOV
        if (set_cc) 
        {
            m_banked_regs[m_mode].m_flags.n = op2 >> 31;
            m_banked_regs[m_mode].m_flags.z = !op2;
            m_banked_regs[m_mode].m_flags.c = carry_out;
        }
        safe_reg_assign(rd, op2);
        break;
    }

    return 1;
}

int CPU::mul(std::uint32_t instr) 
{
    bool s = (instr >> 20) & 1;
    std::uint8_t rd = (instr >> 16) & 0xF;
    std::uint8_t rn = (instr >> 12) & 0xF;
    std::uint8_t rs = (instr >> 8) & 0xF;
    std::uint8_t rm = instr & 0xF;

    switch ((instr >> 21) & 0xF) 
    {
    case 0x0: 
    {
        std::uint32_t result = m_banked_regs[m_mode][rm] * m_banked_regs[m_mode][rs];
        if (s) 
        {
            m_banked_regs[m_mode].m_flags.n = result >> 31;
            m_banked_regs[m_mode].m_flags.z = !result;
        }
        safe_reg_assign(rd, result);
        break;
    }
    case 0x1: 
    {
        std::uint32_t result = (m_banked_regs[m_mode][rm] * m_banked_regs[m_mode][rs]) + m_banked_regs[m_mode][rn];
        if (s) 
        {
            m_banked_regs[m_mode].m_flags.n = result >> 31;
            m_banked_regs[m_mode].m_flags.z = !result;
        }
        safe_reg_assign(rd, result);
        break;
    }
    case 0x4: 
    {
        std::uint64_t result = static_cast<std::uint64_t>(m_banked_regs[m_mode][rm]) * m_banked_regs[m_mode][rs];
        if (s) 
        {
            m_banked_regs[m_mode].m_flags.n = result >> 63;
            m_banked_regs[m_mode].m_flags.z = !result;
        }
        safe_reg_assign(rn, result);
        safe_reg_assign(rd, result >> 32);
        break;
    }
    case 0x5: 
    {
        std::uint64_t result = static_cast<std::uint64_t>(m_banked_regs[m_mode][rm]) * m_banked_regs[m_mode][rs] 
            + ((static_cast<std::uint64_t>(m_banked_regs[m_mode][rd]) << 32) | m_banked_regs[m_mode][rn]);
        if (s) 
        {
            m_banked_regs[m_mode].m_flags.n = result >> 63;
            m_banked_regs[m_mode].m_flags.z = !result;
        }
        safe_reg_assign(rn, result);
        safe_reg_assign(rd, result >> 32);
        break;
    }
    case 0x6: 
    {
        std::int64_t result = static_cast<std::int64_t>(static_cast<std::int32_t>(m_banked_regs[m_mode][rm])) 
            * static_cast<std::int64_t>(static_cast<std::int32_t>(m_banked_regs[m_mode][rs]));
        if (s) 
        {
            m_banked_regs[m_mode].m_flags.n = result >> 63;
            m_banked_regs[m_mode].m_flags.z = !result;
        }
        safe_reg_assign(rn, result);
        safe_reg_assign(rd, result >> 32);
        break;
    }
    case 0x7: 
    {
        std::int64_t result = static_cast<std::int64_t>(static_cast<std::int32_t>(m_banked_regs[m_mode][rm])) 
            * static_cast<std::int64_t>(static_cast<std::int32_t>(m_banked_regs[m_mode][rs]))
                + (static_cast<std::int64_t>((static_cast<std::uint64_t>(m_banked_regs[m_mode][rd]) << 32) ) | m_banked_regs[m_mode][rn]);
        if (s) 
        {
            m_banked_regs[m_mode].m_flags.n = result >> 63;
            m_banked_regs[m_mode].m_flags.z = !result;
        }
        safe_reg_assign(rn, result);
        safe_reg_assign(rd, result >> 32);
        break;
    }
    default: std::unreachable();
    }
    return 1;
}

int CPU::execute()
{
    if (is_thumb_enabled()) 
    {
        std::uint16_t instr = m_pipeline_invalid ? fetch_thumb() : m_pipeline;
        m_pipeline = fetch_thumb();
        m_pipeline_invalid = false;
        switch (m_thumb_lut[instr >> 6]) 
        {
        case InstrFormat::THUMB_1: return alu(thumb_translate_1(instr));
        case InstrFormat::THUMB_2: return alu(thumb_translate_2(instr));
        case InstrFormat::THUMB_3: return alu(thumb_translate_3(instr));
        case InstrFormat::THUMB_4: {
            std::uint32_t translation = 0b11100000000100000000000000000000;
            std::uint32_t rd = instr & 0x7;
            std::uint32_t rs = (instr >> 3) & 0x7;
            std::uint32_t arm_opcode = (instr >> 6) & 0xF;
            ShiftType shift_type = ShiftType::LSL;

            translation |= (rd << 12);

            switch ((instr >> 6) & 0xF) {
            case 0x2:
                goto thumb_shift_instr;
            case 0x3:
                shift_type = ShiftType::LSR;
                goto thumb_shift_instr;
            case 0x4:
                shift_type = ShiftType::ASR;
                goto thumb_shift_instr;
            case 0x7:
                shift_type = ShiftType::ROR;
                goto thumb_shift_instr;
            case 0x9:
                translation |= (0x3 << 21);
                translation |= (0x1 << 25);
                translation |= (rs << 16);
                return alu(translation);
            case 0xD:
                translation |= (rd << 16);
                translation |= rs;
                translation |= (rd << 8);
                return mul(translation);
            }

            translation |= rs;

            complete_translation:
                translation |= (arm_opcode << 21);
                translation |= (rd << 16);
                translation |= (static_cast<std::uint32_t>(std::to_underlying(shift_type)) << 5);
                return alu(translation);

            thumb_shift_instr:
                arm_opcode = 0xD;
                translation |= (0x1 << 4);
                translation |= (rs << 8);
                translation |= rd;
                goto complete_translation;

            std::unreachable();
        }
        case InstrFormat::THUMB_5_ALU: return alu(thumb_translate_5_alu(instr));
        case InstrFormat::THUMB_5_BX: return branch_ex(thumb_translate_5_bx(instr));
        case InstrFormat::THUMB_6: 
        {
            std::uint8_t rd = (instr >> 8) & 0x7;
            std::uint16_t nn = (instr & 0xFF) << 2;
            m_banked_regs[m_mode][rd] = m_mem.read<std::uint32_t>((m_banked_regs[m_mode][15] & ~2) + nn);
            return 1;
        }
        case InstrFormat::THUMB_7: return single_transfer(thumb_translate_7(instr));
        case InstrFormat::THUMB_8: return halfword_transfer(thumb_translate_8(instr));
        case InstrFormat::THUMB_9: return single_transfer(thumb_translate_9(instr));
        case InstrFormat::THUMB_10: return halfword_transfer(thumb_translate_10(instr));
        case InstrFormat::THUMB_11: return single_transfer(thumb_translate_11(instr));
        case InstrFormat::THUMB_12: 
        {
            std::uint8_t rd = (instr >> 8) & 0x7;
            std::uint32_t nn = (instr & 0xFF) << 2;
            if ((instr >> 11) & 1) {
                m_banked_regs[m_mode][rd] = m_banked_regs[m_mode][13] + nn;
            } else {
                m_banked_regs[m_mode][rd] = (m_banked_regs[m_mode][15] & ~2) + nn;
            }
            return 1;
        }
        case InstrFormat::THUMB_13: return alu(thumb_translate_13(instr));
        case InstrFormat::THUMB_14: return block_transfer(thumb_translate_14(instr));
        case InstrFormat::THUMB_15: return block_transfer(thumb_translate_15(instr));
        case InstrFormat::THUMB_16: 
        {
            std::uint32_t arm_instr = thumb_translate_16(instr);
            if (condition(arm_instr)) {
                return branch(arm_instr);
            }
            return 1;
        }
        case InstrFormat::THUMB_17: return swi(thumb_translate_17(instr));
        case InstrFormat::THUMB_18: return branch(thumb_translate_18(instr));
        case InstrFormat::THUMB_19_PREFIX: 
        {
            std::uint32_t upper_half_offset = static_cast<std::int32_t>((instr & 0x7FF) << 21) >> 21;
            m_banked_regs[m_mode][14] = m_banked_regs[m_mode][15] + (upper_half_offset << 12);
            return 1;
        }
        case InstrFormat::THUMB_19_SUFFIX: 
        {
            std::uint32_t lower_half_offset = instr & 0x7FF;
            std::uint32_t curr_pc = m_banked_regs[m_mode][15];
            m_banked_regs[m_mode][15] = m_banked_regs[m_mode][14] + (lower_half_offset << 1);
            m_banked_regs[m_mode][14] = (curr_pc - 2) | 1;
            m_pipeline_invalid = true;
            return 1;
        }
        case InstrFormat::NOP: return 1;
        default: std::unreachable();
        }
    } 
    else 
    {
        std::uint32_t instr = m_pipeline_invalid ? fetch_arm() : m_pipeline;
        m_pipeline = fetch_arm();
        m_pipeline_invalid = false;

        if (condition(instr)) [[likely]] 
        {
            std::uint16_t opcode = (((instr >> 20) & 0xFF) << 4) | ((instr >> 4) & 0xF);
            switch (m_arm_lut[opcode]) 
            {
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
            default: std::unreachable();
            }
        }
        return 1;
    }
}

void CPU::reset()
{
    printf("reset\n");
    exit(1);
    // m_banked_regs = std::array<Registers, 6>{};
    // m_regs = m_banked_regs[SYS];
    // m_pipeline = 0;
    // m_pipeline_invalid = true;
    // is_thumb_enabled() = false;
    // initialize_registers();
    // m_mem.reset_components();
}

FrameBuffer& CPU::view_current_frame() 
{
    return m_mem.get_frame();
}

int CPU::step() 
{
    int cycles = 1;

    if (!m_pipeline_invalid && !is_irq_disabled() && m_mem.pending_interrupts())
    {
        m_banked_regs[IRQ][14] = m_banked_regs[m_mode][15] - (4 >> is_thumb_enabled());
        m_banked_regs[IRQ].m_control = m_banked_regs[SYS].m_control;
        m_banked_regs[IRQ].m_flags = m_banked_regs[SYS].m_flags;
        update_cpsr_thumb_status(false);
        bank_transfer(0b10010);
        m_banked_regs[m_mode][15] = 0x00000018;
        m_pipeline_invalid = true;
    }

    cycles = execute();
    m_mem.tick_components(cycles);

    return cycles;
}

FrameBuffer& CPU::render_frame(std::uint16_t key_input, std::uint32_t breakpoint, bool& breakpoint_reached) 
{
    m_mem.m_key_input = key_input;

    int total_cycles = 0;
    while (total_cycles < CYCLES_PER_FRAME) 
    {
        if (breakpoint == m_banked_regs[m_mode][15]) [[unlikely]] 
        {
            breakpoint_reached = true;
            return m_mem.get_frame();
        }
        total_cycles += step();
    }
    return m_mem.get_frame();
}

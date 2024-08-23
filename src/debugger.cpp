#include "debugger.hpp"

#include "cpu.hpp"

Registers& Debugger::view_registers() {
    return m_cpu->m_regs;
}

void Debugger::decompile_arm_instr(Instr& instr) {
    std::uint16_t opcode = (((instr.opcode >> 20) & 0xFF) << 4) | ((instr.opcode >> 4) & 0xF);
    switch (m_cpu->m_arm_lut[opcode]) {
    default:
    }
}

void Debugger::decompile_thumb_instr(Instr& instr) {
    switch (m_cpu->m_thumb_lut[instr.opcode >> 6]) {
    default:
    }
}

std::array<Debugger::Instr, 64> Debugger::view_nearby_instructions() {
    std::array<Debugger::Instr, 64> instrs;

    std::uint32_t num_instrs = ((instrs.size() / 2) * (4 >> m_cpu->m_thumb_enabled));
    std::uint32_t start = m_cpu->m_regs[15] - num_instrs;
    std::uint32_t end = m_cpu->m_regs[15] + num_instrs;

    for (std::uint32_t addr = start; addr < end; addr += (4 >> m_cpu->m_thumb_enabled)) {
        int idx = (addr - start) / (4 >> m_cpu->m_thumb_enabled);
        instrs[idx].addr = addr;
        instrs[idx].opcode = m_cpu->m_thumb_enabled ? m_cpu->m_mem.read_halfword(addr) : m_cpu->m_mem.read_word(addr);
    }
    return instrs;
}

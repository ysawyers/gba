#include "debugger.hpp"

#include <format>

#include "cpu.hpp"

Registers& Debugger::view_registers() {
    return m_cpu->m_regs;
}

std::uint32_t Debugger::view_cpsr() {
    return m_cpu->get_cpsr();
}

std::uint32_t Debugger::view_psr() {
    return m_cpu->get_psr();
}

std::uint32_t Debugger::current_pc() {
    return m_cpu->m_regs[15] - ((4 >> m_cpu->m_thumb_enabled) * !m_cpu->m_pipeline_invalid);
}

const char* Debugger::condition(std::uint32_t instr) {
    switch ((instr >> 28) & 0xF) {
    case 0x0: return "EQ";
    case 0x1: return "NE";
    case 0x2: return "CS";
    case 0x3: return "CC";
    case 0x4: return "MI";
    case 0x5: return "PL";
    case 0x6: return "VS";
    case 0x7: return "VC";
    case 0x8: return "HI";
    case 0x9: return "LS";
    case 0xA: return "GE";
    case 0xB: return "LT";
    case 0xC: return "GT";
    case 0xD: return "LE";
    case 0xE: return "";
    default: std::unreachable();
    }
}

const char* Debugger::shift_type(std::uint8_t opcode) {
    switch (opcode) {
    case 0: return "LSL";
    case 1: return "LSR";
    case 2: return "ASR";
    case 3: return "ROR";
    default: std::unreachable();
    }
}

const char* Debugger::swi_subroutine(std::uint32_t nn) {
    switch (nn) {
    case 0x0: return "SoftReset";
    case 0x1: return "RegisterRamReset";
    case 0x2: return "Halt";
    case 0x3: return "Stop/Sleep";
    case 0x4: return "IntrWait";
    case 0x5: return "VBlankIntrWait";
    case 0x6: return "Div";
    case 0x7: return "DivArm";
    case 0x8: return "Sqrt";
    case 0x9: return "ArcTan";
    case 0xA: return "ArcTan2";
    case 0xB: return "CpuSet";
    case 0xC: return "CpuFastSet";
    case 0xD: return "GetBiosChecksum";
    case 0xE: return "BgAffineSet";
    case 0xF: return "ObjAffineSet";
    case 0x10: return "BitUnPack";
    default: return "Unknown";
    }
}

void Debugger::decompile_arm_instr(Instr& instr) {
    const char* cond = condition(instr.opcode);
    std::uint16_t opcode = (((instr.opcode >> 20) & 0xFF) << 4) | ((instr.opcode >> 4) & 0xF);
    switch (m_cpu->m_arm_lut[opcode]) {
    case CPU::InstrFormat::B: {
        bool l = (instr.opcode >> 24) & 1;
        std::int32_t nn = ((static_cast<std::int32_t>((instr.opcode & 0xFFFFFF) << 8) >> 8) << 2) >> m_cpu->m_thumb_enabled;
        instr.desc = std::format("B{}{} 0x{:08X}", 
            l ? "L" : "", cond, (instr.addr + (8 >> m_cpu->m_thumb_enabled)) + nn);
        break;
    }
    case CPU::InstrFormat::BX: {
        bool l = ((instr.opcode >> 4) & 0xF) == 0x1;
        std::uint8_t rn = instr.opcode & 0xF;
        instr.desc = std::format("B{}X{} 0x{:08X}",
            l ? "L" : "", cond, m_cpu->m_regs[rn] & ~2);
        break;
    }
    // case CPU::InstrFormat::ALU: {
    //     std::uint8_t rd = (instr.opcode >> 12) & 0xF;
    //     std::uint8_t rn = (instr.opcode >> 16) & 0xF;

    //     switch ((instr.opcode >> 21) & 0xF) {
    //     default:
    //         printf("unhandled %02X\n", (instr.opcode >> 21) & 0xF);
    //         std::exit(1);
    //     }
    //     break;
    // }
    default: break;
    }
}

void Debugger::decompile_thumb_instr(Instr& instr) {
    switch (m_cpu->m_thumb_lut[instr.opcode >> 6]) {
    case CPU::InstrFormat::THUMB_1: {
        std::uint8_t rd = instr.opcode & 7;
        std::uint8_t rs = (instr.opcode >> 3) & 7;
        std::uint32_t offset = (instr.opcode >> 6) & 0x1F;
        instr.desc = std::string(shift_type((instr.opcode >> 11) & 3)) + 'S' + ' '
            + std::format("r{}, r{}, 0x{:02X}", rd, rs, offset);
        break;
    }
    case CPU::InstrFormat::THUMB_17: {
        instr.desc = "SWI " + std::string(swi_subroutine(instr.opcode & 0xFF));
        break;
    }
    default: break;
    }
}

std::array<Debugger::Instr, 64> Debugger::view_nearby_instructions() {
    std::array<Debugger::Instr, 64> instrs{};

    std::int64_t num_instrs = ((instrs.size() / 2) * (4 >> m_cpu->m_thumb_enabled));
    std::int64_t start = m_cpu->m_regs[15] - num_instrs;
    std::int64_t end = m_cpu->m_regs[15] + num_instrs;

    for (std::int64_t addr = start; addr < end; addr += (4 >> m_cpu->m_thumb_enabled)) {
        if (addr < 0) continue;
        int idx = (addr - start) / (4 >> m_cpu->m_thumb_enabled);
        instrs[idx].addr = addr;
        if (m_cpu->m_thumb_enabled) {
            instrs[idx].opcode = m_cpu->m_mem.read_halfword(addr);
            decompile_thumb_instr(instrs[idx]);
        } else {
            instrs[idx].opcode = m_cpu->m_mem.read_word(addr);
            decompile_arm_instr(instrs[idx]);
        }
    }
    return instrs;
}

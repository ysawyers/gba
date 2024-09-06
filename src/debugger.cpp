#include "debugger.hpp"

#include <format>

#include "core/cpu.hpp"

CPU::Registers& Debugger::view_registers() {
    return m_cpu->m_regs;
}

std::uint32_t Debugger::view_cpsr() {
    return m_cpu->get_cpsr();
}

std::uint32_t Debugger::view_psr() {
    return m_cpu->get_psr();
}

// TODO: merge into single function
std::uint16_t Debugger::view_ie() {
    return m_cpu->m_mem.read<std::uint16_t>(0x04000200);
}
std::uint16_t Debugger::view_if() {
    return m_cpu->m_mem.read<std::uint16_t>(0x04000202);
}

std::uint32_t Debugger::current_pc() {
    return m_cpu->m_regs[15] - ((4 >> m_cpu->m_thumb_enabled) * !m_cpu->m_pipeline_invalid);
}

const char* Debugger::amod(std::uint8_t pu) {
    switch (pu) {
    case 0b11: return "IB";
    case 0b01: return "IA";
    case 0b10: return "DB";
    case 0b00: return "DA";
    default: std::unreachable();
    }
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
    case 0x11: return "LZ77UnCompReadNormalWrite8bit; Wram";
    case 0x12: return "LZ77UnCompReadNormalWrite16bit; Vram";
    case 0x13: return "HuffUnCompReadNormal";
    case 0x14: return "RLUnCompReadNormalWrite8bit; Wram";
    case 0x15: return "RLUnCompReadNormalWrite16bit; Vram";
    case 0x16: return "Diff8bitUnFilterWrite8bit; Wram";
    case 0x17: return "Diff8bitUnFilterWrite16bit; Vram";
    case 0x18: return "Diff16bitUnFilter";
    case 0x19: return "SoundBias";
    case 0x1A: return "SoundDriverInit";
    case 0x1B: return "SoundDriverMode";
    case 0x1C: return "SoundDriverMain";
    case 0x1D: return "SoundDriverVSync";
    case 0x1E: return "SoundChannelClear";
    case 0x1F: return "MidiKey2Freq";
    case 0x20: return "SoundWhatever0";
    case 0x21: return "SoundWhatever1";
    case 0x22: return "SoundWhatever2";
    case 0x23: return "SoundWhatever3";
    case 0x24: return "SoundWhatever4";
    case 0x25: return "MultiBoot";
    case 0x26: return "HardReset";
    case 0x27: return "CustomHalt";
    case 0x28: return "SoundDriverVSyncOff";
    case 0x29: return "SoundDriverVSyncOn";
    case 0x2A: return "SoundGetJumpList";
    default: std::unreachable();
    }
}

void Debugger::print_alu(Instr& instr) {
    const char* cond = condition(instr.opcode);
    const char* set_cc = ((instr.opcode >> 20) & 1) ? "S" : "";
    std::uint8_t rd = (instr.opcode >> 12) & 0xF;
    std::uint8_t rn = (instr.opcode >> 16) & 0xF;
    std::uint8_t rm = instr.opcode & 0xF;

    std::uint32_t op1 = 0;
    std::uint32_t op2 = 0;
    bool carry = false;
    m_cpu->get_alu_operands(instr.opcode, op1, op2, carry);

    switch ((instr.opcode >> 21) & 0xF) {
    case 0x0:
        instr.desc = std::format("AND{}{} r{},r{},",
            cond, set_cc, rd, rn);
        break;
    case 0x1:
        instr.desc = std::format("EOR{}{} r{},r{},",
            cond, set_cc, rd, rn);
        break;
    case 0x2:
        instr.desc = std::format("SUB{}{} r{},r{},",
            cond, set_cc, rd, rn);
        break;
    case 0x3:
        instr.desc = std::format("RSB{}{} r{},r{},",
            cond, set_cc, rd, rn);
        break;
    case 0x4:
        instr.desc = std::format("ADD{}{} r{},r{},",
            cond, set_cc, rd, rn);
        break;
    case 0x5:
        instr.desc = std::format("ADC{}{} r{},r{},",
            cond, set_cc, rd, rn);
        break;
    case 0x6:
        instr.desc = std::format("SBC{}{} r{},r{},",
            cond, set_cc, rd, rn);
        break;
    case 0x7:
        instr.desc = std::format("RSC{}{} r{},r{},",
            cond, set_cc, rd, rn);
        break;
    case 0x8:
        instr.desc = std::format("TST{} r{},",
            cond, rn);
        break;
    case 0x9:
        instr.desc = std::format("TEQ{} r{},",
            cond, rn);
        break;
    case 0xA:
        instr.desc = std::format("CMP{} r{},",
            cond, rn);
        break;
    case 0xB:
        instr.desc = std::format("CMN{} r{},",
            cond, rn);
        break;
    case 0xC:
        instr.desc = std::format("ORR{}{} r{},r{},",
            cond, set_cc, rd, rn);
        break;
    case 0xD:
        instr.desc = std::format("MOV{}{} r{},",
            cond, set_cc, rd);
        break;
    case 0xE:
        instr.desc = std::format("BIC{}{} r{},r{},",
            cond, set_cc, rd, rn);
        break;
    case 0xF:
        instr.desc = std::format("MVN{}{} r{},",
            cond, set_cc, rd);
        break;
    default: std::unreachable();
    }
    instr.desc += ((instr.opcode >> 25) & 1) 
        ? std::format("#{:08X}", op2) : std::format("r{}", rm);
}

void Debugger::print_single_transfer(Instr& instr) {
    const char* cond = condition(instr.opcode);
    bool p = (instr.opcode >> 24) & 1;
    bool i = (instr.opcode >> 25) & 1;
    bool l = (instr.opcode >> 20) & 1;
    auto rn = (instr.opcode >> 16) & 0xF;
    auto rd = (instr.opcode >> 12) & 0xF;
    const char* b = ((instr.opcode >> 22) & 1) ? "B" : "";
    const char* u = ((instr.opcode >> 23) & 1) ? "" : "-";
    const char* writeback = (!p || (p && ((instr.opcode >> 21) & 1))) ? "!" : "";

    if (l) {
        instr.desc = std::format("LDR{}{} ", cond, b);
    } else {
        instr.desc = std::format("STR{}{} ", cond, b);
    }
    instr.desc += std::format("r{},[r{}", rd, rn);

    auto offset = instr.opcode & 0xFFF;
    auto rm = instr.opcode & 0xF;
    auto shift_amount = (instr.opcode >> 7) & 0x1F;
    auto shift_opcode = (instr.opcode >> 5) & 3;
    if (p) {
        if (!i) {
            instr.desc += !offset ? "]" : std::format(",#{}{:08X}]{}", u, offset, writeback);
        } else {
            instr.desc += std::format(",{}r{},{}#{}]{}", u, rm, shift_type(shift_opcode), shift_amount, writeback);
        }
    } else {
        instr.desc += "],";
        if (!i) {
            instr.desc += std::format("#{}{:08X}", u, offset);
        } else {
            instr.desc += std::format("{}r{},{}#{}", u, rm, shift_type(shift_opcode), shift_amount);
        }
    }
}

void Debugger::print_halfword_transfer(Instr& instr) {
    const char* cond = condition(instr.opcode);
    bool p = (instr.opcode >> 24) & 1;
    bool i = (instr.opcode >> 22) & 1;
    bool w = (instr.opcode >> 21) & 1;
    bool l = (instr.opcode >> 20) & 1;
    auto rd = (instr.opcode >> 12) & 0xF;
    auto rn = (instr.opcode >> 16) & 0xF;
    auto opcode = (instr.opcode >> 5) & 3;

    if (l) {
        switch (opcode) {
        case 1:
            instr.desc = std::format("STR{}H ", cond);
            break;
        case 2:
            instr.desc = std::format("LDR{}D ", cond);
            break;
        case 3:
            instr.desc = std::format("STR{}D ", cond);
            break;
        default: std::unreachable();
        }
    } else {
        switch (opcode) {
        case 1:
            instr.desc = std::format("LDR{}H ", cond);
            break;
        case 2:
            instr.desc = std::format("LDR{}SB ", cond);
            break;
        case 3:
            instr.desc = std::format("STR{}SH ", cond);
            break;
        default: std::unreachable();
        }
    }
    instr.desc += std::format("r{},[r{}", rd, rn);

    std::uint8_t rm_or_offset = instr.opcode & 0xF;
    std::uint8_t offset = (((instr.opcode >> 8) & 0xF) << 4) | rm_or_offset;
    const char* u = (instr.opcode >> 23) & 1 ? "" : "-";
    const char* writeback = (p && w) || !p ? "!" : "";

    if (p) {
        if (!i) {
            instr.desc += !offset ? "]" : std::format(",#{}{:08X}]{}", u, offset, writeback);
        } else {
            instr.desc += std::format(",{}r{}]{}", u, rm_or_offset, writeback);
        }
    } else {
        instr.desc += "],";
        if (!i) {
            instr.desc += std::format("#{}{:08X}", u, offset);
        } else {
            instr.desc += std::format("{}r{}", u, rm_or_offset);
        }
    }
}

void Debugger::print_block_transfer(Instr& instr) {
    const char* cond = condition(instr.opcode);
    auto reg_list = instr.opcode & 0xFFFF;
    auto rn = (instr.opcode >> 16) & 0xF;
    auto pu = (instr.opcode >> 23) & 3;
    const char* w = (instr.opcode >> 21) & 1 ? "!" : "";

    
    instr.desc = (instr.opcode >> 20) & 1 ? "LDM" : "STM";
    instr.desc += std::format("{}{} r{}{},", cond, amod(pu), rn, w) + "{";

    bool in_range = false;
    for (int i = 0; i < 16; i++) {
        if ((reg_list >> i) & 1) {
            bool next_reg_unset = (i == 15) || !((reg_list >> (i + 1)) & 1);
            if (in_range) {
                if (next_reg_unset) {
                    instr.desc += std::format("-r{},", i);
                    in_range = false;
                }
                // do nothing otherwise
            } else {
                instr.desc += std::format("r{}", i);
                in_range = true;
                if (next_reg_unset) {
                    instr.desc += ',';
                }
            }
        } else {
            in_range = false;
        }
    }
    instr.desc[instr.desc.size() - 1] = '}';
}

void Debugger::decompile_arm_instr(Instr& instr) {
    const char* cond = condition(instr.opcode);
    std::uint16_t opcode = (((instr.opcode >> 20) & 0xFF) << 4) | ((instr.opcode >> 4) & 0xF);
    switch (m_cpu->m_arm_lut[opcode]) {
    case CPU::InstrFormat::NOP: {
        instr.desc = "NOP";
        break;
    }
    case CPU::InstrFormat::B: {
        bool l = (instr.opcode >> 24) & 1;
        std::int32_t nn = (static_cast<std::int32_t>((instr.opcode & 0xFFFFFF) << 8) >> 8) << 2;
        instr.desc = std::format("B{}{} #{:08X}", 
            l ? "L" : "", cond, (instr.addr + 8) + nn);
        break;
    }
    case CPU::InstrFormat::BX: {
        std::uint8_t rn = instr.opcode & 0xF;
        instr.desc = std::format("BX{} r{}", cond, rn);
        break;
    }
    case CPU::InstrFormat::ALU: return print_alu(instr);
    case CPU::InstrFormat::SINGLE_TRANSFER: return print_single_transfer(instr);
    case CPU::InstrFormat::HALFWORD_TRANSFER: return print_halfword_transfer(instr);
    case CPU::InstrFormat::BLOCK_TRANSFER: return print_block_transfer(instr);
    case CPU::InstrFormat::MSR: {
        bool i = (instr.opcode >> 25) & 1;
        const char* f = ((instr.opcode >> 19) & 1) ? "f" : "";
        const char* c = ((instr.opcode >> 16) & 1) ? "c" : "";
        std::uint8_t shift_amount = ((instr.opcode >> 8) & 0xF) * 2;

        instr.desc = std::format("MSR{} ", cond);
        if ((instr.opcode >> 22) & 1) {
            instr.desc += "psr_";
        } else {
            instr.desc += "cpsr_";
        }
        instr.desc += std::format("{}{}, ", f, c);
        if (i) {
            instr.desc += std::format("#{:08X}", m_cpu->ror(instr.opcode & 0xFF, shift_amount));
        } else {
            instr.desc += std::format("r{}", instr.opcode & 0xF);
        }
        break;
    }
    case CPU::InstrFormat::MRS: {
        auto rd = (instr.opcode >> 12) & 0xF;
        instr.desc = std::format("MRS{} r{}, ", cond, rd);
        if ((instr.opcode >> 22) & 1) {
            instr.desc += "spsr";
        } else {
            instr.desc += "cpsr";
        }
        break;
    }
    case CPU::InstrFormat::MUL: {
        // auto rd = (instr.opcode >> 16) & 0xF;
        // auto rn = (instr.opcode >> 12) & 0xF;
        // auto rs = (instr.opcode >> 8) & 0xF;

        switch ((instr.opcode >> 21) & 0xF) {
        default:
            // printf("MUL missing %d\n", (instr.opcode >> 21) & 0xF);
            // std::exit(1);
        }
        break;
    }
    default:
        // printf("%hhu\n", m_cpu->m_arm_lut[opcode]);
        // std::exit(1);
    }
}

void Debugger::decompile_thumb_instr(Instr& instr) {
    switch (m_cpu->m_thumb_lut[instr.opcode >> 6]) {
    case CPU::InstrFormat::THUMB_1: {
        std::uint16_t thumb = instr.opcode;
        instr.opcode = m_cpu->thumb_translate_1(thumb);
        print_alu(instr);
        instr.opcode = thumb;
        break;
    }
    case CPU::InstrFormat::THUMB_2: {
        std::uint16_t thumb = instr.opcode;
        instr.opcode = m_cpu->thumb_translate_2(thumb);
        print_alu(instr);
        instr.opcode = thumb;
        break;
    }
    case CPU::InstrFormat::THUMB_3: {
        std::uint16_t thumb = instr.opcode;
        instr.opcode = m_cpu->thumb_translate_3(thumb);
        print_alu(instr);
        instr.opcode = thumb;
        break;
    }

    case CPU::InstrFormat::THUMB_7: {
        std::uint16_t thumb = instr.opcode;
        instr.opcode = m_cpu->thumb_translate_7(thumb);
        print_single_transfer(instr);
        instr.opcode = thumb;
        break;
    }
    case CPU::InstrFormat::THUMB_8: {
        std::uint16_t thumb = instr.opcode;
        instr.opcode = m_cpu->thumb_translate_8(thumb);
        print_halfword_transfer(instr);
        instr.opcode = thumb;
        break;
    }
    case CPU::InstrFormat::THUMB_9: {
        std::uint16_t thumb = instr.opcode;
        instr.opcode = m_cpu->thumb_translate_9(thumb);
        print_single_transfer(instr);
        instr.opcode = thumb;
        break;
    }
    case CPU::InstrFormat::THUMB_10: {
        std::uint16_t thumb = instr.opcode;
        instr.opcode = m_cpu->thumb_translate_10(thumb);
        print_halfword_transfer(instr);
        instr.opcode = thumb;
        break;
    }
    case CPU::InstrFormat::THUMB_11: {
        std::uint16_t thumb = instr.opcode;
        instr.opcode = m_cpu->thumb_translate_11(thumb);
        print_single_transfer(instr);
        instr.opcode = thumb;
        break;
    }
    case CPU::InstrFormat::THUMB_17: {
        instr.desc = "SWI " + std::string(swi_subroutine(instr.opcode & 0xFF));
        break;
    }
    case CPU::InstrFormat::THUMB_13: {
        std::uint16_t thumb = instr.opcode;
        instr.opcode = m_cpu->thumb_translate_13(thumb);
        print_alu(instr);
        instr.opcode = thumb;
        break;
    }
    case CPU::InstrFormat::THUMB_14: {
        std::uint16_t thumb = instr.opcode;
        instr.opcode = m_cpu->thumb_translate_14(thumb);
        print_block_transfer(instr);
        instr.opcode = thumb;
        break;
    }
    case CPU::InstrFormat::THUMB_15: {
        std::uint16_t thumb = instr.opcode;
        instr.opcode = m_cpu->thumb_translate_15(thumb);
        print_block_transfer(instr);
        instr.opcode = thumb;
        break;
    }
    // case CPU::InstrFormat::THUMB_17: {
    //     return swi(thumb_translate_17(instr));
    //     break;
    // }
    case CPU::InstrFormat::THUMB_18: {
        std::uint16_t thumb = instr.opcode;
        instr.opcode = m_cpu->thumb_translate_18(thumb);
        print_block_transfer(instr);
        instr.opcode = thumb;
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
            instrs[idx].opcode = m_cpu->m_mem.read<std::uint16_t>(addr);
            decompile_thumb_instr(instrs[idx]);
        } else {
            instrs[idx].opcode = m_cpu->m_mem.read<std::uint32_t>(addr);
            decompile_arm_instr(instrs[idx]);
        }
    }
    return instrs;
}

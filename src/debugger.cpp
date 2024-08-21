#include "debugger.hpp"

#include "cpu.hpp"

void Debugger::translate_arm(Instr& instr) {

}

void Debugger::translate_thumb(Instr& instr) {

}

Registers& Debugger::view_registers() {
    return m_cpu.m_regs;
}

bool Debugger::thumb_enabled() {
    return m_cpu.m_thumb_enabled;
}

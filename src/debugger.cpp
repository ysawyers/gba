#include "debugger.hpp"

#include "cpu.hpp"

Registers& Debugger::view_registers(const std::shared_ptr<CPU> cpu) {
    return cpu->m_regs;
}

std::span<Debugger::Instr> Debugger::view_instructions(const std::shared_ptr<CPU> cpu) {
    if ((cpu->m_thumb_enabled == m_decompiled_thumb) && !m_decompiled_instr_cache.empty()) {
        return std::span(m_decompiled_instr_cache);
    }

    printf("get list\n");
    std::exit(1);
}

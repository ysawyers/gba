#include "memory.hpp"

#include <fstream>
#include <iostream>

void Memory::load_bios() {
    FILE *fp = fopen("roms/bios.bin", "rb");
    if (fp == NULL) {
        throw std::runtime_error("failed to open bios file");
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fread(m_bios.data(), sizeof(std::uint8_t), size, fp);
    
    fclose(fp);
}

void Memory::load_rom(const std::string& rom_filepath) {
    FILE *fp = fopen(rom_filepath.c_str(), "rb");
    if (fp == NULL) {
        throw std::runtime_error("failed to open " + rom_filepath);
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fread(m_rom.data(), sizeof(std::uint8_t), size, fp);

    fclose(fp);
}

bool Memory::pending_interrupts(bool irq_disabled) {
    bool ime = m_mmio[0x208] & 1;
    if (ime && !irq_disabled) {
        std::uint32_t if_ie = *reinterpret_cast<std::uint32_t*>(m_mmio.data() + 0x200);
        return (if_ie & 0xFFFF) & (if_ie >> 16);
    }
    return 0;
}

void Memory::tick_components(int cycles) {
    m_ppu.tick(cycles);
}

void Memory::reset_components() {
    m_ewram.clear();
    m_iwram.clear();
    m_key_input = 0xFFFF;
    m_mmio = {};
    m_ppu = {m_mmio.data()};
}

FrameBuffer& Memory::get_frame() {
    return m_ppu.m_frame;
}

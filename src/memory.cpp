#include "memory.hpp"

#include <fstream>
#include <iostream>

void Memory::load_rom(const std::string&& rom_filepath) {
    FILE *fp = fopen(rom_filepath.c_str(), "rb");
    if (fp == NULL) {
        throw std::runtime_error("failed to open " + rom_filepath);
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fread(m_rom, sizeof(std::uint8_t), size, fp);
    
    fclose(fp);
}

std::uint32_t Memory::read_word(std::uint32_t addr) {
    // align to word
    addr &= ~3;

    switch ((addr >> 24) & 0xFF) {
    case 0x00: return *reinterpret_cast<std::uint32_t*>(m_bios + addr);
    case 0x02: return *reinterpret_cast<std::uint32_t*>(m_ewram + ((addr - 0x02000000) & 0x3FFFF));
    case 0x03: return *reinterpret_cast<std::uint32_t*>(m_iwram + ((addr - 0x03000000) & 0x7FFF));
    case 0x08:
    case 0x09:
    case 0x0A:
    case 0x0B:
    case 0x0C:
    case 0x0D: return *reinterpret_cast<std::uint32_t*>(m_rom + ((addr - 0x08000000) & 0x1FFFFFF));
    default:
        std::cout << "unimplemented: " << addr << std::endl;
        std::exit(1);
    }
    return 0;
}

std::uint16_t Memory::read_halfword(std::uint32_t addr) {
    // align to halfword
    addr &= ~1;

    std::cout << "unimplemented: " << addr << std::endl;
    std::exit(1);
}

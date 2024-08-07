#include "memory.hpp"

#include <fstream>
#include <iostream>

void Memory::load_rom(const std::string&& rom_filepath) {
    std::ifstream in(rom_filepath, std::ios_base::in | std::ios_base::binary);

    if (in.is_open()) {
        m_bios.resize(0x4000);
        m_ewram.resize(0x40000);
        m_iwram.resize(0x8000);
        m_rom.resize(0x2000000);

        std::uint8_t byte = 0;
        for (int i = 0; in >> byte; i++) {
            m_rom[i] = byte;
        }
    } else {
        throw std::runtime_error("failed to open " + rom_filepath);
    }

    in.close();
}

std::uint32_t Memory::read_word(std::uint32_t addr) {
    // align to word
    addr &= ~3;

    switch ((addr >> 24) & 0xFF) {
    case 0x00: return *reinterpret_cast<std::uint32_t*>(m_bios.data() + addr);
    case 0x02: return *reinterpret_cast<std::uint32_t*>(m_ewram.data() + ((addr - 0x02000000) & 0x3FFFF));
    case 0x03: return *reinterpret_cast<std::uint32_t*>(m_iwram.data() + ((addr - 0x03000000) & 0x7FFF));
    case 0x08:
    case 0x09:
    case 0x0A:
    case 0x0B:
    case 0x0C:
    case 0x0D: return *reinterpret_cast<std::uint32_t*>(m_rom.data() + ((addr - 0x08000000) & 0x1FFFFFF));
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

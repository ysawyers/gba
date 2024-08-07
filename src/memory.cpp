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
    addr &= ~3;

    switch ((addr >> 24) & 0xFF) {
    case 0x00: return *reinterpret_cast<std::uint32_t*>(m_bios + addr);
    case 0x02: return *reinterpret_cast<std::uint32_t*>(m_ewram + ((addr - 0x02000000) & 0x3FFFF));
    case 0x03: return *reinterpret_cast<std::uint32_t*>(m_iwram + ((addr - 0x03000000) & 0x7FFF));
    case 0x04:
        switch (addr) {
        default:
            if (addr >= 0x04000000 && addr <= 0x04000054) {
                return *reinterpret_cast<std::uint32_t*>(m_ppu.m_mmio + (addr - 0x04000000));
            }
            printf("[read] unmapped hardware register: %08X\n", addr);
            exit(1);
        }
    case 0x05: return *reinterpret_cast<std::uint32_t*>(m_ppu.m_pallete_ram + ((addr - 0x05000000) & 0x3FF));
    case 0x06:
        addr = (addr - 0x06000000) & 0x1FFFF;
        if (addr >= 0x18000 && addr <= 0x1FFFF) {
            return *reinterpret_cast<std::uint32_t*>(m_ppu.m_vram + (addr - 0x8000));
        }
        return *reinterpret_cast<std::uint32_t*>(m_ppu.m_vram + addr);
    case 0x07: return *reinterpret_cast<std::uint32_t*>(m_ppu.m_oam + ((addr - 0x07000000) & 0x3FF));
    case 0x08:
    case 0x09:
    case 0x0A:
    case 0x0B:
    case 0x0C:
    case 0x0D: return *reinterpret_cast<std::uint32_t*>(m_rom + ((addr - 0x08000000) & 0x1FFFFFF));
    case 0x0E:
        printf("cart ram\n");
        exit(1);
    }
    return 0;
}

std::uint16_t Memory::read_halfword(std::uint32_t addr) {
    addr &= ~1;

    switch ((addr >> 24) & 0xFF) {
    case 0x00: return *reinterpret_cast<std::uint16_t*>(m_bios + addr);
    case 0x02: return *reinterpret_cast<std::uint16_t*>(m_ewram + ((addr - 0x02000000) & 0x3FFFF));
    case 0x03: return *reinterpret_cast<std::uint16_t*>(m_iwram + ((addr - 0x03000000) & 0x7FFF));
    case 0x04:
        switch (addr) {
        default:
            if (addr >= 0x04000000 && addr <= 0x04000054) {
                return *reinterpret_cast<std::uint16_t*>(m_ppu.m_mmio + (addr - 0x04000000));
            }
            printf("[read] unmapped hardware register: %08X\n", addr);
            exit(1);
        }
    case 0x05: return *reinterpret_cast<std::uint16_t*>(m_ppu.m_pallete_ram + ((addr - 0x05000000) & 0x3FF));
    case 0x06:
        addr = (addr - 0x06000000) & 0x1FFFF;
        if (addr >= 0x18000 && addr <= 0x1FFFF) {
            return *reinterpret_cast<std::uint16_t*>(m_ppu.m_vram + (addr - 0x8000));
        }
        return *reinterpret_cast<std::uint16_t*>(m_ppu.m_vram + addr);
    case 0x07: return *reinterpret_cast<std::uint16_t*>(m_ppu.m_oam + ((addr - 0x07000000) & 0x3FF));
    case 0x08:
    case 0x09:
    case 0x0A:
    case 0x0B:
    case 0x0C:
    case 0x0D: return *reinterpret_cast<std::uint16_t*>(m_rom + ((addr - 0x08000000) & 0x1FFFFFF));
    case 0x0E:
        printf("cart ram\n");
        exit(1);
    }
    return 0;
}

void Memory::write_word(std::uint32_t addr, std::uint32_t value) {

}

void Memory::write_halfword(std::uint32_t addr, std::uint16_t value) {
    addr &= ~1;

    static void *jump_table[] = {
        &&illegal_write, &&illegal_write, &&external_wram_reg, &&internal_wram_reg,
        &&mapped_registers, &&pallete_ram_reg, &&vram_reg, &&oam_reg, &&illegal_write,
        &&illegal_write, &&illegal_write, &&illegal_write, &&illegal_write, &&illegal_write,
        &&cart_ram_reg, &&cart_ram_reg
    };

    goto *jump_table[(addr >> 24) & 0xF];

    illegal_write: return;

    external_wram_reg:
        *reinterpret_cast<std::uint16_t*>(m_ewram + ((addr - 0x02000000) & 0x3FFFF)) = value;
        return;

    internal_wram_reg:
        *reinterpret_cast<std::uint16_t*>(m_iwram + ((addr - 0x03000000) & 0x7FFF)) = value;
        return;

    mapped_registers:
        switch (addr) {
        case 0x04000208:
            std::cout << "reg ime unimplemented" << std::endl;
            std::exit(1);
        default:
            if (addr >= 0x04000000 && addr <= 0x04000054) {
                *reinterpret_cast<std::uint16_t*>(m_ppu.m_mmio + (addr - 0x04000000)) = value;
                return;
            } else {
                printf("[write] unmapped hardware register: %08X\n", addr);
                std::exit(1);
            }
        }

    pallete_ram_reg:
        std::cout << "pallete ram" << std::endl;
        std::exit(1);
        // *(uint16_t *)(pallete_ram + ((addr - 0x05000000) & 0x3FF)) = halfword;
        // return;

    vram_reg:
        addr = (addr - 0x06000000) & 0x1FFFF;
        if (addr >= 0x18000) addr -= 0x8000;
        *reinterpret_cast<std::uint16_t*>(m_ppu.m_vram + addr) = value;
        return;

    oam_reg:
        std::cout << "oam" << std::endl;
        std::exit(1);
        // *(uint16_t *)(oam + ((addr - 0x07000000) & 0x3FF)) = halfword;
        // return;
    
    cart_ram_reg:
        printf("cart ram write unhandled\n");
        exit(1);
}

void Memory::tick_components(int cycles) {
    m_ppu.tick(cycles);
}

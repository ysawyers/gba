#include "memory.hpp"

#include <fstream>
#include <iostream>

// TODO: https://gbadev.net/gbadoc/registers.html#dma-control-registers

void Memory::load_bios() 
{
    FILE *fp = fopen("roms/bios.bin", "rb");
    if (fp == NULL) 
    {
        throw std::runtime_error("failed to open bios file");
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fread(m_bios.data(), sizeof(std::uint8_t), size, fp);
    
    fclose(fp);
}

void Memory::load_rom(const std::string& rom_filepath) 
{
    FILE *fp = fopen(rom_filepath.c_str(), "rb");
    if (fp == NULL) 
    {
        throw std::runtime_error("failed to open " + rom_filepath);
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fread(m_rom.data(), sizeof(std::uint8_t), size, fp);

    fclose(fp);
}

void Memory::tick_components(int cycles)
{
    m_ppu.tick(cycles);
}

void Memory::reset_components() 
{
    m_ewram.clear();
    m_iwram.clear();
    update_key_input(0xFFFF);
    m_mmio = {};
    // m_ppu = {m_mmio.data()};
}

FrameBuffer& Memory::get_frame() 
{
    return m_ppu.m_frame;
}

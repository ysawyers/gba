#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <string>
#include <vector>

#include "ppu.hpp"
#include "timer.hpp"

class Memory {
    public:
        Memory() : m_key_input(0), m_ime(0) {
            m_bios.resize(0x4000);
            m_ewram.resize(0x40000);
            m_iwram.resize(0x8000);
            m_rom.resize(0x2000000);
        }

        void load_bios();
        void load_rom(const std::string& rom_filepath);
    
        std::uint32_t read_word(std::uint32_t addr);
        std::uint16_t read_halfword(std::uint32_t addr);
        std::uint8_t read_byte(std::uint32_t addr);

        void write_word(std::uint32_t addr, std::uint32_t value);
        void write_halfword(std::uint32_t addr, std::uint16_t value);
        void write_byte(std::uint32_t addr, std::uint8_t value);

        void tick_components(int cycles);
        void reset_components();

        FrameBuffer& get_frame();

        std::uint16_t m_key_input;
        bool m_ime;

    private:
        std::vector<std::uint8_t> m_bios;
        std::vector<std::uint8_t> m_ewram;
        std::vector<std::uint8_t> m_iwram;
        std::vector<std::uint8_t> m_rom;

        PPU m_ppu;
        Timer timer;

        // TODO: once we are not assuming 1cpi, break this up into multiple members 
        // for constant reads across different regions of memory
        std::uint16_t m_waitcnt;
};

#endif

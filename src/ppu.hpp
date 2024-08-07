#ifndef PPU_HPP
#define PPU_HPP

#include <array>

class PPU {
    public:
        PPU() : m_reg_vcount(0) {
            m_vram = new std::uint8_t[0x18000];
            m_oam = new std::uint8_t[0x400];
            m_pallete_ram = new std::uint8_t[0x400];
            m_mmio = new std::uint8_t[0x56];
        }

        ~PPU() {
            delete m_vram;
            delete m_oam;
            delete m_pallete_ram;
            delete m_mmio;
        }

        void tick(int cycles);

        std::array<std::array<std::uint16_t, 160>, 240> m_frame;
        std::uint8_t m_reg_vcount;
        std::uint8_t* m_vram;
        std::uint8_t* m_oam;
        std::uint8_t* m_pallete_ram;
        std::uint8_t* m_mmio;
};

#endif

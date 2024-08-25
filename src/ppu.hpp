#ifndef PPU_HPP
#define PPU_HPP

#include <array>
#include <vector>

typedef std::array<std::array<std::uint16_t, 240>, 160> FrameBuffer;

class PPU {
    public:
        PPU() : m_vcount(0), m_scanline_cycles(0) {
            m_vram.resize(0x18000);
            m_oam.resize(0x400);
            m_pallete_ram.resize(0x400);
        }

        void tick(int cycles);

        bool is_rendering_bitmap();

        FrameBuffer m_frame{{}};
        std::uint16_t m_vcount;
        std::uint32_t m_scanline_cycles;
        std::vector<std::uint8_t> m_vram;
        std::vector<std::uint8_t> m_oam;
        std::vector<std::uint8_t> m_pallete_ram;
        std::array<std::uint8_t, 0x56> m_mmio{};

    private:

        std::uint16_t get_tile_offset(int tx, int ty, bool bg_reg_64x64);

        void render_text_bg(std::uint16_t bgcnt, std::uint16_t bghofs, std::uint16_t bgvofs);

        void scanline_tilemap_0(std::uint16_t dispcnt);
        void scanline_tilemap_1();
        void scanline_tilemap_2();
        void scanline_bitmap_3();
        void scanline_bitmap_4();
        void scanline_bitmap_5();

        std::uint16_t get_dispcnt();
};

#endif

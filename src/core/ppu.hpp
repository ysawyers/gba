#ifndef PPU_HPP
#define PPU_HPP

#include <array>
#include <vector>

typedef std::array<std::array<std::uint16_t, 240>, 160> FrameBuffer;

class PPU {
    public:
        PPU(std::uint8_t* mmio) : m_vcount(0), m_mmio(mmio), m_scanline_cycles(0) 
        {
            m_vram.resize(0x18000);
            m_oam.resize(0x400);
            m_pallete_ram.resize(0x400);
        }

        void tick(int cycles);

        bool is_rendering_bitmap();

    public:
        FrameBuffer m_frame{{}};
        std::uint16_t m_vcount;
        std::vector<std::uint8_t> m_vram;
        std::vector<std::uint8_t> m_oam;
        std::vector<std::uint8_t> m_pallete_ram;
        std::uint8_t* m_mmio;

    private:
        std::uint16_t get_tile_offset(int tx, int ty, bool bg_reg_64x64) const noexcept;
        std::array<std::uint16_t, 4> bg_priority_list() const noexcept;
        std::array<std::uint16_t, 4> sprite_priority_list() const noexcept;

        void render_backdrop();
        void render_text_bg(std::uint16_t bgcnt, std::uint16_t bghofs, std::uint16_t bgvofs);
        void draw_scanline_tilemap_0(std::uint16_t dispcnt);
        void draw_scanline_tilemap_1(std::uint16_t dispcnt);
        void draw_scanline_tilemap_2(std::uint16_t dispcnt);
        void draw_scanline_bitmap_3();
        void draw_scanline_bitmap_4();
        void draw_scanline_bitmap_5();

    private:
        std::uint32_t m_scanline_cycles;
};

#endif
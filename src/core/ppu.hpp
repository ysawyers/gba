#ifndef PPU_HPP
#define PPU_HPP

#include <array>
#include <vector>
#include <span>

typedef std::array<std::array<std::uint16_t, 240>, 160> FrameBuffer;

class PPU
{
    public:
        PPU(std::span<std::uint16_t, 42> mmio) : m_mmio(mmio), m_scanline_cycles(1) 
        {
            m_vram.resize(0x18000);
            m_oam.resize(0x400);
            m_pallete_ram.resize(0x400);
        }

        bool is_rendering_bitmap();
        void tick(int cycles);

    public:
        FrameBuffer m_frame{{}};
        std::vector<std::uint8_t> m_vram;
        std::vector<std::uint8_t> m_oam;
        std::vector<std::uint8_t> m_pallete_ram;
        std::span<std::uint16_t, 42> m_mmio;

    private:
        std::uint16_t get_tile_offset(int tx, int ty, bool bg_reg_64x64) const noexcept;
        std::uint16_t get_sprite_size(std::uint8_t shape) const noexcept;
        std::array<std::uint16_t, 4> bg_priority_list() const noexcept;

        void render_backdrop();
        void render_text_bg(std::uint16_t bgcnt, std::uint16_t bghofs, std::uint16_t bgvofs);
        void render_sprite(std::uint64_t sprite_entry, bool is_dim_1);

        void draw_scanline_tilemap_0();
        void draw_scanline_tilemap_1();
        void draw_scanline_tilemap_2();
        void draw_scanline_bitmap_3();
        void draw_scanline_bitmap_4();
        void draw_scanline_bitmap_5();

    private:
        enum MMIO
        {
            REG_DISPCNT = 0,
            REG_DISPSTAT = 2,
            REG_VCOUNT = 3,
            REGS_BGCNT = 4, // base offset to group of bgxcnt regs
            REGS_OFS = 8 // base offset to group of vofs/hofs regs
        };

        std::uint32_t m_scanline_cycles;
};

#endif
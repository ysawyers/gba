#include "ppu.hpp"

#include <iostream>

constexpr std::uint32_t cycles_per_scanline = 1232;
constexpr std::uint8_t frame_height = 160;
constexpr std::uint8_t frame_width = 240;

std::uint16_t PPU::get_dispcnt() {
    return *reinterpret_cast<std::uint16_t*>(m_mmio);
}

void PPU::scanline_tilemap_0() {
    std::cout << "scanline tilemap 0" << std::endl;
    std::exit(1);
}

void PPU::scanline_tilemap_1() {
    std::cout << "scanline tilemap 1" << std::endl;
    std::exit(1);
}

void PPU::scanline_tilemap_2() {
    std::cout << "scanline tilemap 2" << std::endl;
    std::exit(1);
}

// https://gbadev.net/gbadoc/graphics.html?highlight=video%20modes#mode-3
void PPU::scanline_bitmap_3() {
    for (int col = 0; col < frame_width; col++) {
        m_frame[m_vcount][col] = *reinterpret_cast<uint16_t*>(m_vram + (m_vcount * (frame_width * 2)) + (col * 2));
    }
}

void PPU::scanline_bitmap_4() {
    std::cout << "scanline bitmap 4" << std::endl;
    std::exit(1);
}

void PPU::scanline_bitmap_5() {
    std::cout << "scanline bitmap 5" << std::endl;
    std::exit(1);
}

void PPU::tick(int cycles) {
    for (int i = 0; i < cycles; i++) {
        m_scanline_cycles += 1;

        if (m_vcount >= frame_height) {
            // vblank
            if (m_scanline_cycles == cycles_per_scanline) [[unlikely]] {
                m_scanline_cycles = 0;
                if (++m_vcount == 228) {
                    m_vcount = 0;
                    *reinterpret_cast<std::uint16_t*>(m_mmio + 4) &= ~3;
                }
            }
        } else if (m_scanline_cycles == cycles_per_scanline) {
            // end of scanline
            m_scanline_cycles = 0;
            m_vcount += 1;
            if (m_vcount == frame_height) {
                *reinterpret_cast<std::uint16_t*>(m_mmio + 4) |= 3;
            } else {
                *reinterpret_cast<std::uint16_t*>(m_mmio + 4) &= ~3;
            }
        } else if (m_scanline_cycles == 1004) {
            // hblank
            *reinterpret_cast<std::uint16_t*>(m_mmio + 4) |= 2;
        } else if (m_scanline_cycles == 32) {
            auto reg_dispct = get_dispcnt();
            std::uint8_t enable = reg_dispct >> 8;
            if (enable) [[likely]] {
                switch (reg_dispct & 7) {
                case 0:
                    scanline_tilemap_0();
                    break;
                case 1:
                    scanline_tilemap_1();
                    break;
                case 2:
                    scanline_tilemap_2();
                    break;
                case 3:
                    scanline_bitmap_3();
                    break;
                case 4:
                    scanline_bitmap_4();
                    break;
                case 5:
                    scanline_bitmap_5();
                    break;
                }
            } else {
                std::cout << "no rendering bits enabled yet!\n";
                std::exit(1);
            }
        }
    }
}

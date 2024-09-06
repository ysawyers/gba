#include "ppu.hpp"

#include <iostream>

constexpr std::uint32_t cycles_per_scanline = 1232;
constexpr std::uint8_t frame_height = 160;
constexpr std::uint8_t frame_width = 240;

std::uint16_t PPU::get_dispcnt() {
    return *reinterpret_cast<std::uint16_t*>(m_mmio);
}

bool PPU::is_rendering_bitmap() {
    auto rendering_mode = get_dispcnt() & 7;
    return (rendering_mode == 3 | rendering_mode == 4 | rendering_mode == 5);
}

std::uint16_t PPU::get_tile_offset(int tx, int ty, bool bg_reg_64x64) {
    int tile_offset = (tx * 2) + (ty * 64);
    if (tx >= 32) {
        tile_offset += (0x03E0 * 2);
    }
    if ((ty >= 32) && bg_reg_64x64) {
        tile_offset += (0x0400 * 2);
    }
    return tile_offset;
}

void PPU::render_text_bg(std::uint16_t bgcnt, std::uint16_t bghofs, std::uint16_t bgvofs) {
    auto tm_width = 32 * (1 + ((bgcnt >> 0xE) & 1));
    auto tm_height = 32 * (1 + ((bgcnt >> 0xF) & 1));
    auto tile_data_base = m_vram.data() + (((bgcnt >> 2) & 3) * 0x4000);
    auto tile_map_base = m_vram.data() + (((bgcnt >> 0x8) & 0x1F) * 0x800);
    bool bg_reg_64x64 = ((bgcnt >> 0xE) & 3) == 3;
    bool color_pallete = (bgcnt >> 7) & 1;
    bool mosaic_enable = (bgcnt >> 6) & 1;
    auto bpp = 4 << color_pallete;

    if (mosaic_enable) {
        printf("implement mosaic\n");
        std::exit(1);
    }

    auto tx = ((bghofs & ~7) / 8) & (tm_width - 1);
    auto ty = (((m_vcount + bgvofs) & ~7) / 8) & (tm_height - 1);
    auto scanline_x = 0;

    while (true) {
        auto screen_entry = *reinterpret_cast<std::uint16_t*>(tile_map_base + get_tile_offset(tx, ty, bg_reg_64x64));
        auto tile_id = screen_entry & 0x3FF;
        auto pallete_bank = ((screen_entry >> 0xC) & 0xF) << 4; // only applied with 4bpp
        bool hz_flip = (screen_entry >> 0xA) & 1;
        bool vf_flip = (screen_entry >> 0xB) & 1;

        auto tile_scanline = (m_vcount - (m_vcount & ~7));
        auto tile = tile_data_base + (tile_id * (0x20 << color_pallete)) + ((vf_flip ? 7 - tile_scanline : tile_scanline) * bpp);

        int start, end, step;
        if (hz_flip) {
            start = 3;
            end = -1;
            step = -1;
        } else {
            start = 0;
            end = 4;
            step = 1;
        }

        for (int i = start; i != end; i += step) {
            for (int nibble = hz_flip; nibble != (!hz_flip + step); nibble += step) {
                if (scanline_x >= frame_width) return;

                int px = i * 2 + nibble;
                if (!scanline_x && (px < (bghofs - (bghofs & ~7)))) continue;

                std::uint8_t pallete_id = color_pallete ? tile[px] : pallete_bank | ((tile[i] >> (nibble * 4)) & 0x0F);
                m_frame[m_vcount][scanline_x++] = *reinterpret_cast<std::uint16_t*>(m_pallete_ram.data() + (pallete_id * 2));
            }
        }

        tx = (tx + 1) & (tm_width - 1);
    }
}

void PPU::scanline_tilemap_0(std::uint16_t dispcnt) {
    auto prev_bg = 0;
    for (int i = 0; i < 4; i++) {
        for (int bg = 0; bg < 4; bg++) {
            bool bg_enabled = (dispcnt >> (8 + bg)) & 1;
            if (bg_enabled) {
                auto bgcnt = *reinterpret_cast<std::uint16_t*>(&(m_mmio[8 + (bg * 2)]));
                auto precedence = (bgcnt & 3) + 1;
                if (prev_bg <= precedence) {
                    prev_bg = precedence;
                    auto bghofs = *reinterpret_cast<std::uint16_t*>(&(m_mmio[0x10 + (bg * 4)]));
                    auto bgvofs = *reinterpret_cast<std::uint16_t*>(&(m_mmio[0x10 + 2 + (bg * 4)]));
                    render_text_bg(bgcnt, bghofs & 0x3FF, bgvofs & 0x3FF);
                    break;
                }
            }
        }
    }
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
        m_frame[m_vcount][col] = *reinterpret_cast<uint16_t*>(m_vram.data() + (m_vcount * (frame_width * 2)) + (col * 2));
    }
}

void PPU::scanline_bitmap_4() {
    std::uint8_t* vram_base_ptr = m_vram.data() + (((get_dispcnt() >> 4) & 1) * 0xA000);
    for (int col = 0; col < frame_width; col++) {
        std::uint8_t pallete_idx = *(vram_base_ptr + (m_vcount * frame_width) + col);
        m_frame[m_vcount][col] = *reinterpret_cast<uint16_t*>(m_pallete_ram.data() + pallete_idx * 2);
    }
}

void PPU::scanline_bitmap_5() {
    std::cout << "scanline bitmap 5" << std::endl;
    std::exit(1);
}

void PPU::tick(int cycles) {
    std::uint16_t* dispstat = reinterpret_cast<std::uint16_t*>(m_mmio + 0x004);

    for (int i = 0; i < cycles; i++) {
        m_scanline_cycles += 1;

        if (m_vcount >= frame_height) {
            // vblank
            if (m_scanline_cycles == cycles_per_scanline) {
                m_scanline_cycles = 0;
                if (++m_vcount == 228) {
                    m_vcount = 0;
                    *reinterpret_cast<std::uint16_t*>(m_mmio + 4) &= ~3;
                }
            } else if (m_scanline_cycles == 1004) {
                // hblank-in-vblank IRQ
                if ((*dispstat >> 4) & 1) {
                    *reinterpret_cast<std::uint16_t*>(m_mmio + 0x202) |= 2;
                }
            }
        } else if (m_scanline_cycles == cycles_per_scanline) {
            // end of scanline
            m_scanline_cycles = 0;
            m_vcount += 1;

            if ((*dispstat >> 5) & 1) {
                // vcount IRQ
                if (m_vcount == ((*dispstat >> 8) & 0xFF)) {
                    *reinterpret_cast<std::uint16_t*>(m_mmio + 0x202) |= 4;
                }
            }

            if (m_vcount == frame_height) {
                // entering vblank
                *dispstat |= 3;
                if ((*dispstat >> 3) & 1) {
                    // vblank IRQ
                    *reinterpret_cast<std::uint16_t*>(m_mmio + 0x202) |= 1;
                }
            } else {
                // entering hdraw
                *dispstat &= ~3;
            }
        } else if (m_scanline_cycles == 1004) {
            // entering hblank
            *dispstat |= 2;
            if ((*dispstat >> 4) & 1) {
                // hblank IRQ
                *reinterpret_cast<std::uint16_t*>(m_mmio + 0x202) |= 2;
            }
        } else if (m_scanline_cycles == 960) {
            // hdraw
            auto dispcnt = get_dispcnt();
            std::uint8_t enable = dispcnt >> 8;
            if (enable) [[likely]] {
                switch (dispcnt & 7) {
                case 0:
                    scanline_tilemap_0(dispcnt);
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
                std::uint16_t bg_color = *reinterpret_cast<std::uint16_t*>(m_pallete_ram.data());
                for (int i = 0; i < frame_width; i++) {
                    m_frame[m_vcount][i] = bg_color;
                }
            }
        }
    }
}

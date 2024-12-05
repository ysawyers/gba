#include "ppu.hpp"

#include <iostream>

const std::uint8_t FRAME_HEIGHT = 160;
const std::uint8_t frame_width = 240;

bool PPU::is_rendering_bitmap() 
{
    auto rendering_mode = *reinterpret_cast<std::uint16_t*>(m_mmio) & 7;
    return (rendering_mode == 3 | rendering_mode == 4 | rendering_mode == 5);
}

std::uint16_t PPU::get_tile_offset(int tx, int ty, bool bg_reg_64x64) const noexcept
{
    int tile_offset = (tx * 2) + (ty * 64);
    if (tx >= 32) 
    {
        tile_offset += (0x03E0 * 2);
    }
    if ((ty >= 32) && bg_reg_64x64) 
    {
        tile_offset += (0x0400 * 2);
    }
    return tile_offset;
}

void PPU::render_backdrop()
{
    std::uint16_t backdrop_color = *reinterpret_cast<std::uint16_t*>(m_pallete_ram.data());
    for (int i = 0; i < frame_width; i++)
    {
        m_frame[m_vcount][i] = backdrop_color;
    }
}

void PPU::render_text_bg(std::uint16_t bgcnt, std::uint16_t bghofs, std::uint16_t bgvofs) 
{
    auto tm_width = 32 * (1 + ((bgcnt >> 0xE) & 1));
    auto tm_height = 32 * (1 + ((bgcnt >> 0xF) & 1));
    auto tile_data_base = m_vram.data() + (((bgcnt >> 2) & 3) * 0x4000);
    auto tile_map_base = m_vram.data() + (((bgcnt >> 0x8) & 0x1F) * 0x800);
    bool bg_reg_64x64 = ((bgcnt >> 0xE) & 3) == 3;
    bool color_pallete = (bgcnt >> 7) & 1;
    bool mosaic_enable = (bgcnt >> 6) & 1;
    auto bpp = 4 << color_pallete;

    if (mosaic_enable) 
    {
        printf("implement mosaic\n");
        std::exit(1);
    }

    auto combined_vofs = bgvofs + m_vcount;
    auto tx = ((bghofs & ~7) / 8) & (tm_width - 1);
    auto ty = ((combined_vofs & ~7) / 8) & (tm_height - 1);
    auto scanline_x = 0;

    while (true)
    {
        auto screen_entry = *reinterpret_cast<std::uint16_t*>(tile_map_base + get_tile_offset(tx, ty, bg_reg_64x64));
        auto tile_id = screen_entry & 0x3FF;
        auto pallete_bank = ((screen_entry >> 0xC) & 0xF) << 4;
        bool hz_flip = (screen_entry >> 0xA) & 1;
        bool vf_flip = (screen_entry >> 0xB) & 1;

        auto tile_scanline = combined_vofs - (combined_vofs & ~7);
        auto tile = tile_data_base + (tile_id * (0x20 << color_pallete)) + ((vf_flip ? 7 - tile_scanline : tile_scanline) * bpp);

        int start, end, step;
        if (hz_flip) 
        {
            start = 3;
            end = -1;
            step = -1;
        } 
        else 
        {
            start = 0;
            end = 4;
            step = 1;
        }

        for (int i = start; i != end; i += step) 
        {
            for (int nibble = hz_flip; nibble != (!hz_flip + step); nibble += step) 
            {
                if (scanline_x >= frame_width) return;

                int px = i * 2 + nibble;
                if (!scanline_x && (px < (bghofs - (bghofs & ~7)))) continue;

                std::uint8_t pallete_id;
                std::uint16_t transparent_color;
                
                if (color_pallete)
                {
                    pallete_id = tile[px];
                    transparent_color = *reinterpret_cast<std::uint16_t*>(m_pallete_ram.data());
                }
                else
                {
                    pallete_id = pallete_bank | ((tile[i] >> (nibble * 4)) & 0x0F);
                    transparent_color = *reinterpret_cast<std::uint16_t*>(m_pallete_ram.data() + (pallete_bank * 2));
                }
                
                std::uint16_t pixel_color = *reinterpret_cast<std::uint16_t*>(m_pallete_ram.data() + (pallete_id * 2));
                if (pixel_color != transparent_color)
                {
                    m_frame[m_vcount][scanline_x] = pixel_color;
                }
                scanline_x++;
            }
        }

        tx = (tx + 1) & (tm_width - 1);
    }
}

std::array<std::uint16_t, 4> PPU::bg_priority_list() const noexcept
{
    std::array<std::uint16_t, 4> priority_list;

    for (int bg = 0; bg < 4; bg++)
    {
        auto bgcnt = *reinterpret_cast<std::uint16_t*>(m_mmio + 0x008 + bg * 2);
        priority_list[bg] = bgcnt | (bg << 4);
    }

    for (int i = 1; i < 4; i++)
    {
        int j = i - 1;
        while ((j >= 0) && ((priority_list[j] & 3) > (priority_list[j + 1] & 3)))
        {
            auto temp = priority_list[j];
            priority_list[j] = priority_list[j + 1];
            priority_list[j + 1] = temp;
            j--;
        }
    }

    return priority_list;
}

std::array<std::uint16_t, 4> PPU::sprite_priority_list() const noexcept
{
    std::array<std::uint16_t, 4> priority_list;

    

    return priority_list;
}

void PPU::draw_scanline_tilemap_0(std::uint16_t dispcnt) 
{
    auto bg_list = bg_priority_list();
    for (int bg = bg_list.size() - 1; bg >= 0; bg--)
    {
        auto background_id = (bg_list[bg] >> 4) & 3;
        bool should_display_bg = (dispcnt >> (8 + background_id)) & 1;
        if (should_display_bg)
        {
            auto vofs_hofs = *reinterpret_cast<std::uint32_t*>(m_mmio + 0x010 + background_id * 4);
            render_text_bg(bg_list[bg], vofs_hofs & 0x3FF, (vofs_hofs >> 16) & 0x3FF);
        }
    }
}

void PPU::draw_scanline_tilemap_1(std::uint16_t dispcnt) 
{
    std::cout << "scanline tilemap 1" << std::endl;
    std::exit(1);
}

void PPU::draw_scanline_tilemap_2(std::uint16_t dispcnt) 
{
    std::cout << "scanline tilemap 2" << std::endl;
    std::exit(1);
}

void PPU::draw_scanline_bitmap_3() 
{
    for (int col = 0; col < frame_width; col++) 
    {
        m_frame[m_vcount][col] = *reinterpret_cast<uint16_t*>(m_vram.data() + (m_vcount * (frame_width * 2)) + (col * 2));
    }
}

void PPU::draw_scanline_bitmap_4()
{
    std::uint8_t* vram_base_ptr = m_vram.data() + (((*reinterpret_cast<std::uint16_t*>(m_mmio) >> 4) & 1) * 0xA000);
    for (int col = 0; col < frame_width; col++) 
    {
        std::uint8_t pallete_idx = *(vram_base_ptr + (m_vcount * frame_width) + col);
        m_frame[m_vcount][col] = *reinterpret_cast<uint16_t*>(m_pallete_ram.data() + pallete_idx * 2);
    }
}

void PPU::draw_scanline_bitmap_5() 
{
    std::cout << "scanline bitmap 5" << std::endl;
    std::exit(1);
}

void PPU::tick(int cycles)
{
    for (int i = 0; i < cycles; i++)
    {
        std::uint16_t* dispstat = reinterpret_cast<std::uint16_t*>(m_mmio + 0x004);

        if (m_scanline_cycles == 1006)
        {
            *dispstat |= 2;
            if ((*dispstat >> 4) & 1)
            {
                *reinterpret_cast<std::uint16_t*>(m_mmio + 0x202) |= (1 << 1);
            }
        }
        else if (m_scanline_cycles == 0)
        {
            if (m_vcount < FRAME_HEIGHT)
            {
                render_backdrop();

                *dispstat &= ~3;
                std::uint16_t dispcnt = *reinterpret_cast<std::uint16_t*>(m_mmio);
                bool should_force_blank = (dispcnt >> 7) & 1;
                if (!should_force_blank)
                {
                    switch (dispcnt & 7) 
                    {
                    case 0:
                        draw_scanline_tilemap_0(dispcnt);
                        break;
                    case 1:
                        draw_scanline_tilemap_1(dispcnt);
                        break;
                    case 2:
                        draw_scanline_tilemap_2(dispcnt);
                        break;
                    case 3:
                        draw_scanline_bitmap_3();
                        break;
                    case 4:
                        draw_scanline_bitmap_4();
                        break;
                    case 5:
                        draw_scanline_bitmap_5();
                        break;
                    default: std::unreachable();
                    }
                }
            }
            else
            {
                *dispstat |= 1;
            }

            if ((*dispstat >> 5) & 1)
            {
                std::uint8_t vcount_line_trigger = (*dispstat >> 8) & 0xFF;
                *reinterpret_cast<std::uint16_t*>(m_mmio + 0x202) |= ((m_vcount == vcount_line_trigger) << 2);
                *dispstat |= 4;
            }
            if ((*dispstat >> 3) & 1)
            {
                *reinterpret_cast<std::uint16_t*>(m_mmio + 0x202) |= (m_vcount == 0xA0);
            }

            m_vcount = (m_vcount + 1) % 228;
        }

        m_scanline_cycles = (m_scanline_cycles + 1) % 1232;
    }
}

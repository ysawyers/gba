#include "ppu.hpp"

#include <iostream>

const std::uint8_t FRAME_HEIGHT = 160;
const std::uint8_t frame_width = 240;

bool PPU::is_rendering_bitmap() 
{
    auto rendering_mode = m_mmio[REG_DISPCNT] & 7;
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

std::uint16_t PPU::get_sprite_size(std::uint8_t shape) const noexcept
{
    switch (shape)
    {
    case 0b0000: return 0x0808;
    case 0b0001: return 0x1010;
    case 0b0010: return 0x2020;
    case 0b0011: return 0x4040;
    case 0b0100: return 0x1008;
    case 0b0101: return 0x2008;
    case 0b0110: return 0x2010;
    case 0b0111: return 0x4020;
    case 0b1000: return 0x0810;
    case 0b1001: return 0x0820;
    case 0b1010: return 0x1020;
    case 0b1011: return 0x2040;
    default: std::unreachable();
    }
}

void PPU::render_backdrop()
{
    std::uint16_t backdrop_color = *reinterpret_cast<std::uint16_t*>(m_pallete_ram.data());
    for (int i = 0; i < frame_width; i++)
    {
        m_frame[m_mmio[REG_VCOUNT]][i] = backdrop_color;
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

    if (mosaic_enable) 
    {
        printf("implement mosaic\n");
        std::exit(1);
    }

    auto combined_vofs = bgvofs + m_mmio[REG_VCOUNT];
    auto tx = ((bghofs & ~7) / 8) & (tm_width - 1);
    auto ty = ((combined_vofs & ~7) / 8) & (tm_height - 1);
    auto scanline_x = 0;

    while (true)
    {
        auto screen_entry = *reinterpret_cast<std::uint16_t*>(tile_map_base + get_tile_offset(tx, ty, bg_reg_64x64));
        auto tile_id = screen_entry & 0x3FF;
        auto pallete_bank = ((screen_entry >> 0xC) & 0xF) << 4;
        bool h_flip = (screen_entry >> 0xA) & 1;
        bool v_flip = (screen_entry >> 0xB) & 1;

        auto tile_scanline = combined_vofs - (combined_vofs & ~7);
        auto tile = tile_data_base 
            + (tile_id * (0x20 << color_pallete)) 
            + ((v_flip ? 7 - tile_scanline : tile_scanline) * (4 << color_pallete));

        int start, end, step;
        if (h_flip) 
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
            for (int nibble = h_flip; nibble != (!h_flip + step); nibble += step) 
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
                    m_frame[m_mmio[REG_VCOUNT]][scanline_x] = pixel_color;
                }
                scanline_x++;
            }
        }

        tx = (tx + 1) & (tm_width - 1);
    }
}

void PPU::render_sprite(std::uint64_t sprite_entry, bool is_dim_1)
{
    std::uint8_t y_coord = sprite_entry & 0xFF;
    std::uint16_t sprite_size = get_sprite_size((((sprite_entry >> 0xE) & 3) << 2) | ((sprite_entry >> (16 + 0xE)) & 3));
    std::uint8_t sprite_height = sprite_size & 0xFF;

    if (((y_coord + sprite_height) > m_mmio[REG_VCOUNT]) && (m_mmio[REG_VCOUNT] >= y_coord))
    {
        std::uint8_t sprite_length = (sprite_size >> 8) & 0xFF;
        std::uint16_t x_coord = (sprite_entry >> 16) & 0x1FF;
        auto tm_height = (sprite_height & ~7) / 8;
        auto tm_length = (sprite_length & ~7) / 8;
        auto base_tile_number = (sprite_entry >> 32) & 0x3FF;
        auto tile_scanline = (m_mmio[REG_VCOUNT] - y_coord) - ((m_mmio[REG_VCOUNT] - y_coord) & ~7);
        bool is_256_color_pallete = (sprite_entry >> 0xD) & 1;

        if ((sprite_entry >> 0xC) & 1)
        {
            printf("ENABLE MOSAIC FOR THIS SPRITE!\n");
            exit(1);
        }

        for (int tx = 0; tx < tm_length; tx++)
        {
            auto ty = ((m_mmio[REG_VCOUNT] - y_coord) & ~7) / 8;
            auto tile = m_vram.data() + 0x010000
                + ((base_tile_number + tx + (ty * (is_dim_1 ? tm_height : 32))) * (0x20 << is_256_color_pallete))
                + (tile_scanline * (4 << is_256_color_pallete));

            for (int i = 0; i < 4; i++)
            {
                for (int nibble = 0; nibble < 2; nibble++)
                {
                    int px = tx * 8 + i * 2 + nibble;
                    if ((x_coord + px) >= 240) return;

                    std::uint8_t pallete_id;
                    std::uint16_t transparent_color;
                    
                    if (is_256_color_pallete)
                    {
                        pallete_id = tile[px];
                        transparent_color = *reinterpret_cast<std::uint16_t*>(m_pallete_ram.data() + 0x200);
                    }
                    else
                    {
                        auto pallete_bank = ((sprite_entry >> (32 + 0xC)) & 0xF) << 4;
                        pallete_id = pallete_bank | ((tile[i] >> (nibble * 4)) & 0x0F);
                        transparent_color = *reinterpret_cast<std::uint16_t*>(m_pallete_ram.data() + 0x200 + (pallete_bank * 2));
                    }

                    std::uint16_t pixel_color = *reinterpret_cast<std::uint16_t*>(m_pallete_ram.data() + 0x200 + (pallete_id * 2));
                    if (pixel_color != transparent_color)
                    {
                        m_frame[m_mmio[REG_VCOUNT]][x_coord + px] = pixel_color;
                    }
                }
            }
        }
    }
}

std::array<std::uint16_t, 4> PPU::bg_priority_list() const noexcept
{
    std::array<std::uint16_t, 4> priority_list;

    for (int i = 0; i < 4; i++)
    {
        priority_list[i] = m_mmio[REGS_BGCNT + i] | (i << 4);
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

void PPU::draw_scanline_tilemap_0() 
{
    auto bg_list = bg_priority_list();

    for (int i = 7; i >= 0; i--)
    {
        if (i & 1)
        {
            auto list_idx = (i - 1) / 2;
            auto background_id = (bg_list[list_idx] >> 4) & 3;
            bool should_display_bg = (m_mmio[REG_DISPCNT] >> (8 + background_id)) & 1;
            if (should_display_bg)
            {
                render_text_bg(bg_list[list_idx], m_mmio[REGS_OFS + background_id * 2] & 0x3FF, m_mmio[REGS_OFS + background_id * 2 + 1] & 0x3FF);
            }
        }
        else if ((m_mmio[REG_DISPCNT] >> 0xC) & 1)
        {
            for (int j = 127; j >= 0; j--)
            {
                auto sprite_entry = *reinterpret_cast<std::uint64_t*>(m_oam.data() + j * 8);
                auto priority = (sprite_entry >> (32 + 0xA)) & 3;
                if ((i / 2) == priority)
                {
                    render_sprite(sprite_entry, (m_mmio[REG_DISPCNT] >> 6) & 1);    
                }
            }
        }
    }
}

void PPU::draw_scanline_tilemap_1() 
{
    std::cout << "scanline tilemap 1" << std::endl;
    std::exit(1);
}

void PPU::draw_scanline_tilemap_2() 
{
    std::cout << "scanline tilemap 2" << std::endl;
    std::exit(1);
}

void PPU::draw_scanline_bitmap_3() 
{
    for (int col = 0; col < frame_width; col++) 
    {
        m_frame[m_mmio[REG_VCOUNT]][col] = *reinterpret_cast<uint16_t*>(m_vram.data() + (m_mmio[REG_VCOUNT] * (frame_width * 2)) + (col * 2));
    }
}

void PPU::draw_scanline_bitmap_4()
{
    std::uint8_t* vram_base_ptr = m_vram.data() + (((m_mmio[REG_DISPCNT] >> 4) & 1) * 0xA000);
    for (int col = 0; col < frame_width; col++) 
    {
        std::uint8_t pallete_idx = *(vram_base_ptr + (m_mmio[REG_VCOUNT] * frame_width) + col);
        m_frame[m_mmio[REG_VCOUNT]][col] = *reinterpret_cast<uint16_t*>(m_pallete_ram.data() + pallete_idx * 2);
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
        if (m_scanline_cycles == 1232)
        {
            m_mmio[REG_DISPSTAT] &= ~2; // hdraw has started
            m_mmio[REG_VCOUNT] += 1;
            m_scanline_cycles = 1;

            // DISPTACH VCOUNT IRQ

            if (m_mmio[REG_VCOUNT] == 228)
            {
                m_mmio[REG_VCOUNT] = 0;
                m_mmio[REG_DISPSTAT] &= ~1; // vblank has ended
            }
            else if (m_mmio[REG_VCOUNT] == 160)
            {
                m_mmio[REG_DISPSTAT] |= 1; // vblank has started
                // DISPATCH VBLANK IRQ
            }
        }
        else if (m_scanline_cycles == 1007)
        {
            m_mmio[REG_DISPSTAT] |= 2; // hblank has started
            // DISPATCH HBLANK IRQ
        }
        else if ((m_scanline_cycles == 960) && (m_mmio[REG_VCOUNT] < FRAME_HEIGHT))
        {
            bool should_force_blank = (m_mmio[REG_DISPCNT] >> 7) & 1;
            if (!should_force_blank)
            {
                render_backdrop();
                switch (m_mmio[REG_DISPCNT] & 7) 
                {
                case 0:
                    draw_scanline_tilemap_0();
                    break;
                case 1:
                    draw_scanline_tilemap_1();
                    break;
                case 2:
                    draw_scanline_tilemap_2();
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
            else
            {
                for (int col = 0; col < frame_width; col++)
                {
                    m_frame[m_mmio[REG_VCOUNT]][col] = 0xFFFF;
                }
            }
        }
        m_scanline_cycles++;
    }
}

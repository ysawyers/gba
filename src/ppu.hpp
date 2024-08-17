#ifndef PPU_HPP
#define PPU_HPP

#include <array>

// ideally memory members are private and we make this class a friend of Memory
// but the issue is that will lead to a circular dependency so ??? 

class PPU {
    public:
        PPU() : m_vcount(0), m_scanline_cycles(0) {
            m_vram = new std::uint8_t[0x18000];
            m_oam = new std::uint8_t[0x400];
            m_pallete_ram = new std::uint8_t[0x400];
        }

        ~PPU() {
            delete m_vram;
            delete m_oam;
            delete m_pallete_ram;
        }

        void tick(int cycles);

        bool is_rendering_bitmap();

        std::array<std::array<std::uint16_t, 240>, 160> m_frame;
        std::uint16_t m_vcount;
        std::uint32_t m_scanline_cycles;
        std::uint8_t* m_vram;
        std::uint8_t* m_oam;
        std::uint8_t* m_pallete_ram;
        std::array<std::uint8_t, 0x56> m_mmio;

    private:
        void scanline_tilemap_0();
        void scanline_tilemap_1();
        void scanline_tilemap_2();
        void scanline_bitmap_3();
        void scanline_bitmap_4();
        void scanline_bitmap_5();

        std::uint16_t get_dispcnt();
};

#endif

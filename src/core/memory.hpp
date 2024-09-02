#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <string>
#include <vector>
#include <type_traits>

#include "ppu.hpp"
#include "timer.hpp"

class Memory {
    public:
        Memory() : m_key_input(0xFFFF), m_ppu(m_mmio.data()) {
            m_bios.resize(0x4000);
            m_ewram.resize(0x40000);
            m_iwram.resize(0x8000);
            m_rom.resize(0x2000000);
            m_sram.resize(0xFFFF);
        }

        void load_bios();
        void load_rom(const std::string& rom_filepath);

        template <typename T>
        T read(std::uint32_t addr) {
            if constexpr (std::is_same_v<T, std::uint32_t>) {
                addr &= ~3;
            } else if constexpr (std::is_same_v<T, std::uint16_t>) {
                addr &= ~1;
            }

            switch ((addr >> 24) & 0xFF) {
            case 0x00: return *reinterpret_cast<T*>(m_bios.data() + addr);
            case 0x02: return *reinterpret_cast<T*>(m_ewram.data() + ((addr - 0x02000000) & 0x3FFFF));
            case 0x03: return *reinterpret_cast<T*>(m_iwram.data() + ((addr - 0x03000000) & 0x7FFF));
            case 0x04: {
                // TODO: Just use the array to access vcount/keyinput and make this branchless
                addr -= 0x04000000;
                switch (addr) {
                case 0x006: return m_ppu.m_vcount;
                case 0x120: // scd0
                case 0x122: // scd1
                case 0x124: // scd2
                case 0x126: // scd3
                    // TODO: unimplemented
                    return 0;
                case 0x130: return m_key_input;
                default: return *reinterpret_cast<T*>(m_mmio.data() + addr);
                }
            }
            case 0x05: return *reinterpret_cast<T*>(m_ppu.m_pallete_ram.data() + ((addr - 0x05000000) & 0x3FF));
            case 0x06: {
                addr = (addr - 0x06000000) & 0x1FFFF;
                if (addr >= 0x18000 && addr <= 0x1FFFF) {
                    return *reinterpret_cast<T*>(m_ppu.m_vram.data() + (addr - 0x8000));
                }
                return *reinterpret_cast<T*>(m_ppu.m_vram.data() + addr);
            }
            case 0x07: return *reinterpret_cast<T*>(m_ppu.m_oam.data() + ((addr - 0x07000000) & 0x3FF));
            case 0x08:
            case 0x09:
            case 0x0A:
            case 0x0B:
            case 0x0C:
            case 0x0D: return *reinterpret_cast<T*>(m_rom.data() + ((addr - 0x08000000) & 0x1FFFFFF));
            case 0x0E: return *reinterpret_cast<T*>(m_sram.data() - 0x0E000000);
            }
            return 0;
        }

        template <typename T>
        void write(std::uint32_t addr, T value) {
            if constexpr (std::is_same_v<T, std::uint32_t>) {
                addr &= ~3;
            } else if constexpr (std::is_same_v<T, std::uint16_t>) {
                addr &= ~1;
            }

            switch ((addr >> 24) & 0xFF) {
            case 0x02:
                *reinterpret_cast<T*>(m_ewram.data() + ((addr - 0x02000000) & 0x3FFFF)) = value;
                break;
            case 0x03:
                *reinterpret_cast<T*>(m_iwram.data() + ((addr - 0x03000000) & 0x7FFF)) = value;
                break;
            case 0x04:
                *reinterpret_cast<T*>(m_mmio.data() + (addr - 0x04000000)) = value;
                break;
            case 0x05:
                if constexpr (std::is_same_v<T, std::uint8_t>) {
                    std::uint16_t duplicated_halfword = (value << 8) | value;
                    *reinterpret_cast<std::uint16_t*>(m_ppu.m_pallete_ram.data() + (((addr - 0x05000000) & 0x3FF) & ~1)) = duplicated_halfword;
                } else {
                    *reinterpret_cast<T*>(m_ppu.m_pallete_ram.data() + ((addr - 0x05000000) & 0x3FF)) = value;
                }
                break;
            case 0x06: {
                if constexpr (std::is_same_v<T, std::uint8_t>) {
                    addr = (addr - 0x06000000) & 0x1FFFF;
                    addr -= (addr >= 0x18000) * 0x8000;

                    if (addr >= 0x14000) break;

                    std::uint32_t bg_vram_size = 0x10000 + (0x14000 * (m_ppu.is_rendering_bitmap()));
                    if (addr < bg_vram_size) {
                        std::uint16_t duplicated_halfword = (value << 8) | value;
                        *reinterpret_cast<std::uint16_t*>(m_ppu.m_vram.data() + (addr & ~1)) = duplicated_halfword;
                    }
                    break;
                } else {
                    addr = (addr - 0x06000000) & 0x1FFFF;
                    if (addr >= 0x18000) addr -= 0x8000;
                    *reinterpret_cast<T*>(m_ppu.m_vram.data() + addr) = value;
                    break;
                }
                break;
            }
            case 0x07:
                if constexpr (!std::is_same_v<T, std::uint8_t>) {
                    *reinterpret_cast<T*>(m_ppu.m_oam.data() + ((addr - 0x07000000) & 0x3FF)) = value;
                }
                break;
            case 0x0E:
                *reinterpret_cast<T*>(m_sram.data() - 0x0E000000) = value;
                break;
            }
        }

        void tick_components(int cycles);
        void reset_components();

        bool pending_interrupts(bool irq_disabled);

        FrameBuffer& get_frame();

        std::uint16_t m_key_input;

    private:
        std::vector<std::uint8_t> m_bios;
        std::vector<std::uint8_t> m_ewram;
        std::vector<std::uint8_t> m_iwram;
        std::vector<std::uint8_t> m_rom;
        std::vector<std::uint8_t> m_sram;

        std::array<std::uint8_t, 0x304> m_mmio{};
        PPU m_ppu;
        Timer timer;
};

#endif

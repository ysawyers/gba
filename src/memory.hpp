#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <string>
#include <vector>

class Memory {
    public:
        Memory() {
            m_bios = new std::uint8_t[0x4000];
            m_ewram = new std::uint8_t[0x40000];
            m_iwram = new std::uint8_t[0x8000];
            m_rom = new std::uint8_t[0x2000000];
        }

        ~Memory() {
            delete m_bios;
            delete m_ewram;
            delete m_iwram;
            delete m_rom;
        }

        void load_rom(const std::string&& rom_filepath);
    
        std::uint32_t read_word(std::uint32_t addr);

        std::uint16_t read_halfword(std::uint32_t addr);

    private:
        std::uint8_t* m_bios;
        std::uint8_t* m_ewram;
        std::uint8_t* m_iwram;
        std::uint8_t* m_rom;
};

#endif

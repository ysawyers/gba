#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <string>
#include <vector>

class Memory {
    public:
        void load_rom(const std::string&& rom_filepath);
    
        std::uint32_t read_word(std::uint32_t addr);

        std::uint16_t read_halfword(std::uint32_t addr);

    private:
        std::vector<std::uint8_t> m_bios;
        std::vector<std::uint8_t> m_ewram;
        std::vector<std::uint8_t> m_iwram;
        std::vector<std::uint8_t> m_rom;
};

#endif
#include <iostream>

#include "program_options.hpp"
#include "window.hpp"

int main(int argc, char* argv[]) {
    try {
        ProgramOptions po;
    
        po.add_options()
            ("r", "path to GBA rom");
        po.parse_cli(argc, argv);

        const std::string rom_filepath = po.get_value("r");
        if (rom_filepath.empty()) {
            throw std::runtime_error("rom path required, use -r option");
        }

        Window window;
        window.emulate(std::move(rom_filepath));
    } catch (const std::runtime_error& ex) {
        std::cerr << "error: " << ex.what() << "\n";
    }

    return 0;
}

#include <iostream>

#include "program_options.hpp"
#include "window.hpp"

int main(int argc, char* argv[]) {
    try {
        Window window;
        ProgramOptions po;
    
        po.add_options()
        ("r", "path to GBA rom");
        po.parse_cli(argc, argv);

        const std::string rom_filepath = po.get_value("r");
        if (!rom_filepath.empty()) {
            printf("start game instantly\n");
            std::exit(1);
        }

        window.open(std::move(rom_filepath));
    } catch (const std::runtime_error& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

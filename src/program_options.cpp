#include "program_options.hpp"

#include <iostream>

DefinedOptions& ProgramOptions::add_options() noexcept {
    return m_options;
}

DefinedOptions& DefinedOptions::operator()(std::string&& option, std::string&& description) noexcept {
    m_values[option] = description;
    return *this;
};

void ProgramOptions::parse_cli(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        char* value = *(argv + i);

        if (*value == '-') {
            std::string option(++value);

            if (m_options.m_values.find(option) == m_options.m_values.end()) {
                throw std::runtime_error("unknown option -" + option);
            }

            if (++i < argc) {
                m_args[option] = *(argv + i);
                continue;
            }
            throw std::runtime_error("expected value after option -" + option);
        }
    }
}

std::string ProgramOptions::get_value(const std::string& option) noexcept {
    if (m_args.find(option) == m_args.end()) {
        return "";
    }
    return m_args[option];
}

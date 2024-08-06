#ifndef PROGRAM_OPTIONS_HPP
#define PROGRAM_OPTIONS_HPP

#include <unordered_map>
#include <string>

class DefinedOptions {
    public:
        DefinedOptions& operator()(std::string&& option, std::string&& description) noexcept;

        friend class ProgramOptions;

    private:
        std::unordered_map<std::string, std::string> m_values;
};

class ProgramOptions {
    public:
        DefinedOptions& add_options() noexcept;

        void parse_cli(int argc, char* argv[]);

        std::string get_value(const std::string& option) noexcept;

    private:
        DefinedOptions m_options;
        std::unordered_map<std::string, std::string> m_args;
};

#endif

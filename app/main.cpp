#include <iostream>
#include <string_view>

#include "traceforge/about.hpp"
#include "traceforge/version.hpp"

namespace {

void print_help(std::ostream& output) {
    output << traceforge::project_name() << " - multi-sensor telemetry tooling\n\n"
           << "Usage:\n"
           << "  traceforge --help       Show this message\n"
           << "  traceforge --version    Show the current version\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc == 1) {
        print_help(std::cout);
        return 0;
    }

    const std::string_view argument{argv[1]};
    if (argument == "--help" || argument == "-h") {
        print_help(std::cout);
        return 0;
    }

    if (argument == "--version") {
        std::cout << traceforge::project_name() << ' ' << traceforge::kVersion
                  << '\n';
        return 0;
    }

    std::cerr << "Unknown argument: " << argument << "\n\n";
    print_help(std::cerr);
    return 2;
}

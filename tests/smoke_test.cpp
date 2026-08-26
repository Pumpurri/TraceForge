#include <iostream>

#include "traceforge/about.hpp"
#include "traceforge/version.hpp"

int main() {
    if (traceforge::project_name() != "TraceForge") {
        std::cerr << "Unexpected project name\n";
        return 1;
    }

    if (traceforge::kVersion.empty()) {
        std::cerr << "Project version must not be empty\n";
        return 1;
    }

    return 0;
}

#include "scpi_core.hpp"
#include <algorithm>

std::string HandleScpiCommand(const std::string &cmd) {
    auto trimmed = cmd;
    trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), [](unsigned char c) {
        return c == '\n' || c == '\r';
    }), trimmed.end());

    if (trimmed == "*IDN?") {
        return "python-vxi11-server,bbb,1234,567\n";
    }
    return "ERROR\n";
}

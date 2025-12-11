#include "../include/Protocol.hpp"
#include <sstream>
#include <map>

namespace TCPProtocol {

// Parse une ligne TCP au format "TYPE param1=value1 param2=value2\n"
Message Message::parse(const std::string& line) {
    Message msg;
    std::istringstream iss(line);

    // Extraire le type (premier mot)
    iss >> msg.type;

    // Extraire les paramètres (format clé=valeur)
    std::string param;
    while (iss >> param) {
        size_t equalPos = param.find('=');
        if (equalPos != std::string::npos) {
            std::string key = param.substr(0, equalPos);
            std::string value = param.substr(equalPos + 1);
            msg.params[key] = value;
        }
    }

    return msg;
}

// Sérialise un message TCP au format "TYPE param1=value1 param2=value2\n"
std::string Message::serialize() const {
    std::ostringstream oss;
    oss << type;

    for (const auto& [key, value] : params) {
        oss << " " << key << "=" << value;
    }

    oss << "\n";
    return oss.str();
}

} // namespace TCPProtocol

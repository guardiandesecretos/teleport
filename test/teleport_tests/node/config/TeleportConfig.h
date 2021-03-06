#ifndef TELEPORT_TELEPORTCONFIG_H
#define TELEPORT_TELEPORTCONFIG_H


#include <string>
#include <map>
#include "TeleportConfigDefaults.h"


class TeleportConfig
{
public:
    uint64_t size();
    std::string& operator[](std::string key);

    std::map<std::string, std::string> settings{default_settings};
    std::map<std::string, std::string> command_line_settings{};

    void ReadFromStream(std::stringstream &config_stream);

    void ParseCommandLineArguments(int argc, const char *argv[]);

    void ParseCommandLineArgument(std::string argument);

    void InterpretNegativeSetting(std::string key_name);

    bool Value(std::string key, bool default_value);

    uint64_t Value(std::string key, uint64_t default_value);

    std::string Value(std::string key, std::string default_value);

    std::string String(std::string key, std::string default_value="");

    uint64_t Uint64(std::string key, uint64_t default_value=0);

    bool Bool(std::string key, bool default_value=false);
};


#endif //TELEPORT_TELEPORTCONFIG_H

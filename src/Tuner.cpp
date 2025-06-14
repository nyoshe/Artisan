#include "Tuner.h"

std::list<TunableParam>& tunables() {
    static std::list<TunableParam> params;
    return params;
}

TunableParam& addTunableParam(std::string name, int value, int min, int max, int step) {
    tunables().push_back({ name, value, value, min, max, step });
    TunableParam& param = tunables().back();
    return param;
}


void printWeatherFactoryConfig() {
    std::cout << "{\n";
    for (auto& param : tunables())
    {
        std::cout << "    \"" << param.name << "\": {\n";
        std::cout << "        \"value\": " << param.defaultValue << ",\n";
        std::cout << "        \"min_value\": " << param.min << ",\n";
        std::cout << "        \"max_value\": " << param.max << ",\n";
        std::cout << "        \"step\": " << param.step << "\n";
        std::cout << "    }";
        if (&param != &tunables().back())
            std::cout << ",";
        std::cout << "\n";
    }
    std::cout << "}" << std::endl;
}
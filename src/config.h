#pragma once

#include <string>
#include <windows.h>

struct config_t
{
    std::string module;

    std::uint8_t bottom_shiftable_gauge_sp;
    std::uint8_t bottom_shiftable_gauge_dp;

    bool use_easy_gauge_textures;
};

void init_config(HMODULE module);

extern config_t app_cfg;
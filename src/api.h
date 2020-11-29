#pragma once

#include "bm2dx.h"

extern auto is_valid_game_type() -> bool;

extern std::int16_t p1_graph_values[5][GAUGE_POINTS];
extern std::int16_t p2_graph_values[5][GAUGE_POINTS];

extern "C"
{
    __declspec(dllexport) auto is_enabled() -> bool;
    __declspec(dllexport) auto get_graph_points(std::uint8_t type, std::uint8_t player) -> void*;
    __declspec(dllexport) auto get_graph_point_count() -> std::size_t;
}
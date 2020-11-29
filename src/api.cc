#include "api.h"

auto is_enabled() -> bool
    { return is_valid_game_type(); }

auto get_graph_points(std::uint8_t type, std::uint8_t player) -> void*
    { return player == 1 ? &p1_graph_values[type]: &p2_graph_values[type]; }

auto get_graph_point_count() -> std::size_t
    { return gauge_data_ptr->gauge_point_count; }
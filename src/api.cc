#include "api.h"

auto is_enabled() -> bool
    { return is_valid_game_type(); }

auto get_graph_points(std::uint8_t type, std::uint8_t player) -> void*
    { return player == 1 ? &p1_graph_values[type]: &p2_graph_values[type]; }

auto get_graph_point_count() -> std::size_t
    { return gauge_t::gauge_point_count(); }

auto set_initial_gauge_values(std::uint32_t player,
    std::int32_t ordinary_raw, std::int32_t survival_raw) -> bool
{
    if (player > PLAYER_2 || ordinary_raw < 100 || ordinary_raw > 5000 ||
        survival_raw < 100 || survival_raw > 5000 || state_ptr == nullptr)
        return false;

    auto const active = player == PLAYER_1 ? state_ptr->p1_active: state_ptr->p2_active;

    if (active != 1 || !is_valid_game_type())
        return false;

    auto& values = player == PLAYER_1 ? p1_gauge_values: p2_gauge_values;

    for (auto type = 0; type < 5; ++type)
        values[type] = static_cast<std::int16_t>(type < GAUGE_HARD ? ordinary_raw: survival_raw);

    return true;
}
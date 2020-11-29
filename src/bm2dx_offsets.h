#pragma once

#include <cstdint>
#include <windows.h>
#include "version.h"

struct dummy_impl final: binary_info
{
    dummy_impl() = default;
    void resolve_rva(std::uintptr_t base) override {};
};

namespace offsets
{
    void resolve(HMODULE base);

    extern binary_info* active;

    extern std::uintptr_t death_defying_patch;

    extern std::uintptr_t state_ptr;
    extern std::uintptr_t gauge_data_ptr;
    extern std::uintptr_t graph_data_ptr;
    extern std::uintptr_t option_data_ptr;
    extern std::uintptr_t stage_game_data;

    extern std::uintptr_t input_ptr;

    extern std::uintptr_t p1_groove_gauge_ptr;
    extern std::uintptr_t p2_groove_gauge_ptr;

    extern std::uintptr_t p1_chart_judgement_ptr;
    extern std::uintptr_t p2_chart_judgement_ptr;

    extern std::uintptr_t p1_gauge_option_ptr;
    extern std::uintptr_t p2_gauge_option_ptr;

    extern std::uintptr_t p1_dead_measure_ptr;
    extern std::uintptr_t p2_dead_measure_ptr;

    extern std::uintptr_t calculate_individual_chart_judge_value;

    extern std::uintptr_t target_calculate_chart_judge;
    extern std::uintptr_t target_update_groove_gauge;
    extern std::uintptr_t target_update_graph_data;
    extern std::uintptr_t target_stage_result_ctor;
    extern std::uintptr_t target_result_graph_render;
    extern std::uintptr_t target_return_from_result;
}
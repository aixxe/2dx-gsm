#include "hooks.h"
#include "bm2dx_offsets.h"

// game pointers
decltype(state_ptr) state_ptr = nullptr;
decltype(option_data_ptr) option_data_ptr = nullptr;

decltype(get_gauge_fn) get_gauge_fn = nullptr;
decltype(set_gauge_fn) set_gauge_fn = nullptr;

decltype(input_ptr) input_ptr = nullptr;

decltype(p1_groove_gauge_ptr) p1_groove_gauge_ptr = nullptr;
decltype(p2_groove_gauge_ptr) p2_groove_gauge_ptr = nullptr;

decltype(p1_result_graph_ptr) p1_result_graph_ptr = nullptr;
decltype(p2_result_graph_ptr) p2_result_graph_ptr = nullptr;

decltype(p1_chart_judgement_ptr) p1_chart_judgement_ptr = nullptr;
decltype(p2_chart_judgement_ptr) p2_chart_judgement_ptr = nullptr;

decltype(p1_gauge_option_ptr) p1_gauge_option_ptr = nullptr;
decltype(p2_gauge_option_ptr) p2_gauge_option_ptr = nullptr;

decltype(p1_gauge_option_ptr) p1_dead_measure_ptr = nullptr;
decltype(p2_gauge_option_ptr) p2_dead_measure_ptr = nullptr;

// invokable functions
decltype(calculate_individual_chart_judge_value) calculate_individual_chart_judge_value = nullptr;

// code patches
decltype(death_defying_patch) death_defying_patch = nullptr;

void iidx_gsm_load(HMODULE bm2dx)
{
    // populate version-specific offsets
    offsets::resolve(bm2dx);

    // code patches
    death_defying_patch = std::make_unique<util::code_patch>(
        util::code_patch(reinterpret_cast<void*>(offsets::death_defying_patch), {
            0x90, 0x90, 0x90, 0x90, 0x90
        })
    );

    // game pointers
    state_ptr = reinterpret_cast<decltype(state_ptr)>(offsets::state_ptr);
    option_data_ptr = reinterpret_cast<decltype(option_data_ptr)>(offsets::option_data_ptr);
    get_gauge_fn = reinterpret_cast<decltype(get_gauge_fn)>(offsets::get_gauge_fn);
    set_gauge_fn = reinterpret_cast<decltype(set_gauge_fn)>(offsets::set_gauge_fn);

    input_ptr = reinterpret_cast<decltype(input_ptr)>(offsets::input_ptr);

    p1_groove_gauge_ptr = reinterpret_cast<decltype(p1_groove_gauge_ptr)>(offsets::p1_groove_gauge_ptr);
    p2_groove_gauge_ptr = reinterpret_cast<decltype(p2_groove_gauge_ptr)>(offsets::p2_groove_gauge_ptr);

    p1_result_graph_ptr = reinterpret_cast<decltype(p1_result_graph_ptr)>(offsets::p1_result_graph_ptr);
    p2_result_graph_ptr = reinterpret_cast<decltype(p2_result_graph_ptr)>(offsets::p2_result_graph_ptr);

    p1_chart_judgement_ptr = reinterpret_cast<decltype(p1_chart_judgement_ptr)>(offsets::p1_chart_judgement_ptr);
    p2_chart_judgement_ptr = reinterpret_cast<decltype(p2_chart_judgement_ptr)>(offsets::p2_chart_judgement_ptr);

    p1_gauge_option_ptr = reinterpret_cast<decltype(p1_gauge_option_ptr)>(offsets::p1_gauge_option_ptr);
    p2_gauge_option_ptr = reinterpret_cast<decltype(p2_gauge_option_ptr)>(offsets::p2_gauge_option_ptr);

    p1_dead_measure_ptr = reinterpret_cast<decltype(p1_dead_measure_ptr)>(offsets::p1_dead_measure_ptr);
    p2_dead_measure_ptr = reinterpret_cast<decltype(p2_dead_measure_ptr)>(offsets::p2_dead_measure_ptr);

    // game functions
    calculate_individual_chart_judge_value = reinterpret_cast<decltype(calculate_individual_chart_judge_value)>
        (offsets::calculate_individual_chart_judge_value);

    // install hooks
    calculate_chart_judge_hook = safetyhook::create_inline(offsets::target_calculate_chart_judge, replacement_calculate_chart_judge);
    update_groove_gauge_hook = safetyhook::create_inline(offsets::target_update_groove_gauge, replacement_update_groove_gauge);
    update_graph_data_hook = safetyhook::create_inline(offsets::target_update_graph_data, replacement_update_graph_data);
    draw_graph_ctor_hook = safetyhook::create_inline(offsets::target_draw_graph_ctor, replacement_draw_graph_ctor);
    result_graph_render_hook = safetyhook::create_inline(offsets::target_result_graph_render, replacement_result_graph_render);
    return_from_result_hook = safetyhook::create_inline(offsets::target_return_from_result, replacement_return_from_result);
}
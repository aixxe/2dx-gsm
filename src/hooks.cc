#include <bitset>
#include "hooks.h"
#include "config.h"

// application state
SafetyHookInline calculate_chart_judge_hook;
SafetyHookInline update_groove_gauge_hook;
SafetyHookInline stage_result_ctor_hook;
SafetyHookInline result_graph_render_hook;
SafetyHookInline return_from_result_hook;
SafetyHookMid update_graph_data_hook;

bool backup_starting_gauge = true;

std::uint8_t p1_starting_gauge_type;
std::uint8_t p2_starting_gauge_type;
std::uint8_t dp_starting_gauge_type;

std::int16_t p1_gauge_values[5];
std::int16_t p2_gauge_values[5];

std::int16_t p1_graph_values[5][GAUGE_POINTS];
std::int16_t p2_graph_values[5][GAUGE_POINTS];

chart_judgement_t p1_chart_judgements[5];
chart_judgement_t p2_chart_judgements[5];

std::vector<std::vector<std::int8_t>> allowed_gauge_shifts = {
    {0, 1, 2},          // NORMAL           [NORMAL, EASY, ASSISTED EASY]
    {1},                // ASSISTED EASY    [ASSISTED EASY]
    {1, 2},             // EASY             [EASY, ASSISTED EASY]
    {0, 1, 2, 3},       // HARD             [HARD, NORMAL, EASY, ASSISTED EASY]
    {0, 1, 2, 3, 4},    // EX HARD          [EX HARD, HARD, NORMAL, EASY, ASSISTED EASY]
};

// defaults
std::int8_t gauge_priorities[] = {  2,               0,      1,      3,         4};
const char* gauge_names[] = {"NORMAL", "ASSISTED EASY", "EASY", "HARD", "EX HARD"};

std::int16_t default_gauge_values[5] = {
    (22 * 50), // NORMAL
    (22 * 50), // ASSISTED EASY
    (22 * 50), // EASY
    (100 * 50), // HARD
    (100 * 50), // EX HARD
};

// utility functions
auto set_death_defying_state(const bool enabled) -> void
{
    if (enabled)
    {
        spdlog::debug("disabled stage fail");
        death_defying_patch->enable();
    }
    else
    {
        spdlog::debug("enabled stage fail");
        death_defying_patch->disable();
    }
}

auto is_valid_game_type() -> bool
{
    static auto patch_enabled = false;

    auto game_type = state_ptr->game_type;
    auto patch_state = (game_type == 0); // STANDARD

    // enable and disable the patch depending on the game type & play style
    static auto last_game_type = -1;

    if (last_game_type != game_type)
    {
        if (patch_enabled != patch_state)
        {
            set_death_defying_state(patch_state);
            patch_enabled = patch_state;
        }
    }

    last_game_type = game_type;

    // only allow hook code to run in whitelisted game modes
    return patch_state;
}

bool can_shift_to_gauge(gauge_types type, const std::int16_t* gauge_values, const std::uint8_t starting_gauge)
{
    // check for validity
    if (gauge_values[type] > (100 * 50))
        return false;

    // ensure the player is still alive if this is a hard gauge
    if (gauge_values[type] < (2 * 50) && (type == GAUGE_EX_HARD || type == GAUGE_HARD))
        return false;

    // ensure the player is clearing on this gauge
    if (type < GAUGE_HARD && gauge_values[type] < (80 * 50))
        return false;

    // use different bottom shiftable gauges for SP/DP
    auto bottom_shiftable_gauge = (state_ptr->play_style == 0 ?
        app_cfg.bottom_shiftable_gauge_sp: app_cfg.bottom_shiftable_gauge_dp);

    // ensure the player would want to switch to this gauge
    if (gauge_priorities[type] < gauge_priorities[bottom_shiftable_gauge])
    {
        spdlog::debug("can't shift to {}, lower than bottom shiftable gauge '{}'",
            gauge_names[type], gauge_names[bottom_shiftable_gauge]);

        return false;
    }

    // ensure this is a valid "select-to-under" transition
    auto gauge_shifts = allowed_gauge_shifts.at(starting_gauge);

    return std::ranges::find(gauge_shifts, type) != gauge_shifts.end();
}

std::uint8_t get_most_appropriate_gauge(const std::int16_t* gauge_values, const std::uint8_t starting_gauge)
{
    // use different bottom shiftable gauges for SP/DP
    auto bottom_shiftable_gauge = (state_ptr->play_style == 0 ?
       app_cfg.bottom_shiftable_gauge_sp: app_cfg.bottom_shiftable_gauge_dp);

    // e.g. if player picked EASY but has NORMAL as lowest gauge, still return EASY
    if (gauge_priorities[starting_gauge] < gauge_priorities[bottom_shiftable_gauge])
        return starting_gauge;

    if (can_shift_to_gauge(GAUGE_EX_HARD, gauge_values, starting_gauge))
        return GAUGE_EX_HARD;
    if (can_shift_to_gauge(GAUGE_HARD, gauge_values, starting_gauge))
        return GAUGE_HARD;
    if (can_shift_to_gauge(GAUGE_NORMAL, gauge_values, starting_gauge))
        return GAUGE_NORMAL;
    if (can_shift_to_gauge(GAUGE_EASY, gauge_values, starting_gauge))
        return GAUGE_EASY;

    // either not clearing on any gauge, or the player wants to use assisted easy
    return bottom_shiftable_gauge;
}

void calculate_gauge_judge_values(int player, int type, std::uint32_t* gauge_type_ptr, int notes, chart_judgement_t& out)
{
    // override the gauge type (call site is expected to restore this on their own)
    *gauge_type_ptr = type;

    // get the four judgement values
    for (auto i = 0; i < 4; ++i)
        out.values[i] = calculate_individual_chart_judge_value(player, i, notes);
}

// hook functions
void* replacement_calculate_chart_judge(void* a1, void* a2, int a3, int a4, int a5)
{
    // require the correct game type
    auto result = calculate_chart_judge_hook.call<void*>(a1, a2, a3, a4, a5);

    if (!is_valid_game_type())
        return result;

    // some state checks as variables for convenience
    auto dp = (state_ptr->play_style == 1);
    auto p1 = (state_ptr->p1_active == 1);
    auto p2 = (state_ptr->p2_active == 1);

    // initial init for things that aren't available as soon as the game boots
    if (p1_gauge_type_ptr == nullptr || p2_gauge_type_ptr == nullptr)
    {
        p1_gauge_type_ptr = reinterpret_cast<decltype(p1_gauge_type_ptr)>(&(*option_data_ptr)->p1_gauge_type);
        p2_gauge_type_ptr = reinterpret_cast<decltype(p2_gauge_type_ptr)>(&(*option_data_ptr)->p2_gauge_type);
        dp_gauge_type_ptr = reinterpret_cast<decltype(dp_gauge_type_ptr)>(&(*option_data_ptr)->dp_gauge_type);
    }

    // get note count from CStageGameData instead
    auto p1_notes = (*stage_game_data)->p1_note_count;
    auto p2_notes = (*stage_game_data)->p2_note_count;

    // reset to default values
    std::memcpy(p1_gauge_values, default_gauge_values, sizeof(default_gauge_values));
    std::memcpy(p2_gauge_values, default_gauge_values, sizeof(default_gauge_values));

    // zero out all previous graph points
    std::memset(p1_graph_values, 0, sizeof(p1_graph_values));
    std::memset(p2_graph_values, 0, sizeof(p2_graph_values));

    // backup original gauge types
    if (backup_starting_gauge)
    {
        p1_starting_gauge_type = *p1_gauge_type_ptr;
        p2_starting_gauge_type = *p2_gauge_type_ptr;
        dp_starting_gauge_type = *dp_gauge_type_ptr;

        backup_starting_gauge = false;
    }

    // calculate judge values for each gauge type
    for (auto type = 0; type < 5; ++type)
    {
        if (dp)
        {
            // figure out where to put the chart judgements data
            auto& chart_judgements = (p1 ? p1_chart_judgements: p2_chart_judgements);
            calculate_gauge_judge_values((p1 ? 0: 1), type, dp_gauge_type_ptr, (p1 ? p1_notes: p2_notes), chart_judgements[type]);
        }
        if (!dp && p1) calculate_gauge_judge_values(0, type, p1_gauge_type_ptr, p1_notes, p1_chart_judgements[type]);
        if (!dp && p2) calculate_gauge_judge_values(1, type, p2_gauge_type_ptr, p2_notes, p2_chart_judgements[type]);
    }

    // restore original gauge types
    *p1_gauge_type_ptr = p1_starting_gauge_type;
    *p2_gauge_type_ptr = p2_starting_gauge_type;
    *dp_gauge_type_ptr = dp_starting_gauge_type;

    // addresses a bug where starting on a hard gauge, dropping to a normal gauge, then quick retrying
    // would then result in the starting gauge value being 22% despite also being a hard gauge. was mostly
    // a cosmetic issue, since it would jump back to the correct value after the first note, but this fixes it.
    if (p1) *p1_groove_gauge_ptr = p1_gauge_values[*p1_gauge_type_ptr];
    if (p2) *p2_groove_gauge_ptr = p2_gauge_values[*p2_gauge_type_ptr];
    if (dp) (p1 ? *p1_groove_gauge_ptr: *p2_groove_gauge_ptr) = (p1 ? p1_gauge_values: p2_gauge_values)[*dp_gauge_type_ptr];

    // call the original function
    return result;
}

void replacement_update_groove_gauge(int player, int note_judge)
{
    // require the correct game type
    if (!is_valid_game_type())
        return update_groove_gauge_hook.call(player, note_judge);

    // for double play specific stuff
    auto dp = (state_ptr->play_style == 1);

    // per-player specifics
    auto& player_gauge_value_ptr = (player == 0 ? p1_groove_gauge_ptr: p2_groove_gauge_ptr);
    auto& player_gauge_type_ptr = (dp ? dp_gauge_type_ptr: (player == 0 ? p1_gauge_type_ptr: p2_gauge_type_ptr));
    auto& player_gauge_option_ptr = (player == 0 ? p1_gauge_option_ptr: p2_gauge_option_ptr);
    auto& player_chart_judgements = (player == 0 ? p1_chart_judgements: p2_chart_judgements);
    auto& player_chart_judgement_ptr = (player == 0 ? p1_chart_judgement_ptr: p2_chart_judgement_ptr);
    auto& player_starting_gauge = (dp ? dp_starting_gauge_type: (player == 0 ? p1_starting_gauge_type: p2_starting_gauge_type));
    auto& player_gauge_values = (player == 0 ? p1_gauge_values: p2_gauge_values);
    auto& player_graph_values = (player == 0 ? p1_graph_values: p2_graph_values);

    // continue as normal
    auto current_gauge_type = *player_gauge_type_ptr;

    // calculate gauge values for every gauge type
    for (auto type = 5; type-- > 0;)
    {
        // don't bother if we're dead on EX HARD or HARD gauges
        if ((type == GAUGE_EX_HARD && (player_gauge_values[GAUGE_EX_HARD] < (2 * 50) || player_gauge_values[GAUGE_EX_HARD] > (100 * 50))))
            continue;
        if ((type == GAUGE_HARD && (player_gauge_values[GAUGE_HARD] < (2 * 50) || player_gauge_values[GAUGE_HARD] > (100 * 50))))
            continue;

        // come back from the dead if we shift from hard gauge to normal
        if (type < GAUGE_HARD && player_gauge_values[type] < (2 * 50))
            player_gauge_values[type] = (2 * 50);

        // switch gauges & judgement values
        *player_gauge_value_ptr = player_gauge_values[type];
        *player_chart_judgement_ptr = player_chart_judgements[type];
        *player_gauge_type_ptr = type;

        // calculate the groove gauge value by calling the original
        update_groove_gauge_hook.call(player, note_judge);

        // take note of it
        auto updated_gauge_value = *player_gauge_value_ptr;

        if (updated_gauge_value < (2 * 50) || updated_gauge_value > (100 * 50))
            updated_gauge_value = 0;

        player_gauge_values[type] = updated_gauge_value;
    }

    // find the most appropriate gauge type given the newly updated values
    auto gauge_type = get_most_appropriate_gauge(player_gauge_values, player_starting_gauge);
    spdlog::debug("get_most_appropriate_gauge: {}", gauge_names[gauge_type]);

    if (current_gauge_type != gauge_type)
    {
        spdlog::info("switched player {} from {} gauge to {} gauge",
            player + 1, gauge_names[current_gauge_type], gauge_names[gauge_type]);

        // allow "death" in non-hard gauges
        if (gauge_type < GAUGE_HARD)
        {
            if (player_gauge_values[gauge_type] < (2 * 50))
                player_gauge_values[gauge_type] = (2 * 50);
        }
    }

    // update to most appropriate values
    *player_gauge_type_ptr = gauge_type;
    *player_gauge_option_ptr = gauge_type;
    *player_chart_judgement_ptr = player_chart_judgements[gauge_type];
    *player_gauge_value_ptr = player_gauge_values[gauge_type];
}

void replacement_update_graph_data(SafetyHookContext& ctx)
{
    if (!is_valid_game_type())
        return;

    // current gauge point
    auto index = gauge_data_ptr->gauge_point_count;

    for (auto player = 0; player < 2; ++player)
    {
        if ((player == 0 && !state_ptr->p1_active) || (player == 1 && !state_ptr->p2_active))
            continue;

        auto& player_gauge_values = (player == 0 ? p1_gauge_values: p2_gauge_values);
        auto& player_graph_values = (player == 0 ? p1_graph_values: p2_graph_values);
        auto& player_dead_measure_ptr = (player == 0 ? p1_dead_measure_ptr: p2_dead_measure_ptr);

        // unset dead measure if we shifted gauge
        if (*player_dead_measure_ptr != 0)
        {
            auto player_gauge_type = *(state_ptr->play_style == 1 ? dp_gauge_type_ptr:
                                       (player == 0 ? p1_gauge_type_ptr: p2_gauge_type_ptr));

            if (player_gauge_values[player_gauge_type] >= (2 * 50) &&
                player_gauge_values[player_gauge_type] <= (100 * 50))
            {
                spdlog::debug("unset dead measure: {} -> 0", *player_dead_measure_ptr);
                *player_dead_measure_ptr = 0;
            }
        }

        // take a snapshot of each gauge at this point in time
        for (auto type = 0; type < 5; ++type)
        {
            if (player_gauge_values[type] > (100 * 50))
                player_graph_values[type][index] = 0;
            else
                player_graph_values[type][index] = player_gauge_values[type];

            spdlog::debug("stored point {} for gauge {}: {}",
                index, gauge_names[type], player_gauge_values[type] / 50);
        }
    }
}

void copy_result_graph(std::int32_t* destination, std::int16_t* source)
{
    for (int i = 0; i < GAUGE_POINTS; i++)
    {
        destination[i] = source[i];
    }
}

void* replacement_stage_result_ctor(void* a1)
{
    // require the correct game type
    if (!is_valid_game_type())
        return stage_result_ctor_hook.call<void*>(a1);

    // some state checks as variables for convenience
    auto dp = (state_ptr->play_style == 1);
    auto p1 = (state_ptr->p1_active == 1);
    auto p2 = (state_ptr->p2_active == 1);

    // get the currently active gauge(s)
    if (dp)
    {
        spdlog::debug("writing stored DP gauge points to graph");

        auto gauge_type = *dp_gauge_type_ptr;
        auto graph_values = (p1 ? p1_graph_values: p2_graph_values);
        auto graph_data = (p1 ? &graph_data_ptr->p1_points: &graph_data_ptr->p2_points);

        std::memcpy(graph_data, graph_values[gauge_type], sizeof(std::uint16_t[GAUGE_POINTS]));
    }
    else
    {
        if (p1)
        {
            spdlog::debug("writing stored P1 gauge points to graph");

            auto gauge_type = *p1_gauge_type_ptr;
            std::memcpy(graph_data_ptr->p1_points, p1_graph_values[gauge_type], sizeof(std::uint16_t[GAUGE_POINTS]));
        }

        if (p2)
        {
            spdlog::debug("writing stored P2 gauge points to graph");

            auto gauge_type = *p2_gauge_type_ptr;
            std::memcpy(graph_data_ptr->p2_points, p2_graph_values[gauge_type], sizeof(std::uint16_t[GAUGE_POINTS]));
        }
    }

    return stage_result_ctor_hook.call<void*>(a1);
}

void* replacement_result_graph_render(StageResultDrawGraph* graph)
{
    // require the correct game type
    if (!is_valid_game_type())
        return result_graph_render_hook.call<void*>(graph);

    // poll for vefx button to cycle between other gauge graphs
    static auto last_input = std::bitset<64>();
    auto input = std::bitset<64>(*input_ptr);

    if ((input.test(33) && !last_input.test(33)) ||  // E2   (Controller)
        (input.test(17) && !last_input.test(17)))    // TAB  (Keyboard)
    {
        // update the gauge type on the graph & copy the graph points for this gauge
        if (state_ptr->play_style == 1)
        {
            auto dp_next_gauge_type = (*dp_gauge_type_ptr + 1);

            if (dp_next_gauge_type > GAUGE_EX_HARD)
                dp_next_gauge_type = GAUGE_NORMAL;

            *dp_gauge_type_ptr = dp_next_gauge_type;

            auto graph_data = (state_ptr->p1_active ? graph->p1_graph_points : graph->p2_graph_points);
            auto& values = (state_ptr->p1_active ? p1_graph_values: p2_graph_values);

            copy_result_graph(graph_data, values[dp_next_gauge_type]);
        }
        else
        {
            auto p1_next_gauge_type = (*p1_gauge_type_ptr + 1);
            auto p2_next_gauge_type = (*p2_gauge_type_ptr + 1);

            if (p1_next_gauge_type > GAUGE_EX_HARD)
                p1_next_gauge_type = GAUGE_NORMAL;
            if (p2_next_gauge_type > GAUGE_EX_HARD)
                p2_next_gauge_type = GAUGE_NORMAL;

            *p1_gauge_type_ptr = p1_next_gauge_type;
            *p2_gauge_type_ptr = p2_next_gauge_type;

            copy_result_graph(graph->p1_graph_points, p1_graph_values[p1_next_gauge_type]);
            copy_result_graph(graph->p2_graph_points, p2_graph_values[p2_next_gauge_type]);
        }
    }

    last_input = input;

    return result_graph_render_hook.call<void*>(graph);
}

void* replacement_return_from_result(void* a1)
{
    // require the correct game type
    if (!is_valid_game_type())
        return return_from_result_hook.call<void*>(a1);

    // restore the original gauge types for each player
    *p1_gauge_type_ptr = p1_starting_gauge_type;
    *p2_gauge_type_ptr = p2_starting_gauge_type;
    *dp_gauge_type_ptr = dp_starting_gauge_type;

    // allow backing up of original gauge types again
    backup_starting_gauge = true;

    return return_from_result_hook.call<void*>(a1);
}
#include <bitset>
#include "hooks.h"
#include "config.h"
#include "bm2dx.h"

// application state
SafetyHookInline calculate_chart_judge_hook;
SafetyHookInline update_groove_gauge_hook;
SafetyHookInline update_graph_data_hook;
SafetyHookInline draw_graph_ctor_hook;
SafetyHookInline result_graph_render_hook;
SafetyHookInline return_from_result_hook;

bool backup_starting_gauge = true;

gauge_type p1_gauge_type(0, 0);
gauge_type p2_gauge_type(1, 0);
gauge_type dp_gauge_type(0, 1);

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
    auto practice_trigger = false;

    // step up dan practice check
    static auto is_practice_fn = reinterpret_cast<bool (*) (void*, int, int)>(offsets::is_dan_practice_fn);
    static auto in_practice = false;

    auto p1_is_practice = is_practice_fn(*option_data_ptr, 0, 0);
    auto p2_is_practice = is_practice_fn(*option_data_ptr, 1, 0);
    auto dp_is_practice = is_practice_fn(*option_data_ptr, 0, 1);

    auto is_practice = (p1_is_practice || p2_is_practice || dp_is_practice);

    if (is_practice && !in_practice)
    {
        in_practice = true;
        set_death_defying_state(false);
        spdlog::debug("gauge shift disabled on step up dan practice");
        return false;
    }

    if (in_practice)
    {
        if (!is_practice)
        {
            in_practice = false;
            spdlog::debug("no longer in step up dan practice");
            practice_trigger = true;
        }
        else
        {
            return false;
        }
    }

    static auto patch_enabled = false;

    auto game_type = state_ptr->game_type;
    auto patch_state = (game_type == 5 || game_type == 6); // STEP UP, PREMIUM FREE

    // enable and disable the patch depending on the game type & play style
    static auto last_game_type = -1;

    if (last_game_type != game_type || practice_trigger)
    {
        if (patch_enabled != patch_state || practice_trigger)
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

void calculate_gauge_judge_values(int player, int type, gauge_type& gauge, int notes, chart_judgement_t& out)
{
    // override the gauge type (call site is expected to restore this on their own)
    gauge.set(type);

    // get the four judgement values
    for (auto i = 0; i < 4; ++i)
        out.values[i] = calculate_individual_chart_judge_value(player, i, notes);
}

// hook functions
void* replacement_calculate_chart_judge(std::int64_t p1_notes, std::int64_t p2_notes)
{
    // require the correct game type
    auto result = calculate_chart_judge_hook.call<void*>(p1_notes, p2_notes);

    if (!is_valid_game_type())
        return result;

    // some state checks as variables for convenience
    auto dp = (state_ptr->play_style == 1);
    auto p1 = (state_ptr->p1_active == 1);
    auto p2 = (state_ptr->p2_active == 1);

    // reset to default values
    std::memcpy(p1_gauge_values, default_gauge_values, sizeof(default_gauge_values));
    std::memcpy(p2_gauge_values, default_gauge_values, sizeof(default_gauge_values));

    // zero out all previous graph points
    std::memset(p1_graph_values, 0, sizeof(p1_graph_values));
    std::memset(p2_graph_values, 0, sizeof(p2_graph_values));

    // backup original gauge types
    if (backup_starting_gauge)
    {
        p1_starting_gauge_type = p1_gauge_type.get();
        p2_starting_gauge_type = p2_gauge_type.get();
        dp_starting_gauge_type = dp_gauge_type.get();

        backup_starting_gauge = false;
    }

    // calculate judge values for each gauge type
    for (auto type = 0; type < 5; ++type)
    {
        if (dp)
        {
            // figure out where to put the chart judgements data
            auto& chart_judgements = (p1 ? p1_chart_judgements: p2_chart_judgements);
            calculate_gauge_judge_values((p1 ? 0: 1), type, dp_gauge_type, (p1 ? p1_notes: p2_notes), chart_judgements[type]);
        }
        if (!dp && p1) calculate_gauge_judge_values(0, type, p1_gauge_type, p1_notes, p1_chart_judgements[type]);
        if (!dp && p2) calculate_gauge_judge_values(1, type, p2_gauge_type, p2_notes, p2_chart_judgements[type]);
    }

    // restore original gauge types
    p1_gauge_type.set(p1_starting_gauge_type);
    p2_gauge_type.set(p2_starting_gauge_type);
    dp_gauge_type.set(dp_starting_gauge_type);

    // addresses a bug where starting on a hard gauge, dropping to a normal gauge, then quick retrying
    // would then result in the starting gauge value being 22% despite also being a hard gauge. was mostly
    // a cosmetic issue, since it would jump back to the correct value after the first note, but this fixes it.
    if (p1) *p1_groove_gauge_ptr = p1_gauge_values[p1_gauge_type.get()];
    if (p2) *p2_groove_gauge_ptr = p2_gauge_values[p2_gauge_type.get()];
    if (dp) (p1 ? *p1_groove_gauge_ptr: *p2_groove_gauge_ptr) = (p1 ? p1_gauge_values: p2_gauge_values)[dp_gauge_type.get()];

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
    auto& player_gauge_type = (dp ? dp_gauge_type: (player == 0 ? p1_gauge_type: p2_gauge_type));
    auto& player_gauge_option_ptr = (player == 0 ? p1_gauge_option_ptr: p2_gauge_option_ptr);
    auto& player_chart_judgements = (player == 0 ? p1_chart_judgements: p2_chart_judgements);
    auto& player_chart_judgement_ptr = (player == 0 ? p1_chart_judgement_ptr: p2_chart_judgement_ptr);
    auto& player_starting_gauge = (dp ? dp_starting_gauge_type: (player == 0 ? p1_starting_gauge_type: p2_starting_gauge_type));
    auto& player_gauge_values = (player == 0 ? p1_gauge_values: p2_gauge_values);
    auto& player_graph_values = (player == 0 ? p1_graph_values: p2_graph_values);

    // continue as normal
    auto current_gauge_type = player_gauge_type.get();

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
        player_gauge_type.set(type);

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
    player_gauge_type.set(gauge_type);
    *player_gauge_option_ptr = gauge_type;
    *player_chart_judgement_ptr = player_chart_judgements[gauge_type];
    *player_gauge_value_ptr = player_gauge_values[gauge_type];
}

void* replacement_update_graph_data(void* a1, std::int16_t a2, std::int16_t a3)
{
    // require the correct game type
    auto result = update_graph_data_hook.call<void*>(a1, a2, a3);

    if (!is_valid_game_type())
        return result;

    // current gauge point
    static auto last_index = -1;
    auto index = gauge_t::gauge_point_count();

    if (index == last_index)
        return result;

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
            auto player_gauge_type = (state_ptr->play_style == 1 ? dp_gauge_type.get():
                                      (player == 0 ? p1_gauge_type.get(): p2_gauge_type.get()));

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

            spdlog::debug("stored point {} for P{} gauge {}: {}",
                index, player + 1, gauge_names[type], player_gauge_values[type] / 50);
        }
    }

    // call the original function
    return result;
}

void copy_result_graph(std::int32_t* destination, std::int16_t* source)
{
    for (int i = 0; i < GAUGE_POINTS; i++)
    {
        destination[i] = source[i];
    }
}

void* replacement_draw_graph_ctor(void* a1)
{
    // require the correct game type
    if (!is_valid_game_type())
        return draw_graph_ctor_hook.call<void*>(a1);

    auto result = draw_graph_ctor_hook.call<void*>(a1);

    // some state checks as variables for convenience
    auto dp = (state_ptr->play_style == 1);
    auto p1 = (state_ptr->p1_active == 1);
    auto p2 = (state_ptr->p2_active == 1);

    // get the currently active gauge(s)
    if (dp)
    {
        spdlog::debug("writing stored DP gauge points to graph");

        auto gauge_type = dp_gauge_type.get();
        auto graph_values = (p1 ? p1_graph_values: p2_graph_values);
        auto graph_data = (p1 ? gauge_t::p1_ghost_gauge(): gauge_t::p2_ghost_gauge());
        auto result_graph_data = (p1 ? p1_result_graph_ptr: p2_result_graph_ptr);

        std::memcpy(&graph_data->values, graph_values[gauge_type], sizeof(gauge_t::ghost_gauge_t));
        copy_result_graph(result_graph_data, graph_values[gauge_type]);
    }
    else
    {
        if (p1)
        {
            spdlog::debug("writing stored P1 gauge points to graph");

            auto gauge_type = p1_gauge_type.get();
            std::memcpy(&gauge_t::p1_ghost_gauge()->values, p1_graph_values[gauge_type], sizeof(gauge_t::ghost_gauge_t));
            copy_result_graph(p1_result_graph_ptr, p1_graph_values[gauge_type]);
        }

        if (p2)
        {
            spdlog::debug("writing stored P2 gauge points to graph");

            auto gauge_type = p2_gauge_type.get();
            std::memcpy(&gauge_t::p2_ghost_gauge()->values, p2_graph_values[gauge_type], sizeof(gauge_t::ghost_gauge_t));
            copy_result_graph(p2_result_graph_ptr, p2_graph_values[gauge_type]);
        }
    }

    return result;
}

void* replacement_result_graph_render(void* a1)
{
    // require the correct game type
    if (!is_valid_game_type())
        return result_graph_render_hook.call<void*>(a1);

    // force graph to re-render
    auto state = reinterpret_cast<std::int32_t*>
        (std::bit_cast<std::uintptr_t>(a1) + 0xC10);
    auto original_state = *state;

    // poll for vefx button to cycle between other gauge graphs
    static auto last_input = std::bitset<32>();
    auto input = std::bitset<32>(*input_ptr);

    if (input.test(19) && !last_input.test(19))
    {
        // TODO: can't remember why this is here
        *state = 1;

        // update the gauge type on the graph & copy the graph points for this gauge
        if (state_ptr->play_style == 1)
        {
            auto dp_next_gauge_type = (dp_gauge_type.get() + 1);

            if (dp_next_gauge_type > GAUGE_EX_HARD)
                dp_next_gauge_type = GAUGE_NORMAL;

            dp_gauge_type.set(dp_next_gauge_type);

            auto graph_data = (state_ptr->p1_active ? p1_result_graph_ptr: p2_result_graph_ptr);
            auto& values = (state_ptr->p1_active ? p1_graph_values: p2_graph_values);

            copy_result_graph(graph_data, values[dp_next_gauge_type]);
        }
        else
        {
            auto p1_next_gauge_type = (p1_gauge_type.get() + 1);
            auto p2_next_gauge_type = (p2_gauge_type.get() + 1);

            if (p1_next_gauge_type > GAUGE_EX_HARD)
                p1_next_gauge_type = GAUGE_NORMAL;
            if (p2_next_gauge_type > GAUGE_EX_HARD)
                p2_next_gauge_type = GAUGE_NORMAL;

            p1_gauge_type.set(p1_next_gauge_type);
            p2_gauge_type.set(p2_next_gauge_type);

            copy_result_graph(p1_result_graph_ptr, p1_graph_values[p1_next_gauge_type]);
            copy_result_graph(p2_result_graph_ptr, p2_graph_values[p2_next_gauge_type]);
        }
    }

    last_input = input;

    auto result = result_graph_render_hook.call<void*>(a1);

    *state = original_state;

    return result;
}

void* replacement_return_from_result(void* a1)
{
    // require the correct game type
    if (!is_valid_game_type())
        return return_from_result_hook.call<void*>(a1);

    // restore the original gauge types for each player
    p1_gauge_type.set(p1_starting_gauge_type);
    p2_gauge_type.set(p2_starting_gauge_type);
    dp_gauge_type.set(dp_starting_gauge_type);

    // allow backing up of original gauge types again
    backup_starting_gauge = true;

    return return_from_result_hook.call<void*>(a1);
}
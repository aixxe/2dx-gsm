#pragma once

#include <cstdint>
#include "code_patch.h"
#include "bm2dx_offsets.h"

#define GAUGE_POINTS            384

#define PLAYER_1                0
#define PLAYER_2                1

#define STYLE_DP                1

extern const char* gauge_names[];

enum gauge_types
{
    GAUGE_NORMAL = 0,
    GAUGE_ASSISTED_EASY = 1,
    GAUGE_EASY = 2,
    GAUGE_HARD = 3,
    GAUGE_EX_HARD = 4,
};

struct chart_judgement_t
{
    std::uint16_t values[4];
};

struct COptionGameData;

struct gauge_t
{
    struct ghost_gauge_t
    {
        std::uint16_t values[GAUGE_POINTS];
    }; static_assert(sizeof(ghost_gauge_t) == 0x300);

    auto static p1_ghost_gauge() -> ghost_gauge_t*
        { return reinterpret_cast<ghost_gauge_t*>(offsets::gauge_data_ptr + offsets::gauge_data_ghost_offset); }
    auto static p2_ghost_gauge() -> ghost_gauge_t*
        { return reinterpret_cast<ghost_gauge_t*>(offsets::gauge_data_ptr + offsets::gauge_data_player_offset + offsets::gauge_data_ghost_offset); }
    auto static gauge_point_count() -> std::int16_t
        { return *reinterpret_cast<std::int16_t*>(offsets::gauge_data_ptr + offsets::gauge_data_count_offset); }
};

struct state_t
{
    /* 0x0000 */ std::int32_t game_type;
    /* 0x0004 */ std::int32_t play_style;
    /* 0x0008 */ std::int32_t p1_difficulty;
    /* 0x000C */ std::int32_t p2_difficulty;
    /* 0x0010 */ std::int32_t p1_active;
    /* 0x0014 */ std::int32_t p2_active;
    /* 0x0018 */ std::uint8_t pad_0018[24];
    /* 0x0030 */ void* active_music;
}; static_assert(sizeof(state_t) == 0x38);

extern state_t* state_ptr;
extern COptionGameData** option_data_ptr;

extern std::uint32_t (*get_gauge_fn) (COptionGameData*, int, int);
extern void* (*set_gauge_fn) (COptionGameData*, int, std::uint32_t, int);

extern std::uint32_t* input_ptr;

extern std::int16_t* p1_groove_gauge_ptr;
extern std::int16_t* p2_groove_gauge_ptr;

extern std::int32_t* p1_result_graph_ptr;
extern std::int32_t* p2_result_graph_ptr;

extern chart_judgement_t* p1_chart_judgement_ptr;
extern chart_judgement_t* p2_chart_judgement_ptr;

extern std::uint32_t* p1_gauge_option_ptr;
extern std::uint32_t* p2_gauge_option_ptr;

extern std::uint32_t* p1_dead_measure_ptr;
extern std::uint32_t* p2_dead_measure_ptr;

extern std::int64_t (*calculate_individual_chart_judge_value) (int, int, int);

extern std::unique_ptr<util::code_patch> death_defying_patch;

void iidx_gsm_load(HMODULE bm2dx);
#pragma once

#include <cstdint>
#include "code_patch.h"

#define GAUGE_POINTS            640

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

struct COptionGameData
{
    /* 0x0000 */ char pad_0000[76];
    /* 0x004C */ int32_t p1_gauge_type;
    /* 0x0050 */ char pad_0050[148];
    /* 0x00E4 */ int32_t p2_gauge_type;
    /* 0x00E8 */ char pad_00E8[148];
    /* 0x017C */ int32_t dp_gauge_type;
}; static_assert(sizeof(COptionGameData) == 0x180);

struct StageResultDrawGraph
{
    /* 0x0000 */ std::uint8_t pad_0000[196];
    /* 0x00C4 */ std::int32_t p1_graph_points_count;
    /* 0x00C8 */ std::int32_t p2_graph_points_count;
    /* 0x00CC */ std::int32_t p1_graph_points[GAUGE_POINTS];
    /* 0x0ACC */ std::int32_t p2_graph_points[GAUGE_POINTS];
}; static_assert(sizeof(StageResultDrawGraph) == 0x14CC);

struct gauge_t
{
    /* 0x0000 */ std::int32_t p1_ex_score;
    /* 0x0004 */ std::uint8_t pad_0004[4];
    /* 0x0008 */ std::int32_t p1_last_note;
    /* 0x000C */ std::int32_t p1_note_count;
    /* 0x0010 */ std::uint8_t pad_0010[1];
    /* 0x0011 */ std::int8_t p1_ghost[64];
    /* 0x0051 */ std::uint8_t pad_0051[1];
    /* 0x0052 */ std::int16_t p1_ghost_gauge[GAUGE_POINTS];
    /* 0x0552 */ std::int16_t gauge_point_count;
    /* 0x0554 */ std::uint8_t pad_0554[8];
    /* 0x055C */ std::int32_t p2_ex_score;
    /* 0x0560 */ std::uint8_t pad_0560[4];
    /* 0x0564 */ std::int32_t p2_last_note;
    /* 0x0568 */ std::int32_t p2_note_count;
    /* 0x056C */ std::uint8_t pad_056C[1];
    /* 0x056D */ std::int8_t p2_ghost[64];
    /* 0x05AD */ std::uint8_t pad_05AD[1];
    /* 0x05AE */ std::int16_t p2_ghost_gauge[GAUGE_POINTS];
    /* 0x0AAE */ std::uint8_t pad_0AAE[2];
}; static_assert(sizeof(gauge_t) == 0xAB0);

struct graph_t
{
    /* 0x0000 */ std::int16_t p1_points[GAUGE_POINTS];
    /* 0x0500 */ std::uint8_t pad_0500[92];
    /* 0x055C */ std::int16_t p2_points[GAUGE_POINTS];
};

struct CStageGameData
{
    /* 0x0000 */ char pad_0000[8];
    /* 0x0008 */ int32_t p1_note_count;
    /* 0x000C */ int32_t p2_note_count;
}; static_assert(sizeof(CStageGameData) == 0x10);

struct state_t
{
    /* 0x0000 */ std::int32_t game_type;
    /* 0x0004 */ std::int32_t play_style;
    /* 0x0008 */ std::uint8_t pad_0008[4];
    /* 0x000C */ std::int32_t p1_difficulty;
    /* 0x0010 */ std::int32_t p2_difficulty;
    /* 0x0014 */ std::int32_t p1_active;
    /* 0x0018 */ std::int32_t p2_active;
    /* 0x001C */ std::uint8_t pad_001C[4];
    /* 0x0020 */ void* active_music;
}; static_assert(sizeof(state_t) == 0x28);

extern state_t* state_ptr;
extern gauge_t* gauge_data_ptr;
extern graph_t* graph_data_ptr;
extern COptionGameData** option_data_ptr;
extern CStageGameData** stage_game_data;

extern std::uint64_t* input_ptr;

extern std::int16_t* p1_groove_gauge_ptr;
extern std::int16_t* p2_groove_gauge_ptr;

extern chart_judgement_t* p1_chart_judgement_ptr;
extern chart_judgement_t* p2_chart_judgement_ptr;

extern std::uint32_t* p1_gauge_option_ptr;
extern std::uint32_t* p2_gauge_option_ptr;

extern std::uint32_t* p1_dead_measure_ptr;
extern std::uint32_t* p2_dead_measure_ptr;

extern std::uint32_t* p1_gauge_type_ptr;
extern std::uint32_t* p2_gauge_type_ptr;
extern std::uint32_t* dp_gauge_type_ptr;

extern std::int64_t (*calculate_individual_chart_judge_value) (int, int, int);

extern std::unique_ptr<util::code_patch> death_defying_patch;

void iidx_gsm_load(HMODULE bm2dx);
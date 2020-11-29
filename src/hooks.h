#pragma once

#include <safetyhook.hpp>
#include "bm2dx.h"

extern SafetyHookInline calculate_chart_judge_hook;
extern SafetyHookInline update_groove_gauge_hook;
extern SafetyHookInline stage_result_ctor_hook;
extern SafetyHookInline result_graph_render_hook;
extern SafetyHookInline return_from_result_hook;
extern SafetyHookMid update_graph_data_hook;

extern void* replacement_calculate_chart_judge(void*, void*, int, int, int);
extern void replacement_update_groove_gauge(std::int32_t, std::int32_t);
extern void replacement_update_graph_data(SafetyHookContext& ctx);
extern void* replacement_stage_result_ctor(void*);
extern void* replacement_result_graph_render(StageResultDrawGraph*);
extern void* replacement_return_from_result(void*);
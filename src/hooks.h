#pragma once

#include <safetyhook.hpp>
#include "bm2dx.h"

class gauge_type
{
    public:
        gauge_type(const int player, const int style):
            _player(player), _style(style) {}

        [[nodiscard]] auto get() const
            { return get_gauge_fn(*option_data_ptr, _player, _style); }

        auto set(const std::uint32_t type) const
            { set_gauge_fn(*option_data_ptr, _player, type, _style); }
    private:
        int _player;
        int _style;
};

extern SafetyHookInline calculate_chart_judge_hook;
extern SafetyHookInline update_groove_gauge_hook;
extern SafetyHookInline update_graph_data_hook;
extern SafetyHookInline draw_graph_ctor_hook;
extern SafetyHookInline result_graph_render_hook;
extern SafetyHookInline return_from_result_hook;
extern SafetyHookInline quick_retry_hook;

extern void* replacement_calculate_chart_judge (std::int64_t, std::int64_t);
extern void replacement_update_groove_gauge (std::int32_t, std::int32_t);
extern void* replacement_update_graph_data (void*, std::int16_t, std::int16_t);
extern void* replacement_draw_graph_ctor (void*);
extern void* replacement_result_graph_render (void*);
extern void* replacement_return_from_result (void*);
extern std::int8_t replacement_quick_retry (std::int64_t);

extern void hijack_gauge_textures(safetyhook::Context&);
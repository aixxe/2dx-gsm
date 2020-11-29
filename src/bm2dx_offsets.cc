#include <version.h>
#include "bm2dx_offsets.h"

auto static binary_support = std::vector<binary_info*>{};

void register_binary_info(binary_info* info)
    { binary_support.push_back(info); }

namespace offsets
{
    binary_info* active = nullptr;

    std::uintptr_t death_defying_patch = 0;

    std::uintptr_t state_ptr = 0;
    std::uintptr_t gauge_data_ptr = 0;
    std::uintptr_t graph_data_ptr = 0;
    std::uintptr_t option_data_ptr = 0;
    std::uintptr_t stage_game_data = 0;

    std::uintptr_t input_ptr = 0;

    std::uintptr_t p1_groove_gauge_ptr = 0;
    std::uintptr_t p2_groove_gauge_ptr = 0;

    std::uintptr_t p1_chart_judgement_ptr = 0;
    std::uintptr_t p2_chart_judgement_ptr = 0;

    std::uintptr_t p1_gauge_option_ptr = 0;
    std::uintptr_t p2_gauge_option_ptr = 0;

    std::uintptr_t p1_dead_measure_ptr = 0;
    std::uintptr_t p2_dead_measure_ptr = 0;

    std::uintptr_t calculate_individual_chart_judge_value = 0;

    std::uintptr_t target_calculate_chart_judge = 0;
    std::uintptr_t target_update_groove_gauge = 0;
    std::uintptr_t target_update_graph_data = 0;
    std::uintptr_t target_stage_result_ctor = 0;
    std::uintptr_t target_result_graph_render = 0;
    std::uintptr_t target_return_from_result = 0;

    void resolve(HMODULE base)
    {
        auto const start = reinterpret_cast<std::uintptr_t>(base);

        auto const dos = reinterpret_cast<PIMAGE_DOS_HEADER>(start);
        auto const nt = reinterpret_cast<PIMAGE_NT_HEADERS>(start + dos->e_lfanew);

        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
            throw std::runtime_error("invalid dos signature");

        if (nt->Signature != IMAGE_NT_SIGNATURE)
            throw std::runtime_error("invalid nt signature");

        spdlog::info("checking game version... ({} supported)", binary_support.size());

        for (auto const& info: binary_support)
        {
            if (nt->OptionalHeader.SizeOfCode != info->nt_code_size)
                continue;

            if (nt->OptionalHeader.AddressOfEntryPoint != info->nt_entrypoint)
                continue;

            if (nt->OptionalHeader.SizeOfImage != info->nt_image_size)
                continue;

            spdlog::info("matched version {}!", info->product_version);

            active = info;
        }

        if (active == nullptr)
            throw std::runtime_error("unsupported game version");

        spdlog::info("preflight checks passed, let's rock!");

        active->resolve_rva(start);
    }
}
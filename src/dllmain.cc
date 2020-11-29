#include "bm2dx.h"
#include "config.h"

BOOL APIENTRY DllMain(HMODULE dll_instance, DWORD reason, LPVOID)
{
    if (reason != DLL_PROCESS_ATTACH)
        return TRUE;

    try
    {
        init_config(dll_instance);

        auto game_module = GetModuleHandleA(app_cfg.module.c_str());

        // for infinitas, we can just use the host module
        if (game_module == nullptr)
            game_module = GetModuleHandleA(nullptr);

        iidx_gsm_load(game_module);
    }
    catch (const std::exception& error)
    {
        spdlog::error("init error: {}", error.what());
        return FALSE;
    }

    return TRUE;
}
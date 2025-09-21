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

        if (game_module == nullptr)
            throw std::runtime_error("could not find game module");

        iidx_gsm_load(game_module);
    }
    catch (const std::exception& error)
    {
        spdlog::error("init error: {}", error.what());
        return FALSE;
    }

    return TRUE;
}
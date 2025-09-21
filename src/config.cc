#include <filesystem>
#include "bm2dx.h"
#include "config.h"

config_t app_cfg;

inline gauge_types bottom_shiftable_gauge_value_to_enum(const wchar_t* value)
{
    if (wcscmp(value, L"NORMAL") == 0)
        return GAUGE_NORMAL;
    if (wcscmp(value, L"EASY") == 0)
        return GAUGE_EASY;
    return GAUGE_ASSISTED_EASY; // default
}

std::string config_get_module(const std::filesystem::path& path)
{
    char value[MAX_PATH] = {0};
    GetPrivateProfileStringA("Debug", "Module", "", (LPSTR) &value, MAX_PATH, path.string().c_str());

    // user value
    if (!std::string_view(value).empty())
        return value;

    // common arcade names
    if (GetModuleHandleA("bm2dx_omni.dll") != nullptr)
        return "bm2dx_omni.dll";

    return "bm2dx.dll";
}

bool config_get_verbose(const std::filesystem::path& path)
{
    wchar_t value[32] = {0};
    GetPrivateProfileString(L"Debug", L"Verbose", L"False", (LPWSTR) &value, 32, path.wstring().c_str());

    return std::wstring_view(value) == L"True";
}

void config_get_bottom_shiftable_gauge(const std::filesystem::path& path, std::uint8_t& result_sp, std::uint8_t& result_dp)
{
    // read values from file
    wchar_t value_sp[32] = {0};
    wchar_t value_dp[32] = {0};

    GetPrivateProfileString(L"Gauge", L"Bottom Shiftable Gauge", L"ASSISTED EASY", (LPWSTR) &value_sp, 32, path.wstring().c_str());
    GetPrivateProfileString(L"Gauge", L"Bottom Shiftable Gauge (DP)", L"ASSISTED EASY", (LPWSTR) &value_dp, 32, path.wstring().c_str());

    result_sp = bottom_shiftable_gauge_value_to_enum(value_sp);
    spdlog::info("set: bottom_shiftable_gauge_sp = {}", gauge_names[result_sp]);

    result_dp = bottom_shiftable_gauge_value_to_enum(value_dp);
    spdlog::info("set: bottom_shiftable_gauge_dp = {}", gauge_names[result_dp]);
}

bool config_get_easy_gauge_textures(const std::filesystem::path& path)
{
    wchar_t value[32] = {0};
    GetPrivateProfileString(L"Extras", L"Use Easy Gauge Textures", L"False", (LPWSTR) &value, 32, path.wstring().c_str());

    return std::wstring_view(value) == L"True";
}

void init_config(HMODULE module)
{
    // get directory of module
    wchar_t module_dir[MAX_PATH] = {0};
    GetModuleFileNameW(module, module_dir, MAX_PATH);

    // determine path of config file
    auto cwd = std::filesystem::path(module_dir).remove_filename();
    auto config_file = cwd.append("2dx-gsm.ini");

    // enable verbose
    if (config_get_verbose(config_file))
        spdlog::set_level(spdlog::level::debug);

    // read values
    app_cfg.module = config_get_module(config_file);
    spdlog::info("set 'module' to '{}'...", app_cfg.module);

    app_cfg.use_easy_gauge_textures = config_get_easy_gauge_textures(config_file);
    spdlog::info("set 'use_easy_gauge_textures' to '{}'...", app_cfg.use_easy_gauge_textures);

    config_get_bottom_shiftable_gauge(config_file, app_cfg.bottom_shiftable_gauge_sp, app_cfg.bottom_shiftable_gauge_dp);
}

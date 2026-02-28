#pragma once
// Logging
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/spdlog.h>
// Version
using namespace std::literals;
// --- CommonLibF4 ---
// Main
// clang-format off
#include "F4SE/Impl/PCH.h"
#include "REL/REL.h"
#include "F4SE/API.h"
// clang-format on
#include "F4SE/F4SE.h"
#include "F4SE/Interfaces.h"
#include "F4SE/Logger.h"
#include "F4SE/Trampoline.h"
#include "F4SE/Version.h"
// General Fallout4 includes
#include "RE/Fallout.h"

// --- Windows ---
#define WIN32_LEAN_AND_MEAN
#include <ShlObj.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <functional>
#include <map>
#include <new>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>
#include <windows.h>

// Graphics
#include "REX/W32/DXGI.h"
#include "REX/W32/DXGI_6.h"
#include <initguid.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// threads and synchronization
#include <mutex>
#include <shared_mutex>

// UI
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

// --- Fixes ---
// STD FORMATTER RE::BSFixedString
#include <format>
template <> struct std::formatter<RE::BSFixedString>
{
    template <typename ParseContext> constexpr auto parse(ParseContext &a_ctx)
    {
        return a_ctx.begin();
    }
    template <typename FormatContext> auto format(const RE::BSFixedString &a_str, FormatContext &a_ctx) const
    {
        return std::format_to(a_ctx.out(), "{}", a_str.c_str());
    }
};
#pragma once
#include <PCH.h>
#include <Plugin.h>

// Global logger pointer
extern std::shared_ptr<spdlog::logger> gLog;

// Global data handler
extern RE::TESDataHandler* g_dataHandle;
// Global messaging interface
extern const F4SE::MessagingInterface* g_messaging;
// Global task interface for scheduling tasks on the main thread
extern const F4SE::TaskInterface* g_taskInterface;
// Global imgui state flag
extern bool g_imguiInitialized;

// Global module name
extern std::string g_moduleName;
// Global ini file content
extern const char* defaultIni;
// Flash replacement shader HLSL file path
extern const char* flashPixelShaderHLSL;
// Global compiled flash shader
extern REX::W32::ID3D11PixelShader* g_flashPixelShader;
// Global plugin path
extern std::filesystem::path g_pluginPath;
// Global debug flag
extern bool DEBUGGING;
// Custom buffer update flag
extern bool CUSTOMBUFFER_ON;
// Custom resource view slot in shader
extern UINT CUSTOMBUFFER_SLOT;
// Global development flag
extern bool DEVELOPMENT;
// Dev GUI flag
extern bool DEVGUI_ON;
// Dev GUI Width
extern int DEVGUI_WIDTH;
// Dev GUI Height
extern int DEVGUI_HEIGHT;
// Dev GUI Opacity
extern float DEVGUI_OPACITY;
// Shader definitions from INI
extern ShaderDefDB g_shaderDefinitions;
// Global original shader database
extern ShaderDB g_ShaderDB;
// Global shader include path
extern std::filesystem::path g_commonShaderHeaderPath;
// Global custom buffer data structure instance for updating CB13
extern GFXBoosterAccessData g_customBufferData;
// Global custom resource to pass data to shaders
extern REX::W32::ID3D11Buffer* g_customSRVBuffer;
extern REX::W32::ID3D11ShaderResourceView* g_customSRV;
// Global flag if an INI reload is queued
extern std::atomic<bool> g_reloadQueued;

// Helper function to convert string to lowercase
inline std::string ToLower(const std::string& str) {
    std::string out = str;
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

// REX Logging Compatibility
#undef ERROR
namespace REX
{
    template <class... Args> void INFO(spdlog::format_string_t<Args...> a_fmt, Args &&...a_args)
    {
        gLog->info(a_fmt, std::forward<Args>(a_args)...);
    }
    template <class... Args> void WARN(spdlog::format_string_t<Args...> a_fmt, Args &&...a_args)
    {
        gLog->warn(a_fmt, std::forward<Args>(a_args)...);
    }
} // namespace REX

// GetRenderData() wrapper for 1.11.191 portability
namespace RE::BSGraphics {
    [[nodiscard]] inline RendererData* GetRendererData() noexcept {
        return RendererData::GetSingleton();
    }
}

// GetHealthPerc() wrapper for 1.11.191 portability
#ifndef GetHealthPerc
#  define GetHealthPerc GetHealthPercent
#endif

// --- Classes ---

// HLSL file watcher class to monitor shader files for changes and trigger recompile
class HlslFileWatcher {
private:
    std::filesystem::path filePath;
    std::filesystem::file_time_type lastWriteTime;
    ShaderDefinition* shaderDef;
    std::atomic<bool> running{false};
    std::thread watcherThread;
public:
    HlslFileWatcher(std::filesystem::path path, ShaderDefinition* def)
        : filePath(std::move(path)), shaderDef(def) {
        if (std::filesystem::exists(filePath)) {
            lastWriteTime = std::filesystem::last_write_time(filePath);
        }
    }
    ~HlslFileWatcher() {
        Stop();
    }
    void Start() {
        running = true;
        watcherThread = std::thread([this]() {
            while (running) {
                Check();
                std::this_thread::sleep_for(std::chrono::seconds(1)); // Check every 1 second
            }
        });
    }
    void Stop() {
        running = false;
        if (watcherThread.joinable()) {
            watcherThread.join();
        }
    }
    void Check() {
        try {
            if (!std::filesystem::exists(filePath)) return;
            auto currentTime = std::filesystem::last_write_time(filePath);
            if (currentTime != lastWriteTime) {
                lastWriteTime = currentTime;
                OnFileChanged();
            }
        } catch (...) {
            // Ignore errors during check
        }
    }
private:
    void OnFileChanged() {
        if (!shaderDef) return;
        // Release old compiled shader objects
        if (shaderDef->loadedPixelShader) {
            shaderDef->loadedPixelShader->Release();
            shaderDef->loadedPixelShader = nullptr;
        }
        if (shaderDef->loadedVertexShader) {
            shaderDef->loadedVertexShader->Release();
            shaderDef->loadedVertexShader = nullptr;
        }
        if (shaderDef->compiledShader) {
            shaderDef->compiledShader->Release();
            shaderDef->compiledShader = nullptr;
        }
        // Reset buggy flag to allow recompilation
        shaderDef->buggy = false;
        // Clear all replacement shaders in ShaderDB entries that use this definition
        g_ShaderDB.ClearReplacementsForDefinition(shaderDef);
        REX::INFO("HlslFileWatcher: Shader file '{}' changed, cleared compiled shaders for reload", filePath.string());
    }
};
// Shader.ini file watcher class to monitor shader files for changes and trigger recompile
class ShaderIniFileWatcher {
private:
    std::filesystem::path filePath;
    std::filesystem::file_time_type lastWriteTime;
    std::string folderName;
    std::atomic<bool> running{false};
    std::thread watcherThread;
public:
    ShaderIniFileWatcher(std::filesystem::path path, std::string folder)
        : filePath(std::move(path)), folderName(std::move(folder)) {
        if (std::filesystem::exists(filePath)) {
            lastWriteTime = std::filesystem::last_write_time(filePath);
        }
    }
    ~ShaderIniFileWatcher() {
        Stop();
    }
    void Start() {
        running = true;
        watcherThread = std::thread([this]() {
            while (running) {
                Check();
                std::this_thread::sleep_for(std::chrono::seconds(1)); // Check every 1 second
            }
        });
    }
    void Stop() {
        running = false;
        if (watcherThread.joinable()) {
            watcherThread.join();
        }
    }
    void Check() {
        try {
            if (!std::filesystem::exists(filePath)) return;
            auto currentTime = std::filesystem::last_write_time(filePath);
            if (currentTime != lastWriteTime) {
                lastWriteTime = currentTime;
                OnFileChanged();
            }
        } catch (...) {
            // Ignore errors during check
        }
    }
private:
    void OnFileChanged() {
        if (g_reloadQueued.exchange(true)) {
            // Already queued, skip
            return;
        }
        if (DEBUGGING)
            REX::INFO("ShaderIniFileWatcher: Detected change in '{}', queuing reload...", filePath.string());
        // Queue the reload on the game main thread
        if (g_taskInterface) {
            g_taskInterface->AddTask([]() {
                // Unlock the UI locked shader list to prevent crashes if definitions do not exist anymore
                UIUnlockShaderList_Internal();
                ReloadAllShaderDefinitions_Internal();
                g_reloadQueued = false;  // Reset after reload completes
            });
        } else {
            REX::WARN("ShaderIniFileWatcher: Task interface not available, cannot reload");
        }
    }
};

// Custom include handler for D3DCompile to resolve #include directives relative to the plugin directory
class ShaderIncludeHandler : public ID3DInclude {
public:
    HRESULT __stdcall Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) override {
        std::filesystem::path includePath;
        // Build the include file path
        if (IncludeType == D3D_INCLUDE_LOCAL) {
            // For #include "file.hlsl" - look in common include path
            includePath = g_commonShaderHeaderPath / pFileName;
        } else {
            // For #include <file.hlsl> - also look in common include path
            includePath = g_commonShaderHeaderPath / pFileName;
        }
        // Try to open the file
        std::ifstream file(includePath, std::ios::binary);
        if (!file.good()) {
            REX::WARN("ShaderIncludeHandler: Failed to open include file: {}", includePath.string());
            return E_FAIL;
        }
        // Read file contents
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);
        char* buffer = new char[size];
        file.read(buffer, size);
        file.close();
        *ppData = buffer;
        *pBytes = static_cast<UINT>(size);
        return S_OK;
    }
    HRESULT __stdcall Close(LPCVOID pData) override {
        delete[] static_cast<const char*>(pData);
        return S_OK;
    }
};
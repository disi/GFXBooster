#pragma once
#include <Global.h>

// Forward declaration of HlslFileWatcher for use in ShaderDefinition
class HlslFileWatcher; 
// Forward declaration of g_pluginPath
extern std::filesystem::path g_pluginPath;
// Helper function to convert string to lowercase
std::string ToLower(const std::string& str);

// --- Hooks ---

// -- Structs ---

// Custom t# buffer structure for passing data to shaders
struct alignas(16) GFXBoosterAccessData
{
    float time;        //  0
    float delta;       
    float dayCycle;    
    float frame;       //  3

    float fps;         //  4
    float resX;        
    float resY;        
    float mouseX;      //  7

    float mouseY;      //  8
    float windSpeed;   
    float windAngle;   
    float windTurb;    // 11

    float vpLeft;      // 12
    float vpTop;       
    float vpWidth;     
    float vpHeight;    // 15

    float camX;        // 16
    float camY;        
    float camZ;        
    float pRadDmg;     // 19

    float viewDirX;    // 20
    float viewDirY;    
    float viewDirZ;    

    float pHealthPerc; // 23
    DirectX::XMFLOAT4 g_InvProjRow0; // 24
    DirectX::XMFLOAT4 g_InvProjRow1; // 25
    DirectX::XMFLOAT4 g_InvProjRow2; // 26
    DirectX::XMFLOAT4 g_InvProjRow3; // 27

    float random;    // 28
    float  inCombat;     // 29
    float  inInterior;   // 30
    float _padding;   // 31

    // Forward view-projection rows (needed for SSR world → clip reprojection)
    DirectX::XMFLOAT4 g_ViewProjRow0; // 32
    DirectX::XMFLOAT4 g_ViewProjRow1; // 33
    DirectX::XMFLOAT4 g_ViewProjRow2; // 34
    DirectX::XMFLOAT4 g_ViewProjRow3; // 35

    // Shader settings — offset tracked automatically via kCustomBase = offsetof(..., modularFloats)
    float    modularFloats[200]; // 4 bytes each, 800 bytes total
    int32_t  modularInts[100]; // 4-byte ints 400 bytes total, starting at byte offset 800
    int32_t  modularBools[100]; // 4-byte ints 400 bytes total, starting at byte offset 1200
};

// base byte‑offset of the first “modular…” field in the buffer
//constexpr uint32_t kCustomBase = offsetof(GFXBoosterAccessData, GFXBoosterAccessData::modularFloats);

// Shader settings value
struct ShaderValue {
    bool global = false; // If true, this value is global and not per-shader
    std::string shaderDefinitionId = ""; // The ID of the shader definition this value belongs to, empty for global values
    std::string id = ""; // Unique ID for this value
    std::string label = ""; // Label to show in UI
    enum class Type { Float, Int, Bool } type = Type::Float; // Type of the value for UI and storage
    // Value Tracking
    struct Data {
        float f;
        int i;
        bool b;
    } current, min, max, step, def;
    uint32_t bufferIndex = 0; // The slot index (0-199 for floats, etc.)
    // Helper to get the byte offset within the SRV buffer based on type and index
    //uint32_t GetByteOffset() const {
    //    switch (type) {
    //    case Type::Float: return kCustomBase + bufferIndex * 4;
    //    case Type::Int:   return kCustomBase + 800 + bufferIndex * 4;
    //    case Type::Bool:  return kCustomBase + 1200 + bufferIndex * 4;
    //    default:          return 0;
    //    }
    //}
};

// Global shader settings holder
class GlobalShaderSettings {
    std::vector<ShaderValue*> globalShaderValues;
    std::vector<ShaderValue*> localShaderValues;
    std::vector<ShaderValue*> boolShaderValues;
    std::vector<ShaderValue*> floatShaderValues;
    std::vector<ShaderValue*> intShaderValues;
public:
    // Add a new shader value to the appropriate vector based on its type
    void AddShaderValue(ShaderValue* value) {
        if (!value) return;
        // Check if we have this id already
        auto dedupCheck = [&](const std::vector<ShaderValue*>& vec) {
            return std::any_of(vec.begin(), vec.end(), [&](ShaderValue* s) {
                return s && s->id == value->id;
        });
        };
        if (value->global) {
            if (!dedupCheck(globalShaderValues)) {
                globalShaderValues.push_back(value);
            }
        } else {
            if (!dedupCheck(localShaderValues)) {
                localShaderValues.push_back(value);
            }
        }
        if (value->type == ShaderValue::Type::Bool) {
            if (!dedupCheck(boolShaderValues)) {
                value->bufferIndex = static_cast<uint32_t>(boolShaderValues.size());
                boolShaderValues.push_back(value);
            }
        } else if (value->type == ShaderValue::Type::Float) {
            if (!dedupCheck(floatShaderValues)) {
                value->bufferIndex = static_cast<uint32_t>(floatShaderValues.size());
                floatShaderValues.push_back(value);
            }
        } else if (value->type == ShaderValue::Type::Int) {
            if (!dedupCheck(intShaderValues)) {
                value->bufferIndex = static_cast<uint32_t>(intShaderValues.size());
                intShaderValues.push_back(value);
            }
        }
    }
    // Save the current shader settings values to a file (e.g. JSON or INI)
    void SaveSettings() {
        std::filesystem::path settingsPath = g_pluginPath / "GFXBoosterCLShaderSettings.ini";
        std::ofstream file(settingsPath, std::ios::out);
        // Save global shader values
        file << "; Global Shader Values\n";
        for (const auto* sValue : globalShaderValues) {
            if (!sValue) continue;
            if (sValue) {
                file << "[global]\n";
                file << "id=" << sValue->id << "\n";
                file << "label=" << sValue->label << "\n";
                file << "type=";
                switch (sValue->type) {
                    case ShaderValue::Type::Float: file << "float"; break;
                    case ShaderValue::Type::Int: file << "int"; break;
                    case ShaderValue::Type::Bool: file << "bool"; break;
                };
                file << "\n";
                file << "value=";
                switch (sValue->type) {
                    case ShaderValue::Type::Float: file << sValue->current.f; break;
                    case ShaderValue::Type::Int: file << sValue->current.i; break;
                    case ShaderValue::Type::Bool: file << (sValue->current.b ? "true" : "false"); break;
                };
                file << "\n";
                file << "[/global]\n\n";
            }
        }
        // Save local shader values
        file << "; Local Shader Values\n";
        for (const auto* sValue : localShaderValues) {
            if (!sValue) continue;
            if (sValue) {
                file << "[local]\n";
                file << "shaderDefinitionId=" << sValue->shaderDefinitionId << "\n";
                file << "id=" << sValue->id << "\n";
                file << "label=" << sValue->label << "\n";
                file << "type=";
                switch (sValue->type) {
                    case ShaderValue::Type::Float: file << "float"; break;
                    case ShaderValue::Type::Int: file << "int"; break;
                    case ShaderValue::Type::Bool: file << "bool"; break;
                };
                file << "\n";
                file << "value=";
                switch (sValue->type) {
                    case ShaderValue::Type::Float: file << sValue->current.f; break;
                    case ShaderValue::Type::Int: file << sValue->current.i; break;
                    case ShaderValue::Type::Bool: file << (sValue->current.b ? "true" : "false"); break;
                };
                file << "\n";
                file << "[/local]\n\n";
            }
        }
        file.close();
    }
    // Load shader settings values from a file and update the current values
    void LoadSettings() {
        std::filesystem::path settingsPath = g_pluginPath / "GFXBoosterCLShaderSettings.ini";
        std::ifstream file(settingsPath, std::ios::in);
        // Read the entire file into a string for easier parsing
        std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        // Helper function to parse the file for global and local shader values
        auto parseValue = [&](ShaderValue* sValue) {
            if (!sValue) return;
            // look for a line "id=sValue->id"
            std::string key = "id=" + sValue->id;
            size_t pos = text.find(key);
            if (pos == std::string::npos) return; // not present
            // advance to end of line
            pos = text.find('\n', pos);
            if (pos == std::string::npos) return;
            // now search for "value=" after that line
            size_t valpos = text.find("value=", pos);
            if (valpos == std::string::npos) return;
            valpos += 6; // skip "value="
            // read until end of line
            size_t end = text.find('\n', valpos);
            std::string val = text.substr(valpos, end - valpos);
            // convert based on type
            try {
                switch (sValue->type) {
                case ShaderValue::Type::Bool:
                    sValue->current.b = (ToLower(val) == "true" || val == "1");
                    break;
                case ShaderValue::Type::Float:
                    sValue->current.f = std::stof(val);
                    break;
                case ShaderValue::Type::Int:
                    sValue->current.i = std::stoi(val);
                    break;
                }
            } catch (...) { /* ignore bad lines */ }
        };
        // Parse global shader values
        for (auto* sValue : globalShaderValues) {
            parseValue(sValue);
        }
        // Parse local shader values
        for (auto* sValue : localShaderValues) {
            parseValue(sValue);
        }
    }
    std::vector<ShaderValue*>& GetGlobalShaderValues() {
        return globalShaderValues;
    }
    std::vector<ShaderValue*>& GetLocalShaderValues() {
        return localShaderValues;
    }
    std::vector<ShaderValue*>& GetFloatShaderValues() {
        return floatShaderValues;
    }
    std::vector<ShaderValue*>& GetIntShaderValues() {
        return intShaderValues;
    }
    std::vector<ShaderValue*>& GetBoolShaderValues() {
        return boolShaderValues;
    }
};

// ENUM for shader type
enum class ShaderType {
    Vertex,
    Pixel
};

// Shader size requirement operator for matching definitions
enum class SizeOp { Equal, Greater, Less };
struct SizeRequirement {
    SizeOp op;
    std::size_t value;
};
struct InputCountRequirement {
    SizeOp op;
    int value;
};
struct OutputCountRequirement {
    SizeOp op;
    int value;
};

// Shader definitions from INI file
struct ShaderDefinition {
    std::string id;
    bool active = false;
    int priority = 0;
    // Matching criteria
    ShaderType type = ShaderType::Pixel;
    std::vector<std::string> shaderUID = {};
    std::vector<std::uint32_t> hash = {};
    std::vector<std::uint32_t> asmHash = {};
    std::vector<SizeRequirement> sizeRequirements;
    std::vector<std::pair<int, int>> bufferSizes;
    std::vector<int> textureSlots;
    std::vector<std::pair<int, int>> textureDimensions;
    std::uint32_t textureSlotMask = 0;
    std::uint32_t textureDimensionMask = 0;
    std::vector<InputCountRequirement> inputTextureCountRequirements = {};
    std::vector<InputCountRequirement> inputCountRequirements = {};
    std::uint32_t inputMask = 0;
    std::vector<OutputCountRequirement> outputCountRequirements = {};
    std::uint32_t outputMask = 0;
    // Replacement shader info
    std::filesystem::path shaderFile = "";
    // Compile state
    bool buggy = false;
    ID3DBlob* compiledShader = nullptr;
    REX::W32::ID3D11PixelShader* loadedPixelShader = nullptr;
    REX::W32::ID3D11VertexShader* loadedVertexShader = nullptr;
    // File watcher for this shader definition Shader.ini
    std::unique_ptr<HlslFileWatcher> hlslFileWatcher;
    // Logging and dumping options
    bool log = false;
    bool dump = false;
    // Collection of shader values defined for this shader, keyed by the byte offset in the SRV buffer
    std::vector<ShaderValue> shaderValues;
};

// Shader DB Entry primary key is the original shader pointer
struct ShaderDBEntry {
    // Constructors
    // Delete copy operations (atomics can't be copied)
    ShaderDBEntry(const ShaderDBEntry&) = delete;
    ShaderDBEntry& operator=(const ShaderDBEntry&) = delete;
    // Default constructor
    ShaderDBEntry() = default;
    // Move constructor
    ShaderDBEntry(ShaderDBEntry&& other) noexcept
        : originalShader(other.originalShader)
        , type(other.type)
        , shaderUID(other.shaderUID)
        , hash(other.hash)
        , asmHash(other.asmHash)
        , size(other.size)
        , textureSlots(std::move(other.textureSlots))
        , textureDimensions(std::move(other.textureDimensions))
        , textureSlotMask(other.textureSlotMask)
        , textureDimensionMask(other.textureDimensionMask)
        , inputTextureCount(other.inputTextureCount)
        , inputCount(other.inputCount)
        , inputMask(other.inputMask)
        , outputCount(other.outputCount)
        , outputMask(other.outputMask)
        , matchedDefinition(other.matchedDefinition)
        , bytecode(std::move(other.bytecode))
    {
        std::memcpy(expectedCBSizes, other.expectedCBSizes, sizeof(expectedCBSizes));
        valid.store(other.valid.load(std::memory_order_relaxed));
        matched.store(other.matched.load(std::memory_order_relaxed));
        dumped.store(other.dumped.load(std::memory_order_relaxed));
        recentlyUsed.store(other.recentlyUsed.load(std::memory_order_relaxed));
        replacementPixelShader.store(other.replacementPixelShader.load(std::memory_order_relaxed));
        replacementVertexShader.store(other.replacementVertexShader.load(std::memory_order_relaxed));
    }
    // Move assignment operator
    ShaderDBEntry& operator=(ShaderDBEntry&& other) noexcept {
        if (this != &other) {
            originalShader = other.originalShader;
            other.originalShader = nullptr; // Prevent dangling pointer
            type = other.type;
            shaderUID = std::move(other.shaderUID);
            hash = other.hash;
            asmHash = other.asmHash;
            size = other.size;
            textureSlots = std::move(other.textureSlots);
            textureDimensions = std::move(other.textureDimensions);
            textureSlotMask = other.textureSlotMask;
            textureDimensionMask = other.textureDimensionMask;
            inputTextureCount = other.inputTextureCount;
            inputCount = other.inputCount;
            inputMask = other.inputMask;
            outputCount = other.outputCount;
            outputMask = other.outputMask;
            matchedDefinition = other.matchedDefinition;
            bytecode = std::move(other.bytecode);
            std::memcpy(expectedCBSizes, other.expectedCBSizes, sizeof(expectedCBSizes));
            valid.store(other.valid.load(std::memory_order_relaxed));
            matched.store(other.matched.load(std::memory_order_relaxed));
            dumped.store(other.dumped.load(std::memory_order_relaxed));
            recentlyUsed.store(other.recentlyUsed.load(std::memory_order_relaxed));
            replacementPixelShader.store(other.replacementPixelShader.load(std::memory_order_relaxed));
            replacementVertexShader.store(other.replacementVertexShader.load(std::memory_order_relaxed));
        }
        return *this;
    }
    // For identification and matching
    // Will be initialized in the CreatePixelShader hook once and very early in the game
    void* originalShader = nullptr;
    // Matching criteria
    ShaderType type = ShaderType::Pixel;
    std::string shaderUID = "";
    std::uint32_t hash = 0;
    std::uint32_t asmHash = 0;
    std::size_t size = 0;
    std::uint32_t expectedCBSizes[14] = {0};
    std::vector<int> textureSlots = {};
    std::vector<std::pair<int, int>> textureDimensions = {};
    std::uint32_t textureSlotMask = 0;
    std::uint32_t textureDimensionMask = 0;
    int inputTextureCount = 0;
    int inputCount = 0;
    std::uint32_t inputMask = 0;
    int outputCount = 0;
    std::uint32_t outputMask = 0;
    // Shader states
    std::atomic<bool> valid{false}; // Whether this entry is valid (initialized)
    std::atomic<bool> matched{false}; // Whether we found a match for this shader
    std::atomic<bool> dumped{false}; // Whether we've already dumped this shader to disk for analysis
    std::atomic<bool> recentlyUsed{false}; // Whether this shader was used in the most recent frame
    // The matching definition from the INI file
    // We need this to compile the shader when it is needed during rendering
    ShaderDefinition* matchedDefinition = nullptr;
    // Compiled replacement shader
    std::atomic<REX::W32::ID3D11PixelShader*> replacementPixelShader{nullptr};
    std::atomic<REX::W32::ID3D11VertexShader*> replacementVertexShader{nullptr};
    // Info
    std::vector<uint8_t> bytecode = {}; // Raw bytecode for hashing and analysis
    // Helper functions for thread-safe access
    void SetValid(bool value) { valid.store(value, std::memory_order_release); }
    bool IsValid() const { return valid.load(std::memory_order_acquire); }
    void SetMatched(bool value) { matched.store(value, std::memory_order_release); }
    bool IsMatched() const { return matched.load(std::memory_order_acquire); }
    void SetDumped(bool value) { dumped.store(value, std::memory_order_release); }
    bool IsDumped() const { return dumped.load(std::memory_order_acquire); }
    ShaderDefinition* GetMatchedDefinition() const { return matchedDefinition; }
    void SetRecentlyUsed(bool value) { recentlyUsed.store(value, std::memory_order_release); }
    bool IsRecentlyUsed() const { return recentlyUsed.load(std::memory_order_acquire); }
    void SetReplacementPixelShader(REX::W32::ID3D11PixelShader* shader) {
        replacementPixelShader.store(shader, std::memory_order_release);
    }
    REX::W32::ID3D11PixelShader* GetReplacementPixelShader() const {
        return replacementPixelShader.load(std::memory_order_acquire);
    }
    void SetReplacementVertexShader(REX::W32::ID3D11VertexShader* shader) {
        replacementVertexShader.store(shader, std::memory_order_release);
    }
    REX::W32::ID3D11VertexShader* GetReplacementVertexShader() const {
        return replacementVertexShader.load(std::memory_order_acquire);
    }
};

// Database for storing shader definitions loaded from the INI file
struct ShaderDefDB {
    std::vector<ShaderDefinition*> definitions;
    mutable std::shared_mutex mutex;
    // Add a new shader definition to the database
    void AddDefinition(ShaderDefinition* def) {
        std::unique_lock lock(mutex);
        definitions.push_back(def);
    }
    // Sort definitions by priority (lower number = higher priority)
    void SortByPriority() {
        std::unique_lock lock(mutex);
        std::sort(definitions.begin(), definitions.end(), [](const ShaderDefinition* a, const ShaderDefinition* b) {
            return a->priority < b->priority;
        });
    }
    // Clear the shader definition database
    void Clear() {
        std::unique_lock lock(mutex);
        for (auto def : definitions) {
            delete def;
        }
        definitions.clear();
    }
};

// Shader database for storing parsed shader info for all shaders we encounter
struct ShaderDB {
    // Key = original shader pointer
    std::unordered_map<void*, ShaderDBEntry> entries;
    mutable std::shared_mutex mutex;
    // Functions for managing the shader database
    void AddShaderEntry(ShaderDBEntry&& entry) {
        if (!entry.IsValid()) return; // Invalid entry, skip
        std::unique_lock lock(mutex); // Exclusive for writes
        entries[entry.originalShader] = std::move(entry);
    }
    bool HasEntry(REX::W32::ID3D11PixelShader* shader) {
        std::shared_lock lock(mutex);
        return entries.find(shader) != entries.end();
    }
    bool HasEntry(REX::W32::ID3D11VertexShader* shader) {
        std::shared_lock lock(mutex);
        return entries.find(shader) != entries.end();
    }
    bool IsEntryMatched(REX::W32::ID3D11PixelShader* shader) {
        std::shared_lock lock(mutex);
        auto it = entries.find(shader);
        return (it != entries.end()) ? it->second.matched.load() : false;
    }
    bool IsEntryMatched(REX::W32::ID3D11VertexShader* shader) {
        std::shared_lock lock(mutex);
        auto it = entries.find(shader);
        return (it != entries.end()) ? it->second.matched.load() : false;
    }
    void SetEntryMatched(REX::W32::ID3D11PixelShader* shader, bool matched) {
        std::shared_lock lock(mutex);
        auto it = entries.find(shader);
        if (it != entries.end()) {
            it->second.SetMatched(matched);
        }
    }
    void SetEntryMatched(REX::W32::ID3D11VertexShader* shader, bool matched) {
        std::shared_lock lock(mutex);
        auto it = entries.find(shader);
        if (it != entries.end()) {
            it->second.SetMatched(matched);
        }
    }
    bool IsEntryDumped(REX::W32::ID3D11PixelShader* shader) {
        std::shared_lock lock(mutex);
        auto it = entries.find(shader);
        return (it != entries.end()) ? it->second.dumped.load() : false;
    }
    bool IsEntryDumped(REX::W32::ID3D11VertexShader* shader) {
        std::shared_lock lock(mutex);
        auto it = entries.find(shader);
        return (it != entries.end()) ? it->second.dumped.load() : false;
    }
    void SetEntryDumped(REX::W32::ID3D11PixelShader* shader, bool dumped) {
        std::shared_lock lock(mutex);
        auto it = entries.find(shader);
        if (it != entries.end()) {
            it->second.SetDumped(dumped);
        }
    }
    void SetEntryDumped(REX::W32::ID3D11VertexShader* shader, bool dumped) {
        std::shared_lock lock(mutex);
        auto it = entries.find(shader);
        if (it != entries.end()) {
            it->second.SetDumped(dumped);
        }
    }
    bool IsEntryRecentlyUsed(REX::W32::ID3D11PixelShader* shader) {
        std::shared_lock lock(mutex);
        auto it = entries.find(shader);
        return (it != entries.end()) ? it->second.IsRecentlyUsed() : false;
    }
    bool IsEntryRecentlyUsed(REX::W32::ID3D11VertexShader* shader) {
        std::shared_lock lock(mutex);
        auto it = entries.find(shader);
        return (it != entries.end()) ? it->second.IsRecentlyUsed() : false;
    }
    void SetEntryRecentlyUsed(REX::W32::ID3D11PixelShader* shader, bool recentlyUsed) {
        std::shared_lock lock(mutex);
        auto it = entries.find(shader);
        if (it != entries.end()) {
            it->second.SetRecentlyUsed(recentlyUsed);
        }
    }
    void SetEntryRecentlyUsed(REX::W32::ID3D11VertexShader* shader, bool recentlyUsed) {
        std::shared_lock lock(mutex);
        auto it = entries.find(shader);
        if (it != entries.end()) {
            it->second.SetRecentlyUsed(recentlyUsed);
        }
    }
    ShaderDefinition* GetMatchedDefinition(REX::W32::ID3D11PixelShader* shader) {
        std::shared_lock lock(mutex);
        auto it = entries.find(shader);
        return (it != entries.end()) ? it->second.GetMatchedDefinition() : nullptr;
    }
    ShaderDefinition* GetMatchedDefinition(REX::W32::ID3D11VertexShader* shader) {
        std::shared_lock lock(mutex);
        auto it = entries.find(shader);
        return (it != entries.end()) ? it->second.GetMatchedDefinition() : nullptr;
    }
    REX::W32::ID3D11PixelShader* GetReplacementShader(REX::W32::ID3D11PixelShader* shader) {
        std::shared_lock lock(mutex);
        auto it = entries.find(shader);
        return (it != entries.end()) ? it->second.GetReplacementPixelShader() : nullptr;
    }
    REX::W32::ID3D11VertexShader* GetReplacementShader(REX::W32::ID3D11VertexShader* shader) {
        std::shared_lock lock(mutex);
        auto it = entries.find(shader);
        return (it != entries.end()) ? it->second.GetReplacementVertexShader() : nullptr;
    }
    void SetReplacementShader(REX::W32::ID3D11PixelShader* shader, REX::W32::ID3D11PixelShader* replacement) {
        std::shared_lock lock(mutex);
        auto it = entries.find(shader);
        if (it != entries.end()) {
            it->second.SetReplacementPixelShader(replacement);
        }
    }
    void SetReplacementShader(REX::W32::ID3D11VertexShader* shader, REX::W32::ID3D11VertexShader* replacement) {
        std::shared_lock lock(mutex);
        auto it = entries.find(shader);
        if (it != entries.end()) {
            it->second.SetReplacementVertexShader(replacement);
        }
    }
    void ClearReplacementsForDefinition(ShaderDefinition* def) {
        std::shared_lock lock(mutex);
        for (auto& [shader, entry] : entries) {
            if (entry.matchedDefinition == def) {
                entry.SetReplacementPixelShader(nullptr);
                entry.SetReplacementVertexShader(nullptr);
            }
        }
    }
    void Clear() {
        std::unique_lock lock(mutex); // Exclusive for writes
        entries.clear();
    }
    void UnmatchAll() {
        std::unique_lock lock(mutex);
        for (auto& [shader, entry] : entries) {
            entry.SetMatched(false);
            entry.matchedDefinition = nullptr;
            entry.SetReplacementPixelShader(nullptr);
            entry.SetReplacementVertexShader(nullptr);
        }
    }
};

// --- Functions ---

ShaderDBEntry AnalyzeShader_Internal(REX::W32::ID3D11PixelShader* pixelShader, REX::W32::ID3D11VertexShader* vertexShader, std::vector<uint8_t> bytecode, SIZE_T BytecodeLength);
bool CompileShader_Internal(ShaderDefinition* def);
bool DoesEntryMatchDefinition_Internal(ShaderDBEntry const& entry, ShaderDefinition* def);
void DumpOriginalShader_Internal(ShaderDBEntry const& entry, ShaderDefinition* def);
bool InstallGFXHooks_Internal();
bool InstallShaderCreationHooks_Internal();
bool ReflectShader_Internal(ShaderDBEntry& entry);
void ReloadAllShaderDefinitions_Internal();
void RematchAllShaders_Internal();
void ShaderDumpWorker();
void ShutdownShaderDumping_Internal();
void UIDrawShaderSettingsOverlay();
void UIDrawShaderDebugOverlay();
void UILockShaderList_Internal();
void UIUnlockShaderList_Internal();
void UpdateCustomBuffer_Internal();
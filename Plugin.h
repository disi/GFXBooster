#pragma once
#include <Global.h>

// Forward declaration of HlslFileWatcher for use in ShaderDefinition
class HlslFileWatcher; 

// --- Hooks ---

// -- Structs ---

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
    std::vector<std::uint32_t> hash = {};
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
        , hash(other.hash)
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
            hash = other.hash;
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
    std::uint32_t hash = 0;
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
    float _padding[3]; // Pad to 16 bytes
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
void UIDrawShaderDebugOverlay();
void UILockShaderList_Internal();
void UIUnlockShaderList_Internal();
void UpdateCustomBuffer_Internal();
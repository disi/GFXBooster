#include <Global.h>

// Global logger pointer
std::shared_ptr<spdlog::logger> gLog;

// --- Explicit F4SE_API Definition ---
// This macro is essential for exporting functions from the DLL.
// If the F4SE headers aren't providing it correctly for your setup,
// we define it directly.
#define F4SE_API __declspec(dllexport)

// This is used by commonLibF4
namespace Version
{
    inline constexpr std::size_t MAJOR = 0;
    inline constexpr std::size_t MINOR = 1;
    inline constexpr std::size_t PATCH = 5;
    inline constexpr auto NAME = "0.1.5"sv;
    inline constexpr auto AUTHORNAME = "disi"sv;
    inline constexpr auto PROJECT = "GFXBoosterCL"sv;
} // namespace Version


// Declare the F4SEMessagingInterface and F4SEScaleformInterface
const F4SE::MessagingInterface *g_messaging = nullptr;
// Papyrus interface
const F4SE::PapyrusInterface *g_papyrus = nullptr;
// Task interface for menus and threads
const F4SE::TaskInterface *g_taskInterface = nullptr;
// Scaleform interface
const F4SE::ScaleformInterface *g_scaleformInterface = nullptr;
// Plugin handle
F4SE::PluginHandle g_pluginHandle = 0;
// Datahandler
RE::TESDataHandler *g_dataHandle = 0;

// Variables
// Global Module Name
std::string g_moduleName = "GFXBoosterCL.dll";
// Global INI Name
std::string g_iniName = "GFXBoosterCL.ini";
// Global Log Name
std::string g_logName = "GFXBoosterCL.log";
// Global plugin path
std::filesystem::path g_pluginPath;
// Shader folder path for loading custom shaders and watching for changes in development mode
std::filesystem::path g_shaderFolderPath;
// Global debug flag
bool DEBUGGING = false;
// Custom buffer update flag
bool CUSTOMBUFFER_ON = true;
// Custom resource view slot in shader
UINT CUSTOMBUFFER_SLOT = 14;
// Shader settings menu flag
bool SHADERSETTINGS_ON = false;
// Shader settings menu hotkey (default END key)
UINT SHADERSETTINGS_MENUHOTKEY = VK_END;
// Settings save hotkey (default HOME key)
UINT SHADERSETTINGS_SAVEHOTKEY = VK_HOME;
// Settings menu width
int SHADERSETTINGS_WIDTH = 600;
// Settings menu height
int SHADERSETTINGS_HEIGHT = 300;
// Settings menu opacity (0.0 - 1.0)
float SHADERSETTINGS_OPACITY = 0.75f;
// Global Development features flag (like dumping shaders, extra logging, etc)
bool DEVELOPMENT = false;
// Development GUI flag
bool DEVGUI_ON = false;
// Development GUI Width
int DEVGUI_WIDTH = 600;
// Development GUI Height
int DEVGUI_HEIGHT = 300;
// Development GUI opacity (0.0 - 1.0)
float DEVGUI_OPACITY = 0.75f;
// Global ImGui state flag
bool g_imguiInitialized = false;
// Global flag if GFX hooks are installed
bool GFX_HOOKS_INSTALLED = false;
// Shader definitions from INI
ShaderDefDB g_shaderDefinitions = {};
// Shader values from INI
GlobalShaderSettings g_shaderSettings = {};
// Shader include path for D3DCompile calls
std::filesystem::path g_commonShaderHeaderPath;
// Global INI watcher map for hot-reloading shader definitions when their files change
std::unordered_map<std::filesystem::path, std::unique_ptr<ShaderIniFileWatcher>> g_iniWatchers;
static std::atomic<bool> g_reloadQueued{false};

// Helper to check if a file exists
bool FileExists(const std::filesystem::path& filepath) {
    return std::filesystem::exists(filepath) && std::filesystem::is_regular_file(filepath);
}
// Helper to remove all whitespace from a string
inline std::string RemoveAllWhitespace(std::string line) {
    line.erase(std::remove_if(line.begin(), line.end(),
        [](unsigned char c){ return std::isspace(c); }), line.end());
    return line;
}
// Helper to remove inline comments from a line (anything after ';')
std::string RemoveInlineComment(const std::string& line) {
    // Find semicolon
    size_t pos = line.find(';');
    // Get substring before semicolon (or entire line if no semicolon)
    std::string result = (pos != std::string::npos) ? line.substr(0, pos) : line;
    // Trim trailing whitespace
    size_t end = result.find_last_not_of(" \t\r\n");
    if (end != std::string::npos) {
        result = result.substr(0, end + 1);
    } else {
        result.clear(); // Line was all whitespace
    }
    return result;
}
// Helper function to extract key-value pair from a line
inline std::pair<std::string, std::string> GetKeyValueFromLine(const std::string& line) {
    // Remove inline comments first
    std::string cleanLine = RemoveInlineComment(line);
    if (cleanLine.empty()) {
        return {"", ""};
    }
    // Remove all whitespace for easier parsing
    cleanLine = RemoveAllWhitespace(cleanLine);
    // Find the '=' character
    size_t eqPos = cleanLine.find('=');
    if (eqPos == std::string::npos) {
        return {"", ""};
    }
    // Extract key and value
    std::string key = cleanLine.substr(0, eqPos);
    std::string value = cleanLine.substr(eqPos + 1);
    return {key, value};
}
// Helper to get the directory of the plugin DLL OS agnostic
std::filesystem::path GetPluginDirectory(HMODULE hModule) {
#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(hModule, path, MAX_PATH);
    std::filesystem::path fullPath(path);
    return fullPath.parent_path();
#else
    // Linux/macOS: Use dladdr() or similar
    Dl_info dl_info;
    if (dladdr((void*)GetPluginDirectory, &dl_info)) {
        std::filesystem::path fullPath(dl_info.dli_fname);
        return fullPath.parent_path();
    }
    return std::filesystem::current_path(); // Fallback
#endif
}
// Helper to parse hex string to uint32_t
uint32_t ParseHexFormID(const std::string &hexStr)
{
    return static_cast<uint32_t>(std::stoul(hexStr, nullptr, 16));
}
// Helper to enumerate directories
std::vector<std::filesystem::path> GetSubdirectories(const std::filesystem::path& path) {
    std::vector<std::filesystem::path> directories;
    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
        return directories;
    }
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_directory()) {
            directories.push_back(entry.path());
        }
    }
    return directories;
}
// Helper to load all shader definitions from a Shader.ini file
// Returns number of definitions loaded
int LoadShaderDefinitionsFromFile(const std::filesystem::path& shaderFolderPath, const std::string& folderName) {
    std::filesystem::path iniShaderPath = shaderFolderPath / "Shader.ini";
    if (!FileExists(iniShaderPath)) {
        REX::WARN("LoadShaderDefinitionsFromFile: Shader.ini not found in: {}", shaderFolderPath.string());
        return 0;
    }
    std::ifstream file(iniShaderPath, std::ios::in);
    if (!file.is_open()) {
        REX::WARN("LoadShaderDefinitionsFromFile: Could not open Shader.ini: {}", iniShaderPath.string());
        return 0;
    }
    int loadedCount = 0;
    std::string cachedShaderID;
    bool hasCachedShaderID = false;
    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == ';') continue;
        // Remove inline comments and trim
        std::string clean = RemoveInlineComment(line);
        // Remove all whitespace for easier parsing
        clean = RemoveAllWhitespace(clean);
        if (clean.empty()) continue;
        // Check if this is a section start (e.g., [loading_screen])
        if (clean[0] == '[' && clean.back() == ']' && clean.find('/') != 0) {
            // Extract shader ID from section name
            std::string shaderId = clean.substr(1, clean.length() - 2);
            if (shaderId.empty()) {
                REX::WARN("LoadShaderDefinitionsFromFile: Empty section name in {}", iniShaderPath.string());
                continue;
            }
            // Create new shader definition
            ShaderDefinition def;
            def.id = shaderId;
            // Read all properties until we hit the closing tag [/shader_id]
            std::string endTag = "[/" + shaderId + "]";
            while (std::getline(file, line)) {
                // Skip empty lines and comments
                if (line.empty() || line[0] == ';') continue;
                // Remove inline comments and trim
                std::string clean = RemoveInlineComment(line);
                // Remove all whitespace for easier parsing
                clean = RemoveAllWhitespace(clean);
                // Check for end tag
                if (ToLower(clean) == ToLower(endTag)) {
                    break;
                }
                // Check if we hit another section start (malformed INI)
                if (clean[0] == '[') {
                    REX::WARN("LoadShaderDefinitionsFromFile: Found new section before closing tag {} in {}", endTag, iniShaderPath.string());
                    break;
                }
                // Get key-value pair
                auto[key, value] = GetKeyValueFromLine(clean);
                // If key or value is empty, skip and use default values for this line
                if (key.empty() || value.empty()) continue;
                // create lower key for easier comparison
                auto lowerKey = ToLower(key);
                // Default to false
                if (lowerKey == "active") {
                    if (ToLower(value) == "true" || value == "1") {
                        def.active = true;
                    }
                }
                // Default to 0
                else if (lowerKey == "priority") {
                    try {
                        def.priority = std::stoi(value);
                    } catch (...) {
                        REX::WARN("LoadShaderDefinitionsFromFile: Invalid priority for {}: {}", shaderId, value);
                        def.priority = 0;
                    }
                }
                // Default to ps
                else if (lowerKey == "type") {
                    std::string type = ToLower(value);
                    if (type == "vs" || type == "vertex") {
                        def.type = ShaderType::Vertex;
                    }
                }
                // Default empty vector
                else if (lowerKey == "shaderuid") {
                    std::stringstream ss(value);
                    std::string segment;
                    while (std::getline(ss, segment, ',')) {
                        try {
                            def.shaderUID.push_back(segment);
                        } catch (...) {
                            REX::WARN("LoadShaderDefinitionsFromFile: Invalid shaderUID for {}: {}", shaderId, segment);
                        }
                    }
                }
                // Default to 0
                else if (lowerKey == "hash") {
                    std::stringstream ss(value);
                    std::string segment;
                    while (std::getline(ss, segment, ',')) {
                        try {
                            def.hash.push_back(ParseHexFormID(segment));
                        } catch (...) {
                            REX::WARN("LoadShaderDefinitionsFromFile: Invalid hash for {}: {}", shaderId, segment);
                        }
                    }
                }
                // Default to 0
                else if (lowerKey == "asmhash") {
                    std::stringstream ss(value);
                    std::string segment;
                    while (std::getline(ss, segment, ',')) {
                        try {
                            def.asmHash.push_back(ParseHexFormID(segment));
                        } catch (...) {
                            REX::WARN("LoadShaderDefinitionsFromFile: Invalid asmHash for {}: {}", shaderId, segment);
                        }
                    }
                }
                else if (lowerKey == "size") {
                    std::stringstream ss(value);
                    std::string segment;
                    while (std::getline(ss, segment, ',')) {
                        // Remove spaces and parentheses: ( >1024 ) -> >1024
                        segment.erase(std::remove(segment.begin(), segment.end(), ' '), segment.end());
                        segment.erase(std::remove(segment.begin(), segment.end(), '('), segment.end());
                        segment.erase(std::remove(segment.begin(), segment.end(), ')'), segment.end());
                        SizeRequirement req;
                        if (segment.find('>') != std::string::npos) {
                            req.op = SizeOp::Greater;
                            req.value = std::stoull(segment.substr(segment.find('>') + 1));
                        } else if (segment.find('<') != std::string::npos) {
                            req.op = SizeOp::Less;
                            req.value = std::stoull(segment.substr(segment.find('<') + 1));
                        } else {
                            req.op = SizeOp::Equal;
                            req.value = std::stoull(segment);
                        }
                        def.sizeRequirements.push_back(req);
                    }
                }
                else if (lowerKey == "buffersize") {
                    std::istringstream ss(value);
                    std::string token;
                    while (std::getline(ss, token, ',')) {
                        size_t atPos = token.find('@');
                        if (atPos != std::string::npos) {
                            try {
                                int size = std::stoi(token.substr(0, atPos));
                                std::string slotStr = token.substr(atPos + 1);
                                // Trim whitespace from slot string
                                slotStr.erase(0, slotStr.find_first_not_of(" \t"));
                                slotStr.erase(slotStr.find_last_not_of(" \t") + 1);
                                // If slot is empty, use -1 for "any slot"
                                int slot = slotStr.empty() ? -1 : std::stoi(slotStr);
                                def.bufferSizes.emplace_back(size, slot);
                            } catch (...) {
                                REX::WARN("LoadConfig: Invalid or empty buffer size in INI: {}", value);
                                def.bufferSizes.clear();
                            }
                        }
                    }
                }
                else if (lowerKey == "textures") {
                    std::istringstream ss(value);
                    std::string token;
                    while (std::getline(ss, token, ',')) {
                        def.textureSlots.push_back(std::stoi(token));
                    }
                    // Update textureSlotMask bitmask based on specified texture slots
                    if (def.textureSlots.size() > 0) {
                        for (const auto& tex : def.textureSlots) {
                            int slot = tex;
                            if (slot >= 0 && slot < 32) {
                                def.textureSlotMask |= (std::uint32_t(1) << slot);
                            }
                        }
                    } else {
                        def.textureSlotMask = 0;
                    }
                }
                else if (lowerKey == "texturedimensions") {
                    std::istringstream ss(value);
                    std::string token;
                    while (std::getline(ss, token, ',')) {
                        size_t atPos = token.find('@');
                        if (atPos != std::string::npos) {
                            try {
                                int slot = std::stoi(token.substr(0, atPos));
                                std::string dimStr = token.substr(atPos + 1);
                                // Trim whitespace from dimension string
                                dimStr.erase(0, dimStr.find_first_not_of(" \t"));
                                dimStr.erase(dimStr.find_last_not_of(" \t") + 1);
                                int dimension = dimStr.empty() ? -1 : std::stoi(dimStr);
                                def.textureDimensions.emplace_back(dimension, slot);
                                if (dimension >= 0 && dimension < 32) {
                                    def.textureDimensionMask |= (1UL << dimension);
                                }
                            } catch (...) {
                                REX::WARN("LoadConfig: Invalid or empty texture dimension in INI: {}", token);
                                def.textureDimensions.clear();
                            }
                        }
                    }
                    // Update textureDimensionMask bitmask based on specified texture dimensions
                    if (def.textureDimensions.size() > 0) {
                        for (const auto& tex : def.textureDimensions) {
                            int dimension = tex.first;
                            if (dimension >= 0 && dimension < 32) {
                                def.textureDimensionMask |= (std::uint32_t(1) << dimension);
                            }
                        }
                    } else {
                        def.textureDimensionMask = 0;
                    }
                }
                // Default to 0
                else if (lowerKey == "textureslotmask") {
                    try {
                        def.textureSlotMask = std::stoul(value, nullptr, 16);
                    } catch (...) {
                        REX::WARN("LoadShaderDefinitionsFromFile: Invalid textureSlotMask for {}: {}", shaderId, value);
                        def.textureSlotMask = 0;
                    }
                }
                // Default to 0
                else if (lowerKey == "texturedimensionmask") {
                    try {
                        def.textureDimensionMask = std::stoul(value, nullptr, 16);
                    } catch (...) {
                        REX::WARN("LoadShaderDefinitionsFromFile: Invalid textureDimensionMask for {}: {}", shaderId, value);
                        def.textureDimensionMask = 0;
                    }
                }
                else if (lowerKey == "inputTextureCount") {
                    std::stringstream ss(value);
                    std::string segment;
                    while (std::getline(ss, segment, ',')) {
                        // Remove spaces and parentheses: ( >2 ) -> >2
                        segment.erase(std::remove(segment.begin(), segment.end(), ' '), segment.end());
                        segment.erase(std::remove(segment.begin(), segment.end(), '('), segment.end());
                        segment.erase(std::remove(segment.begin(), segment.end(), ')'), segment.end());
                        InputCountRequirement req;
                        if (segment.find('>') != std::string::npos) {
                            req.op = SizeOp::Greater;
                            req.value = std::stoi(segment.substr(segment.find('>') + 1));
                        } else if (segment.find('<') != std::string::npos) {
                            req.op = SizeOp::Less;
                            req.value = std::stoi(segment.substr(segment.find('<') + 1));
                        } else {
                            req.op = SizeOp::Equal;
                            req.value = std::stoi(segment);
                        }
                        def.inputTextureCountRequirements.push_back(req);
                    }
                }
                else if (lowerKey == "inputcount") {
                    std::stringstream ss(value);
                    std::string segment;
                    while (std::getline(ss, segment, ',')) {
                        // Remove spaces and parentheses: ( >2 ) -> >2
                        segment.erase(std::remove(segment.begin(), segment.end(), ' '), segment.end());
                        segment.erase(std::remove(segment.begin(), segment.end(), '('), segment.end());
                        segment.erase(std::remove(segment.begin(), segment.end(), ')'), segment.end());
                        InputCountRequirement req;
                        if (segment.find('>') != std::string::npos) {
                            req.op = SizeOp::Greater;
                            req.value = std::stoi(segment.substr(segment.find('>') + 1));
                        } else if (segment.find('<') != std::string::npos) {
                            req.op = SizeOp::Less;
                            req.value = std::stoi(segment.substr(segment.find('<') + 1));
                        } else {
                            req.op = SizeOp::Equal;
                            req.value = std::stoi(segment);
                        }
                        def.inputCountRequirements.push_back(req);
                    }
                }
                else if (lowerKey == "inputmask") {
                    try {
                        def.inputMask = std::stoul(value, nullptr, 16);
                    } catch (...) {
                        REX::WARN("LoadShaderDefinitionsFromFile: Invalid inputMask for {}: {}", shaderId, value);
                        def.inputMask = 0;
                    }
                }
                else if (lowerKey == "outputcount") {
                    std::stringstream ss(value);
                    std::string segment;
                    while (std::getline(ss, segment, ',')) {
                        // Remove spaces and parentheses: ( >2 ) -> >2
                        segment.erase(std::remove(segment.begin(), segment.end(), ' '), segment.end());
                        segment.erase(std::remove(segment.begin(), segment.end(), '('), segment.end());
                        segment.erase(std::remove(segment.begin(), segment.end(), ')'), segment.end());
                        OutputCountRequirement req;
                        if (segment.find('>') != std::string::npos) {
                            req.op = SizeOp::Greater;
                            req.value = std::stoi(segment.substr(segment.find('>') + 1));
                        } else if (segment.find('<') != std::string::npos) {
                            req.op = SizeOp::Less;
                            req.value = std::stoi(segment.substr(segment.find('<') + 1));
                        } else {
                            req.op = SizeOp::Equal;
                            req.value = std::stoi(segment);
                        }
                        def.outputCountRequirements.push_back(req);
                    }
                }
                else if (lowerKey == "outputmask") {
                    try {
                        def.outputMask = std::stoul(value, nullptr, 16);
                    } catch (...) {
                        REX::WARN("LoadShaderDefinitionsFromFile: Invalid outputMask for {}: {}", shaderId, value);
                        def.outputMask = 0;
                    }
                }
                // Default to empty string
                else if (lowerKey == "shader") {
                    // Resolve shader path relative to shader folder
                    std::filesystem::path shaderPath = shaderFolderPath / value;
                    if (!FileExists(shaderPath)) {
                        REX::WARN("LoadShaderDefinitionsFromFile: Shader file not found for {}: {}", shaderId, shaderPath.string());
                        def.shaderFile = "";
                    } else {
                        def.shaderFile = shaderPath;
                    }
                }
                // Default to false
                else if (lowerKey == "log") {
                    if (ToLower(value) == "true" || value == "1") {
                        def.log = true;
                    }
                }
                // Default to false
                else if (lowerKey == "dump") {
                    if (ToLower(value) == "true" || value == "1") {
                        def.dump = true;
                    }
                }
            }
            // Cache the ShaderDefinition ID
            std::string shaderID = def.id;
            if (!hasCachedShaderID) {
                cachedShaderID = shaderID;
                hasCachedShaderID = true;
            }
            // Add the definition to global list
            g_shaderDefinitions.AddDefinition(new ShaderDefinition(std::move(def)));
            loadedCount++;
            REX::INFO("LoadShaderDefinitionsFromFile: Loaded shader '{}' from {}/Shader.ini", shaderID, folderName);
        }
    }
    file.close();
    // Load Values.ini once per folder and assign values to the first loaded definition ID
    std::filesystem::path iniValuesPath = shaderFolderPath / "Values.ini";
    if (!FileExists(iniValuesPath)) {
        REX::WARN("LoadShaderDefinitionsFromFile: Values.ini not found in: {}", shaderFolderPath.string());
    }
    std::ifstream valuesFile(iniValuesPath, std::ios::in);
    if (!valuesFile.is_open()) {
        REX::WARN("LoadShaderDefinitionsFromFile: Could not open Values.ini: {}", iniValuesPath.string());
    } else if (!hasCachedShaderID) {
        REX::WARN("LoadShaderDefinitionsFromFile: No shader definition loaded in {}. Skipping Values.ini", shaderFolderPath.string());
    } else {
        std::string valueLine;
        while (std::getline(valuesFile, valueLine)) {
            // Create new shader value
            ShaderValue shaderV;
            // Skip empty lines and comments
            if (valueLine.empty() || valueLine[0] == ';') continue;
            // Remove inline comments and trim
            std::string clean = RemoveInlineComment(valueLine);
            // Remove all whitespace for easier parsing
            clean = RemoveAllWhitespace(clean);
            if (clean.empty()) continue;
            // Check if this is a section start (e.g., [global] or [local])
            if (clean[0] == '[' && clean.back() == ']' && clean.find('/') != 0) {
                // Extract value scope from section name
                std::string valueScope = clean.substr(1, clean.length() - 2);
                if (valueScope.empty()) {
                    REX::WARN("LoadShaderDefinitionsFromFile: Empty value scope in {}", iniValuesPath.string());
                    continue;
                }
                shaderV.shaderDefinitionId = cachedShaderID;
                // Read all properties until we hit the closing tag [/scope]
                std::string endTag = "[/" + valueScope + "]";
                // Check if global values section
                if (ToLower(valueScope) == "global") {
                    shaderV.global = true;
                }
                while (std::getline(valuesFile, valueLine)) {
                    // Skip empty lines and comments
                    if (valueLine.empty() || valueLine[0] == ';') continue;
                    // Remove inline comments and trim
                    std::string valueClean = RemoveInlineComment(valueLine);
                    // Remove all whitespace for easier parsing
                    valueClean = RemoveAllWhitespace(valueClean);
                    if (valueClean.empty()) continue;
                    // Check for end tag
                    if (ToLower(valueClean) == ToLower(endTag)) {
                        break;
                    }
                    // Check if we hit another section start (malformed INI)
                    if (valueClean[0] == '[') {
                        REX::WARN("LoadShaderDefinitionsFromFile: Found new value before closing tag {} in {}", endTag, iniValuesPath.string());
                        break;
                    }
                    // Get key-value pair
                    auto[key, value] = GetKeyValueFromLine(valueClean);
                    // If key or value is empty, skip and use default values for this line
                    if (key.empty() || value.empty()) continue;
                    // create lower key for easier comparison
                    auto lowerKey = ToLower(key);
                    if (lowerKey == "id") {
                        shaderV.id = value;
                    }
                    else if (lowerKey == "label") {
                        shaderV.label = value;
                    }
                    else if (lowerKey == "type") {
                        std::string type = ToLower(value);
                        if (type == "bool") {
                            shaderV.type = ShaderValue::Type::Bool;
                        }
                        else if (type == "int") {
                            shaderV.type = ShaderValue::Type::Int;
                        }
                        else if (type == "float") {
                            shaderV.type = ShaderValue::Type::Float;
                        }
                    }
                    else if (lowerKey == "value") {
                        try {
                            if (shaderV.type == ShaderValue::Type::Bool) {
                                shaderV.current.b = (ToLower(value) == "true" || value == "1");
                                shaderV.def.b = (ToLower(value) == "true" || value == "1");
                            }
                            else if (shaderV.type == ShaderValue::Type::Int) {
                                shaderV.current.i = std::stoi(value);
                                shaderV.def.i = std::stoi(value);
                            }
                            else if (shaderV.type == ShaderValue::Type::Float) {
                                shaderV.current.f = std::stof(value);
                                shaderV.def.f = std::stof(value);
                            }
                        } catch (...) {
                            REX::WARN("LoadShaderDefinitionsFromFile: Invalid value for {}: {}", shaderV.id, value);
                        }
                    }
                    else if (lowerKey == "min") {
                        try {
                            if (shaderV.type == ShaderValue::Type::Int) {
                                shaderV.min.i = std::stoi(value);
                            }
                            else if (shaderV.type == ShaderValue::Type::Float) {
                                shaderV.min.f = std::stof(value);
                            }
                        } catch (...) {
                            REX::WARN("LoadShaderDefinitionsFromFile: Invalid min value for {}: {}", shaderV.id, value);
                        }
                    }
                    else if (lowerKey == "max") {
                        try {
                            if (shaderV.type == ShaderValue::Type::Int) {
                                shaderV.max.i = std::stoi(value);
                            }
                            else if (shaderV.type == ShaderValue::Type::Float) {
                                shaderV.max.f = std::stof(value);
                            }
                        } catch (...) {
                            REX::WARN("LoadShaderDefinitionsFromFile: Invalid max value for {}: {}", shaderV.id, value);
                        }
                    }
                    else if (lowerKey == "step") {
                        try {
                            if (shaderV.type == ShaderValue::Type::Int) {
                                shaderV.step.i = std::stoi(value);
                            }
                            else if (shaderV.type == ShaderValue::Type::Float) {
                                shaderV.step.f = std::stof(value);
                            }
                        } catch (...) {
                            REX::WARN("LoadShaderDefinitionsFromFile: Invalid step value for {}: {}", shaderV.id, value);
                        }
                    }
                }
                ShaderValue finalShaderV = shaderV; // Make a copy for ownership handoff
                g_shaderSettings.AddShaderValue(new ShaderValue(std::move(finalShaderV)));
            }
        }
        REX::INFO("LoadShaderDefinitionsFromFile: Loaded values for shader '{}' from {}/Values.ini", cachedShaderID, folderName);
    }
    valuesFile.close();
    // After loading all definitions from this INI file, create a watcher for it
    if (DEVELOPMENT && loadedCount > 0) {
        auto watcher = std::make_unique<ShaderIniFileWatcher>(iniShaderPath, folderName);
        watcher->Start();
        g_iniWatchers[iniShaderPath] = std::move(watcher);
    }
    return loadedCount;
}

// Load config and shader definitions from INI file
void LoadConfig(HMODULE hModule) {
    std::filesystem::path configPath = GetPluginDirectory(hModule) / g_iniName;
    g_pluginPath = configPath.parent_path();
    std::string configPathStr = configPath.string();
    REX::INFO("LoadConfig: Loading config from: {}", configPathStr);
    std::ifstream file(configPathStr, std::ios::in);
    if (!file.is_open()) {
        REX::WARN("LoadConfig: Could not open INI file: {}. Creating default.", configPathStr);
        // Create default INI file with debugging enabled if it doesn't exist
        std::ofstream out(configPathStr, std::ios::out);
        if (out.is_open()) {
            out << defaultIni;
            out.close();
        }
        file.open(configPathStr, std::ios::in);
    }
    // Parse main INI file for global settings
    std::string line;
    while (std::getline(file, line)) {
        auto[key, value] = GetKeyValueFromLine(line);
        if (key.empty()) continue;
        // create lower key for easier comparison
        auto lowerKey = ToLower(key);
        if (lowerKey == "debugging") {
            DEBUGGING = (ToLower(value) == "true" || value == "1");
            REX::INFO("LoadConfig: DEBUGGING set to {}", DEBUGGING);
            continue;
        }
        else if (lowerKey == "custombuffer_on") {
            CUSTOMBUFFER_ON = (ToLower(value) == "true" || value == "1");
            REX::INFO("LoadConfig: CUSTOMBUFFER_ON set to {}", CUSTOMBUFFER_ON);
            continue;
        }
        else if (lowerKey == "custombuffer_slot") {
            try {
                CUSTOMBUFFER_SLOT = static_cast<UINT>(std::stoi(value));
                REX::INFO("LoadConfig: CUSTOMBUFFER_SLOT set to {}", CUSTOMBUFFER_SLOT);
            } catch (...) {
                REX::WARN("LoadConfig: Invalid CUSTOMBUFFER_SLOT value: {}. Using default: {}", value, CUSTOMBUFFER_SLOT);
            }
            continue;
        }
        else if (lowerKey == "shadersettings_on") {
            SHADERSETTINGS_ON = (ToLower(value) == "true" || value == "1");
            REX::INFO("LoadConfig: SHADERSETTINGS_ON set to {}", SHADERSETTINGS_ON);
            continue;
        }
        else if (lowerKey == "shadersettings_menuhotkey") {
            try {
                SHADERSETTINGS_MENUHOTKEY = static_cast<UINT>(std::stoi(value));
                REX::INFO("LoadConfig: SHADERSETTINGS_MENUHOTKEY set to {}", SHADERSETTINGS_MENUHOTKEY);
            } catch (...) {
                REX::WARN("LoadConfig: Invalid SHADERSETTINGS_MENUHOTKEY value: {}. Using default: {}", value, SHADERSETTINGS_MENUHOTKEY);
            }
            continue;
        }
        else if (lowerKey == "shadersettings_savehotkey") {
            try {
                SHADERSETTINGS_SAVEHOTKEY = static_cast<UINT>(std::stoi(value));
                REX::INFO("LoadConfig: SHADERSETTINGS_SAVEHOTKEY set to {}", SHADERSETTINGS_SAVEHOTKEY);
            } catch (...) {
                REX::WARN("LoadConfig: Invalid SHADERSETTINGS_SAVEHOTKEY value: {}. Using default: {}", value, SHADERSETTINGS_SAVEHOTKEY);
            }
            continue;
        }
        else if (lowerKey == "shadersettings_width") {
            try {
                SHADERSETTINGS_WIDTH = std::stoi(value);
                REX::INFO("LoadConfig: SHADERSETTINGS_WIDTH set to {}", SHADERSETTINGS_WIDTH);
            } catch (...) {
                REX::WARN("LoadConfig: Invalid SHADERSETTINGS_WIDTH value: {}. Using default: {}", value, SHADERSETTINGS_WIDTH);
            }
            continue;
        }
        else if (lowerKey == "shadersettings_height") {
            try {
                SHADERSETTINGS_HEIGHT = std::stoi(value);
                REX::INFO("LoadConfig: SHADERSETTINGS_HEIGHT set to {}", SHADERSETTINGS_HEIGHT);
            } catch (...) {
                REX::WARN("LoadConfig: Invalid SHADERSETTINGS_HEIGHT value: {}. Using default: {}", value, SHADERSETTINGS_HEIGHT);
            }
            continue;
        }
        else if (lowerKey == "shadersettings_opacity") {
            try {
                SHADERSETTINGS_OPACITY = std::stof(value);
                if (SHADERSETTINGS_OPACITY < 0.0f || SHADERSETTINGS_OPACITY > 1.0f) {
                    REX::WARN("LoadConfig: SHADERSETTINGS_OPACITY value out of range (0.0 - 1.0): {}. Using default: {}", value, SHADERSETTINGS_OPACITY);
                    SHADERSETTINGS_OPACITY = 0.75f;
                } else {
                    REX::INFO("LoadConfig: SHADERSETTINGS_OPACITY set to {}", SHADERSETTINGS_OPACITY);
                }
            } catch (...) {
                REX::WARN("LoadConfig: Invalid SHADERSETTINGS_OPACITY value: {}. Using default: {}", value, SHADERSETTINGS_OPACITY);
            }
            continue;
        }
        else if (lowerKey == "development") {
            DEVELOPMENT = (ToLower(value) == "true" || value == "1");
            REX::INFO("LoadConfig: DEVELOPMENT set to {}", DEVELOPMENT);
            continue;
        }
        else if (lowerKey == "devgui_on") {
            DEVGUI_ON = (ToLower(value) == "true" || value == "1");
            REX::INFO("LoadConfig: DEVGUI_ON set to {}", DEVGUI_ON);
            continue;
        }
        else if (lowerKey == "devgui_width") {
            try {
                DEVGUI_WIDTH = std::stoi(value);
                REX::INFO("LoadConfig: DEVGUI_WIDTH set to {}", DEVGUI_WIDTH);
            } catch (...) {
                REX::WARN("LoadConfig: Invalid DEVGUI_WIDTH value: {}. Using default: {}", value, DEVGUI_WIDTH);
            }
            continue;
        }
        else if (lowerKey == "devgui_height") {
            try {
                DEVGUI_HEIGHT = std::stoi(value);
                REX::INFO("LoadConfig: DEVGUI_HEIGHT set to {}", DEVGUI_HEIGHT);
            } catch (...) {
                REX::WARN("LoadConfig: Invalid DEVGUI_HEIGHT value: {}. Using default: {}", value, DEVGUI_HEIGHT);
            }
            continue;
        }
        else if (lowerKey == "devgui_opacity") {
            try {
                DEVGUI_OPACITY = std::stof(value);
                if (DEVGUI_OPACITY < 0.0f || DEVGUI_OPACITY > 1.0f) {
                    REX::WARN("LoadConfig: DEVGUI_OPACITY value out of range (0.0 - 1.0): {}. Using default: {}", value, DEVGUI_OPACITY);
                    DEVGUI_OPACITY = 0.5f;
                } else {
                    REX::INFO("LoadConfig: DEVGUI_OPACITY set to {}", DEVGUI_OPACITY);
                }
            } catch (...) {
                REX::WARN("LoadConfig: Invalid DEVGUI_OPACITY value: {}. Using default: {}", value, DEVGUI_OPACITY);
            }
            continue;
        }
    }
    file.close();
    // Scan for shader definitions in subdirectories
    if (g_shaderFolderPath.empty()) {
        g_shaderFolderPath = configPath.parent_path() / "GFXBooster";
    }
    REX::INFO("LoadConfig: Scanning for shaders in: {}", g_shaderFolderPath.string());
    auto shaderFolders = GetSubdirectories(g_shaderFolderPath);
    REX::INFO("LoadConfig: Found {} shader folder(s)", shaderFolders.size());
    int totalDefinitions = 0;
    for (const auto& folderPath : shaderFolders) {
        std::string folderName = folderPath.filename().string();
        int count = LoadShaderDefinitionsFromFile(folderPath, folderName);
        totalDefinitions += count;
        if (count > 0) {
            REX::INFO("LoadConfig: Loaded {} definition(s) from {}", count, folderName);
        }
    }
    // Load stored shader settings values from disk
    g_shaderSettings.LoadSettings();
    REX::INFO("LoadConfig: Completed. Loaded {} shader definition(s) total", totalDefinitions);
    // Add HlslFileWatcher for shaderBasePath to automatically reload shader definitions on changes
    if (DEVELOPMENT) {
        REX::INFO("LoadConfig: Setting up development file watcher for shader definitions in: {}", g_shaderFolderPath.string());
        for (auto* def : g_shaderDefinitions.definitions) {
            if (def->active && !def->shaderFile.empty()) {
                def->hlslFileWatcher = std::make_unique<HlslFileWatcher>(def->shaderFile, def);
                def->hlslFileWatcher->Start();
            }
        }
    }
    // Set global shader include path for D3DCompile calls
    try {
    g_commonShaderHeaderPath = g_shaderFolderPath / "Include";
    } catch (...) {
        // Fallback if canonical fails
        g_commonShaderHeaderPath = configPath.parent_path() / "Plugins" / "GFXBooster" / "Include";
    }
    if (DEBUGGING) {
        REX::INFO("LoadConfig: Setting global shader include path for D3DCompile to: {}", g_commonShaderHeaderPath.string());
    }
}

// Hot-reload all shader definitions and rematch shaders in ShaderDB
void ReloadAllShaderDefinitions_Internal() {
    if (DEBUGGING) {
        REX::INFO("ReloadAllShaderDefinitions_Internal: Starting hot reload of shader definitions...");
    }
    // Remove all connections ShaderDB <> ShaderDefDB
    g_ShaderDB.UnmatchAll();
    // STOP and destroy all file watchers BEFORE processing new INI files
    for (auto* def : g_shaderDefinitions.definitions) {
        if (def->hlslFileWatcher) {
            def->hlslFileWatcher->Stop();  // Stop background thread
            def->hlslFileWatcher.reset();  // Destroy file watcher
        }
    }
    g_iniWatchers.clear();  // Stop INI file watchers
    // At this point we have no active watchers and no connections between ShaderDB and ShaderDefDB
    g_shaderDefinitions.Clear();  // Deletes old definitions
    // Build a new ShaderDefDB from the INI files on disk
    if (g_shaderFolderPath.empty()) {
        g_shaderFolderPath = g_pluginPath / "GFXBooster";
    }
    auto shaderFolders = GetSubdirectories(g_shaderFolderPath);
    for (const auto& folderPath : shaderFolders) {
        std::string folderName = folderPath.filename().string();
        LoadShaderDefinitionsFromFile(folderPath, folderName);  // Loads into g_shaderDefinitions
    }
    // Sort definitions by priority
    g_shaderDefinitions.SortByPriority();
    // Start HLSL watchers for definitions
    if (DEVELOPMENT) {
        REX::INFO("ReloadAllShaderDefinitions_Internal: Setting up development file watchers for shader definitions...");
        for (auto* def : g_shaderDefinitions.definitions) {
            if (def->active && !def->shaderFile.empty()) {
                def->hlslFileWatcher = std::make_unique<HlslFileWatcher>(def->shaderFile, def);
                def->hlslFileWatcher->Start();
            }
        }
    }
    // Reconnect ShaderDB <> ShaderDefDB based on new definitions
    RematchAllShaders_Internal();
    if (DEBUGGING) {
        REX::INFO("ReloadAllShaderDefinitions_Internal: Completed hot reload of shader definitions.");
    }
}

// Message handler definition
void F4SEMessageHandler(F4SE::MessagingInterface::Message *a_message) {
    switch (a_message->type) {
        case F4SE::MessagingInterface::kPostLoad:
            REX::INFO("Received kMessage_PostLoad. Game data is now loaded!");
            break;
        case F4SE::MessagingInterface::kPostPostLoad:
            REX::INFO("Received kMessage_PostPostLoad. Game data finished loading.");
            break;
        case F4SE::MessagingInterface::kGameLoaded:
            REX::INFO("Received kMessage_GameLoaded. A save game has been loaded.");
            break;
        case F4SE::MessagingInterface::kGameDataReady:
            REX::INFO("Received kMessage_GameDataReady. Game data is ready.");
            // Get the global data handle and interfaces
            g_dataHandle = RE::TESDataHandler::GetSingleton();
            if (g_dataHandle) {
                REX::INFO("TESDataHandler singleton acquired successfully.");
            } else {
                REX::WARN("Failed to acquire TESDataHandler singleton.");
            }
            // Install graphics hooks
            if (!GFX_HOOKS_INSTALLED) {
                if (InstallGFXHooks_Internal()) {
                    GFX_HOOKS_INSTALLED = true;
                    REX::INFO("GFX hooks installed successfully on kMessage_GameDataReady.");
                }
            } else {
                REX::WARN("Failed to install GFX hooks on kMessage_GameDataReady.");
            }
            break;
        case F4SE::MessagingInterface::kPostLoadGame:
            REX::INFO("Received kMessage_PostLoadGame. A save game has been loaded.");
            break;
        case F4SE::MessagingInterface::kNewGame:
            REX::INFO("Received kMessage_NewGame. A new game has been started.");
            // Install graphics hooks
            if (!GFX_HOOKS_INSTALLED) {
                if (InstallGFXHooks_Internal()) {
                    GFX_HOOKS_INSTALLED = true;
                    REX::INFO("GFX hooks installed successfully on kMessage_NewGame.");
                }
            } else {
                REX::WARN("Failed to install GFX hooks on kMessage_NewGame.");
            }
            break;
    }
}

// --- F4SE Entry Points - MUST have C linkage for F4SE to find them ---
extern "C"
{ // This block ensures C-style (unmangled) names for the linker

    F4SE_API bool F4SEPlugin_Query(const F4SE::QueryInterface *f4se, F4SE::PluginInfo *info)
    {
        // Set the plugin information
        // This is crucial to load the plugin
        info->infoVersion = F4SE::PluginInfo::kVersion;
        info->name = Version::PROJECT.data();
        info->version = Version::MAJOR;
        // Set up the logger
        // F4SE::log::log_directory().value(); == Documents/My Games/F4SE/
        std::filesystem::path logPath = F4SE::log::log_directory().value();
        logPath = logPath.parent_path() / "Fallout4" / "F4SE" / g_logName;
        // Create the file
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(), true);
        auto aLog = std::make_shared<spdlog::logger>("aLog"s, sink);
        // Configure the logger
        aLog->set_level(spdlog::level::info);
        aLog->flush_on(spdlog::level::info);
        // Set pattern
        aLog->set_pattern("[%T] [%^%l%$] %v"s);
        // Register to make it global accessable
        spdlog::register_logger(aLog);
        // Assign to global pointer
        gLog = spdlog::get("aLog");
        // First log
        REX::INFO("{}: Plugin Query started.", Version::PROJECT);
        // Minimum version 1.10.163
        //const auto ver = f4se->RuntimeVersion();
        //if (ver < F4SE::RUNTIME_1_10_162) {
        //    gLog->critical("unsupported runtime v{}", ver.string());
        //    return false;
        //}
        return true;
    }

    // This function is called after F4SE has loaded all plugins and the game is about to start.
    F4SE_API bool F4SEPlugin_Load(const F4SE::LoadInterface *f4se) {
        // Initialize the plugin with logger false to prevent F4SE to use its own logger
        F4SE::Init(f4se, false);
        // Log information
        REX::INFO("{}: Plugin loaded!", Version::PROJECT);
        REX::INFO("F4SE version: {}", F4SE::GetF4SEVersion().string());
        REX::INFO("Game runtime version: {}", f4se->RuntimeVersion().string());
        // Get the global plugin handle and interfaces
        g_pluginHandle = f4se->GetPluginHandle();
        g_messaging = F4SE::GetMessagingInterface();
        g_papyrus = F4SE::GetPapyrusInterface();
        // Get the DLL handle for this plugin
        HMODULE hModule = GetModuleHandleA(g_moduleName.c_str());
        // Fill the global plugin path variable for use in other parts of the plugin (like config loading and shader loading)
        g_pluginPath = GetPluginDirectory(hModule);
        // Load config
        LoadConfig(hModule);
        // Sort the shader definitions by priority (highest first)
        // So we match the definitions with then highest priority
        g_shaderDefinitions.SortByPriority();
        // Get the task interface
        g_taskInterface = F4SE::GetTaskInterface();
        // Allocate Trampoline size
        F4SE::AllocTrampoline(14);
        // Install the shader creation hooks very early.
        // Add a thread to run every 10ms until we succeed, since the game's renderer and device might not be fully initialized yet.
        std::thread([]() {
            const int maxAttempts = 2000;
            for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
                if (InstallShaderCreationHooks_Internal()) {
                    REX::INFO("Shader creation hooks installed successfully after {} attempts.", attempt);
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }).detach();
        // Get the scaleform interface
        g_scaleformInterface = F4SE::GetScaleformInterface();
        // Set the messagehandler to listen to events
        if (g_messaging && g_messaging->RegisterListener(F4SEMessageHandler, "F4SE")) {
            REX::INFO("Registered F4SE message handler.");
        } else {
            REX::WARN("Failed to register F4SE message handler.");
            return false;
        }
        return true;
    }

    F4SE_API void F4SEPlugin_Release() {
        // This is a new function for cleanup. It is called when the plugin is unloaded.
        REX::INFO("%s: Plugin released.", Version::PROJECT);
        gLog->flush();
        spdlog::drop_all();
        // Stop all file watchers
        for (auto* def : g_shaderDefinitions.definitions) {
            if (def->hlslFileWatcher) {
                def->hlslFileWatcher->Stop();
                def->hlslFileWatcher.reset();
            }
        }
        // Stop all INI watchers
        for (auto& [path, watcher] : g_iniWatchers) {
            if (watcher) {
                watcher->Stop();
                watcher.reset();
            }
        }
        // Clear Shader resources
        if (g_customSRV)       { g_customSRV->Release();       g_customSRV = nullptr; }
        if (g_customSRVBuffer) { g_customSRVBuffer->Release(); g_customSRVBuffer = nullptr; }
    }
}
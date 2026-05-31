#include <PCH.h>
#include <Global.h>

const char* defaultIni = R"(
; Enable/disable debugging of the plugin
; This is extensive debugging for the plugin
DEBUGGING=false
; Enable/disable custom resource view updates and bindings for replaced shaders
; This setting applies to all replaced shaders
; Shaders with #include "common.inc" have access to ingame data like FPS, Camera position and shader settings
CUSTOMBUFFER_ON=true
; Custom depth SRV slot in shader (default t30)
DEPTHBUFFER_SLOT=30
; Custom resource view slot in shader (beyond what the game uses, default t31)
CUSTOMBUFFER_SLOT=31
; --- SHADER SETTINGS ---
; Enable/disable settings menu
SHADERSETTINGS_ON=false
; MENU Hotkey Unused at the moment and defaults to the END key (ImGui hardcoded ENUM)
SHADERSETTINGS_MENUHOTKEY=VK_END
; Settings save Hotkey Unused at the moment and defaults to the HOME key (ImGui hardcoded ENUM)
SHADERSETTINGS_SAVEHOTKEY=VK_HOME
; Menu width in pixels
SHADERSETTINGS_WIDTH=600
; Menu height in pixels
SHADERSETTINGS_HEIGHT=300
; Shader settings menu opacity (0.0 - 1.0)
SHADERSETTINGS_OPACITY=0.75
; --- DEVELOPMENT SETTINGS ---
; Enable/disable development features like dump/log shaders
; This also enables file watchers for the INI and hlsl files
DEVELOPMENT=false
; Enable/disable Development GUI ingame (it may render the UI window on ingame textures like NPC foreheads :))
; Without this enabled, the entire ImGui initialization is skipped
DEVGUI_ON=false
; Development GUI Width
DEVGUI_WIDTH=600
; Development GUI Height
DEVGUI_HEIGHT=300
; Development GUI opacity (0.0 - 1.0)
DEVGUI_OPACITY=0.75

; Folder structure
; /F4SE/Plugins/ShaderEngine.ini - main configuration file for shader replacement rules
; /F4SE/Plugins/ShaderEngineDumps/<ShaderDefinition ID>/ - folder with dumped original shaders for analysis
; /F4SE/Plugins/ShaderEngine/<ShaderDefinition>/ - folder for replacement shaders
; /F4SE/Plugins/ShaderEngine/<ShaderDefinition>/Shader.ini - settings for the replacement shader, see below for example format
; /F4SE/Plugins/ShaderEngine/<ShaderDefinition>/Values.ini - settings for shader values, see below for example format
; /F4SE/Plugins/ShaderEngine/<ShaderDefinition>/<Shadername>.ps.hlsl - example replacement pixel shader in HLSL
; /F4SE/Plugins/ShaderEngine/<ShaderDefinition>/<Shadername>.vs.hlsl - example replacement vertex shader in HLSL

;Dimensions:
; D3D11_SRV_DIMENSION_TEXTURE1D = 3
; D3D11_SRV_DIMENSION_TEXTURE2D = 4
; D3D11_SRV_DIMENSION_TEXTURE2DMS = 6
; D3D11_SRV_DIMENSION_TEXTURE3D = 7
; D3D11_SRV_DIMENSION_TEXTURECUBE = 8
; D3D11_SRV_DIMENSION_TEXTURE1DARRAY = 4
; D3D11_SRV_DIMENSION_TEXTURE2DARRAY = 5
; D3D11_SRV_DIMENSION_TEXTURECUBEARRAY = 11

; Example shader definition in /F4SE/Plugins/ShaderEngine/<ShaderDefinitionName>/Shader.ini
;[loadingScreen]             ; unique ShaderDefinition ID for this replacement rule, whitespace is removed for parsing
;active=true                 ; whether this shader replacement rule is active
;priority=0                  ; priority of this rule for matching when multiple rules could apply (lower number = higher priority)
;type=ps                     ; shader type (vs=vertex, ps=pixel) defaults to ps if not specified
;shaderUID=PS1A2B3C4DI3O2    ; Unique identifier for the shader, used for matching and logging, can be more than one comma separated values
;hash=0x8D118ECC             ; vector of exact match of expected hash of the original shader bytecode for detection (can be obtained from logs or dumps)
;asmHash=0x12345678          ; vector of exact match of expected hash of the original shader assembly for detection (can be obtained from logs or dumps)
;size=(>1024), (<4096)       ; size definitions for the shader bytecode, can specify multiple separated by commas for multiple acceptable sizes (e.g. (512), (>1024), (<4096)), or leave empty to ignore size check
;buffersize=368@2            ; exact match of expected buffer size for the shader (size@slot), can specify multiple separated by commas for multiple buffers
;textures=2,4                ; list of texture register slots used by the shader (e.g. 0,1,2 or 4,5 for t0,t1,t2 or t4,t5)
;textureDimensions=4@2,8@4   ; texture dimension @ slot (e.g. 4@2 = Texture2D at t2, 8@4 = TextureCube at t4). Dimensions: 1D=3, 2D=4, 2DMS=6, 3D=7, Cube=8, 2DArray=5, CubeArray=11
;textureSlotMask=0x14        ; bitmask for required texture slots (bit i=1 if ti required; 0x14 = t2,t4)
;textureDimensionMask=0x110  ; bitmask for texture dimensions (bit i=1 if dimension i used; 0x110 = Texture2D(4) + Cube(8))
;inputTextureCount=(>0)      ; input texture count definitions for the shader, can specify multiple separated by commas (e.g. (0), (>0), (<4)), or leave empty to ignore input texture count check
;inputcount=(>7)             ; non texture inputcount definitions for the shader, can specify multiple separated by commas (e.g. (8), (>4), (<16)), or leave empty to ignore input count check
;inputMask=0x0               ; match of the bitmask for required input registers (bit i is 1 if input register i is required)
;outputcount=(1)             ; outputcount definitions for the shader, can specify multiple separated by commas (e.g. (1), (>0), (<4)), or leave empty to ignore output count check
;outputMask=0x1              ; match of the bitmask for required output registers (bit i is 1 if output register o[i] is required)
;shader=GFXBoosterLS.hlsl    ; the replacement shader file name in the shader definition folder, CANNOT have white spaces in the filename and must be a .hlsl file
;log=true                    ; whether to log shader detection and reflection details to the F4SE logs for this shader replacement rule
;dump=true                   ; whether to dump the original shader for analysis to the ShaderEngineDumps folder for this shader replacement rule (existing dumps files will not be overwritten, but skipped)
;[/loadingScreen]

; Adding #include "common.inc" to the replacement shader gives access to:
;    float    GFXInjected[0].g_Time
;    float    GFXInjected[0].g_Delta
;    float    GFXInjected[0].g_DayCycle
;    float    GFXInjected[0].g_Frame
;    float    GFXInjected[0].g_FPS
;    float    GFXInjected[0].g_ResX
;    float    GFXInjected[0].g_ResY
;    float    GFXInjected[0].g_MouseX
;    float    GFXInjected[0].g_MouseY
;    float    GFXInjected[0].g_WindSpeed    // updated every 30 frames
;    float    GFXInjected[0].g_WindAngle    // updated every 30 frames
;    float    GFXInjected[0].g_WindTurb     // updated every 30 frames
;    float4   GFXInjected[0].g_ViewPort
;    float3   GFXInjected[0].g_CameraPos
;    float    GFXInjected[0].g_RadExp       // rad dmg taken over 30 frames
;    float3   GFXInjected[0].g_ViewDir
;    float    GFXInjected[0].g_Random       // random value updated every frame
;    float    GFXInjected[0].g_Combat       // updated every 30 frames
;    float    GFXInjected[0].g_Interior     // updated every 30 frames
;    float    GFXInjected[0].g_HealthPerc   // updated every 30 frames
;    float4   GFXInjected[0].g_ViewMatrixRow0
;    float4   GFXInjected[0].g_ViewMatrixRow1
;    float4   GFXInjected[0].g_ViewMatrixRow2
;    float4   GFXInjected[0].g_ViewMatrixRow3
;    float4   GFXInjected[0].g_ProjMatrixRow0
;    float4   GFXInjected[0].g_ProjMatrixRow1
;    float4   GFXInjected[0].g_ProjMatrixRow2
;    float4   GFXInjected[0].g_ProjMatrixRow3
;    float4   GFXInjected[0].g_ViewProjMatrixRow0
;    float4   GFXInjected[0].g_ViewProjMatrixRow1
;    float4   GFXInjected[0].g_ViewProjMatrixRow2
;    float4   GFXInjected[0].g_ViewProjMatrixRow3
;    float4   GFXInjected[0].g_InvViewProjMatrixRow0
;    float4   GFXInjected[0].g_InvViewProjMatrixRow1
;    float4   GFXInjected[0].g_InvViewProjMatrixRow2
;    float4   GFXInjected[0].g_InvViewProjMatrixRow3
;    float4   GFXInjected[0].g_InvProjMatrixRow0
;    float4   GFXInjected[0].g_InvProjMatrixRow1
;    float4   GFXInjected[0].g_InvProjMatrixRow2
;    float4   GFXInjected[0].g_InvProjMatrixRow3
;    float2   GFXInjected[0].g_ViewDepthRange
;    float    GFXInjected[0].modularFloats[0..199]
;    int      GFXInjected[0].modularInts[0..99]
;    int      GFXInjected[0].modularBools[0..99]

; Settings for shaders can be defined in the Values.ini file in the shader definition folder
; Globals are at the top of the menu, while locals are grouped with other values of the shader definition
;[global]
;id=g_TAAEnabled          ; the name of the variable in the shader to set, e.g. g_TAAEnabled
;label="TAA Enabled"      ; the label to show in the menu for this setting
;type=bool                ; the type of the variable (bool, int, float)
;value=true               ; the default value to set (true/false for bool, numeric value for int and float)
;[/global]
;[local]
;id=g_SomeFloatValue      ; the name of the variable in the shader to set, e.g. g_SomeFloatValue
;label="Some Float Value" ; the label to show in the menu for this setting
;type=float               ; the type of the variable (bool, int, float)
;value=0.5                ; the default value to set (true/false for bool, numeric value for int and float)
;min=0.0                  ; optional minimum value for float and int types
;max=1.0                  ; optional maximum value for float and int types
;step=0.1                 ; optional step value for float and int types
;[/local]
;[local]
;id=g_SomeIntValue        ; the name of the variable in the shader to set, e.g. g_SomeIntValue
;label="Some Int Value"   ; the label to show in the menu for this setting
;type=int                 ; the type of the variable (bool, int, float)
;value=5                  ; the default value to set (true/false for bool, numeric value for int and float)
;min=0                    ; optional minimum value for float and int types
;max=10                   ; optional maximum value for float and int types
;step=1                   ; optional step value for float and int types
;[/local]
)";

const char* flashPixelShaderHLSL = R"(
// Pixel Shader that outputs a bright neon green color for testing shader replacement.
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 extra0 : TEXCOORD0;
    float4 extra1 : TEXCOORD1;
};
float4 main(PS_INPUT input) : SV_Target {
    // High-intensity Neon Green (Values > 1.0 trigger Bloom/HDR)
    // Change this to float3(10.0, 0.0, 10.0) for Magenta
    float3 neonColor = float3(0.0, 10.0, 0.5); 
    return float4(neonColor, 1.0);
}
)";

std::string GetCommonShaderHeaderHLSLTop()
{
    return R"(
        // Data passed from the plugin as resource view
        struct GFXBoosterAccessData
        {
            // Block 0 (Bytes 0-15)
            float    g_Time;
            float    g_Delta;
            float    g_DayCycle;
            float    g_Frame;

            // Block 1 (Bytes 16-31)
            float    g_FPS;
            float    g_ResX;
            float    g_ResY;
            float    g_MouseX;

            // Block 2 (Bytes 32-47)
            float    g_MouseY;
            float    g_WindSpeed;
            float    g_WindAngle;
            float    g_WindTurb;

            // Block 3 (Bytes 48-63)
            float4   g_ViewPort;

            // Block 4 (Bytes 64-79)
            float3   g_CameraPos;
            float    g_RadExp;

            // Block 5 (Bytes 80-95)
            float3   g_ViewDir;
            float    g_Random;

            // Block 6 (Bytes 96-159)
            float    g_Combat;
            float    g_Interior;
            float    _padding;
            float    g_HealthPerc;

            float4 g_ViewMatrixRow0;
            float4 g_ViewMatrixRow1;
            float4 g_ViewMatrixRow2;
            float4 g_ViewMatrixRow3;

            float4 g_ProjMatrixRow0;
            float4 g_ProjMatrixRow1;
            float4 g_ProjMatrixRow2;
            float4 g_ProjMatrixRow3;

            float4 g_ViewProjMatrixRow0;
            float4 g_ViewProjMatrixRow1;
            float4 g_ViewProjMatrixRow2;
            float4 g_ViewProjMatrixRow3;

            float4 g_InvViewProjMatrixRow0;
            float4 g_InvViewProjMatrixRow1;
            float4 g_InvViewProjMatrixRow2;
            float4 g_InvViewProjMatrixRow3;

            float4 g_InvProjMatrixRow0;
            float4 g_InvProjMatrixRow1;
            float4 g_InvProjMatrixRow2;
            float4 g_InvProjMatrixRow3;

            float2 g_ViewDepthRange;

            // Shader settings — fixed-size arrays, layout matches C++ struct exactly
            float modularFloats[200];
            int   modularInts[100];
            int   modularBools[100];
        };
        )";
}

// Here will be the dynamic Shader Settings values defined

std::string GetCommonShaderHeaderHLSLBottom()
{
    return std::string(R"(
        StructuredBuffer<GFXBoosterAccessData> GFXInjected : register(t)") +
        // Adding the custom buffer slot from the INI
        // The line will produce: StructuredBuffer<GFXBoosterAccessData> GFXInjected : register(t14);
        std::to_string(CUSTOMBUFFER_SLOT) +
        std::string(R"();

        // --- Coordinate Space Helpers ---

        // Transforms screen UV and raw depth into world-space coordinates using the inverse View Projection matrix from the injected data.
        float3 ReconstructWorldPos(float2 uv, float depth) {
            float2 ndc = uv * 2.0 - 1.0;
            ndc.y = -ndc.y; // keep or remove depending on whether your UV->NDC flip is already handled
            float4 clipPos = float4(ndc, depth, 1.0);
            float4x4 invViewProj = float4x4(
                GFXInjected[0].g_InvViewProjMatrixRow0,
                GFXInjected[0].g_InvViewProjMatrixRow1,
                GFXInjected[0].g_InvViewProjMatrixRow2,
                GFXInjected[0].g_InvViewProjMatrixRow3
            );
            float4 worldH = mul(clipPos, invViewProj);
            return worldH.xyz / worldH.w;
        }

        // Transforms world-space coordinates into screen UV and depth using the View Projection matrix from the injected data.
        float3 ReconstructScreenPos(float3 worldPos) {
            float4 worldH = float4(worldPos, 1.0);
            float4x4 viewProj = float4x4(
                GFXInjected[0].g_ViewProjMatrixRow0,
                GFXInjected[0].g_ViewProjMatrixRow1,
                GFXInjected[0].g_ViewProjMatrixRow2,
                GFXInjected[0].g_ViewProjMatrixRow3
            );
            float4 clipPos = mul(worldH, viewProj);
            clipPos /= clipPos.w; // perspective divide
            float2 uv = clipPos.xy * 0.5 + 0.5;
            return float3(uv, clipPos.z); // returning UV and depth
        }

        // --- Color Conversion Helpers ---

        // Generates an RGB spectrum color based on a 0.0-1.0 hue input.
        float3 HueToRGB(float h) {
            float r = abs(h * 6.0 - 3.0) - 1.0;
            float g = 2.0 - abs(h * 6.0 - 2.0);
            float b = 2.0 - abs(h * 6.0 - 4.0);
            return saturate(float3(r, g, b));
        }

        // Returns the perceptual brightness of an RGB color using standard luminance weights.
        float GetLuma(float3 rgb) {
            return dot(rgb, float3(0.299, 0.587, 0.114));
        }

        // Performs a three-way linear interpolation across a color gradient (a to b to c).
        float3 Lerp3(float3 a, float3 b, float3 c, float t) {
            if (t < 0.5) return lerp(a, b, t * 2.0);
            return lerp(b, c, (t - 0.5) * 2.0);
        }

        // --- Other helpers ---

        // ditherValue: 0 to 100 (0 = fully opaque, 100 = fully transparent)
        void transparentDither(uint2 pixelPos, float transparency) {
            // A simple 2x2 checkerboard logic
            // This creates the "grain" look
            bool checker = ((pixelPos.x + pixelPos.y) % 2 == 0);
            // For 50% transparency (your sweet spot)
            if (transparency >= 50.0) {
                if (checker) discard;
            }
            // For higher transparency (like your 97% ghost shader)
            // We add an extra skip to thin out the remaining 50%
            if (transparency > 75.0) {
                if ((pixelPos.x % 2 == 0) || (pixelPos.y % 2 == 0)) discard;
            }
        }

        float2 GetWindDir() {
            float s, c;
            // Using sincos to transform your scalar angle into a 2D vector
            sincos(GFXInjected[0].g_WindAngle, s, c);
            return float2(c, s);
        }

        float2 GetWindFlow(float speedMult) {
            return GetWindDir() * (GFXInjected[0].g_Time * GFXInjected[0].g_WindSpeed * speedMult);
        }

        // --- Math Helpers ---

        // Produces a static, deterministic pseudo-random value based strictly on UV coordinates.
        float RandomHash(float2 uv) {
            return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
        }

        // Produces a dynamic pseudo-random value that changes every frame using the injected random seed.
        float RandomTemporal(float2 uv) {
            return frac(sin(dot(uv + GFXInjected[0].g_Random, float2(12.9898, 78.233))) * 43758.5453);
        }

        // Maps a numeric value from an input range [min1, max1] to an output range [min2, max2].
        float Remap(float value, float min1, float max1, float min2, float max2) {
            return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
        }

        // Converts a non-linear raw depth buffer value into a linear 0.0 to 1.0 distance.
        float GetLinearDepth(float rawDepth) {
            // clip = (ndc.x=0, ndc.y=0, ndc.z=rawDepth, w=1)
            float4 clipPos = float4(0, 0, rawDepth, 1.0);
            float4x4 invProj = float4x4(
            GFXInjected[0].g_InvProjMatrixRow0,
            GFXInjected[0].g_InvProjMatrixRow1,
            GFXInjected[0].g_InvProjMatrixRow2,
            GFXInjected[0].g_InvProjMatrixRow3
            );
            float4 viewPos = mul(clipPos, invProj);
            viewPos /= viewPos.w; // important
            // Now viewPos.z is the view-space z. Return signed or absolute depending on convention:
            // If camera looks along -Z: return -viewPos.z;
            // If camera looks along +Z: return viewPos.z;
            return abs(viewPos.z);
        }
        )");
}

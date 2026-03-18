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
; Custom resource view slot in shader (beyond what the game uses, default t14)
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
; /F4SE/Plugins/GFXBoosterCL.ini - main configuration file for shader replacement rules
; /F4SE/Plugins/GFXBoosterDumps/<ShaderDefinition ID>/ - folder with dumped original shaders for analysis
; /F4SE/Plugins/GFXBoosterCL/<ShaderDefinition>/ - folder for replacement shaders
; /F4SE/Plugins/GFXBoosterCL/<ShaderDefinition>/Shader.ini - settings for the replacement shader, see below for example format
; /F4SE/Plugins/GFXBoosterCL/<ShaderDefinition>/Values.ini - settings for shader values, see below for example format
; /F4SE/Plugins/GFXBoosterCL/<ShaderDefinition>/<Shadername>.ps.hlsl - example replacement pixel shader in HLSL
; /F4SE/Plugins/GFXBoosterCL/<ShaderDefinition>/<Shadername>.vs.hlsl - example replacement vertex shader in HLSL

;Dimensions:
; D3D11_SRV_DIMENSION_TEXTURE1D = 3
; D3D11_SRV_DIMENSION_TEXTURE2D = 4
; D3D11_SRV_DIMENSION_TEXTURE2DMS = 6
; D3D11_SRV_DIMENSION_TEXTURE3D = 7
; D3D11_SRV_DIMENSION_TEXTURECUBE = 8
; D3D11_SRV_DIMENSION_TEXTURE1DARRAY = 4
; D3D11_SRV_DIMENSION_TEXTURE2DARRAY = 5
; D3D11_SRV_DIMENSION_TEXTURECUBEARRAY = 11

; Example shader definition in /F4SE/Plugins/GFXBoosterCL/<ShaderDefinitionName>/Shader.ini
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
;dump=true                   ; whether to dump the original shader for analysis to the GFXBoosterDumps folder for this shader replacement rule (existing dumps files will not be overwritten, but skipped)
;[/loadingScreen]

; Adding #include "common.inc" to the replacement shader gives access to:
;    float    GFXInjected[0].g_Time
;    float    GFXInjected[0].g_Delta;
;    float    GFXInjected[0].g_DayCycle;
;    float    GFXInjected[0].g_Frame;
;    float    GFXInjected[0].g_FPS;
;    float    GFXInjected[0].g_ResX;
;    float    GFXInjected[0].g_ResY;
;    float    GFXInjected[0].g_MouseX;
;    float    GFXInjected[0].g_MouseY;
;    float    GFXInjected[0].g_WindSpeed; // updated every 30 frames
;    float    GFXInjected[0].g_WindAngle; // updated every 30 frames
;    float    GFXInjected[0].g_WindTurb; // updated every 30 frames
;    float    GFXInjected[0].g_VpLeft;
;    float    GFXInjected[0].g_VpTop;
;    float    GFXInjected[0].g_VpWidth;
;    float    GFXInjected[0].g_VpHeight;
;    float3   GFXInjected[0].g_CameraPos;
;    float    GFXInjected[0].g_RadExp; // rad dmg taken over 30 frames
;    float3   GFXInjected[0].g_ViewDir;
;    float    GFXInjected[0].g_HealthPerc; // updated every 30 frames
;    float4   GFXInjected[0].g_InvProjRow0;
;    float4   GFXInjected[0].g_InvProjRow1;
;    float4   GFXInjected[0].g_InvProjRow2;
;    float4   GFXInjected[0].g_InvProjRow3;
;    float    GFXInjected[0].g_Random; // random value updated every frame
;    float    GFXInjected[0].g_Combat; // updated every 30 frames
;    float    GFXInjected[0].g_Interior; // updated every 30 frames
;    float4   GFXInjected[0].g_ViewProjRow0;
;    float4   GFXInjected[0].g_ViewProjRow1;
;    float4   GFXInjected[0].g_ViewProjRow2;
;    float4   GFXInjected[0].g_ViewProjRow3;

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
            float    g_VpLeft;
            float    g_VpTop;
            float    g_VpWidth;
            float    g_VpHeight;

            // Block 4 (Bytes 64-79)
            float3   g_CameraPos;
            float    g_RadExp;

            // Block 5 (Bytes 80-95)
            float3   g_ViewDir;
            float    g_HealthPerc;

            // Block 6 (Bytes 96-159)
            float4   g_InvProjRow0;
            float4   g_InvProjRow1;
            float4   g_InvProjRow2;
            float4   g_InvProjRow3;

            // Block 7 (Bytes 160-175)
            float    g_Random;
            float    g_Combat;
            float    g_Interior;
            float    _padding;

            // Block 8 (Bytes 176-239)
            float4   g_ViewProjRow0;
            float4   g_ViewProjRow1;
            float4   g_ViewProjRow2;
            float4   g_ViewProjRow3;

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

        // Transforms screen UV and raw depth into world-space coordinates using the inverse projection matrix.
        float3 ReconstructWorldPos(float2 uv, float rawDepth)
        {
            float4 clipPos;
            clipPos.x = uv.x * 2.0 - 1.0;
            clipPos.y = (1.0 - uv.y) * 2.0 - 1.0; 
            clipPos.z = rawDepth;
            clipPos.w = 1.0;
            float4x4 invProj = float4x4(
                GFXInjected[0].g_InvProjRow0,
                GFXInjected[0].g_InvProjRow1,
                GFXInjected[0].g_InvProjRow2,
                GFXInjected[0].g_InvProjRow3
            );
            float4 worldPos = mul(clipPos, invProj);
            return worldPos.xyz / worldPos.w;
        }

        // --- Color Conversion Helpers ---

        // Generates an RGB spectrum color based on a 0.0-1.0 hue input.
        float3 HueToRGB(float h)
        {
            float r = abs(h * 6.0 - 3.0) - 1.0;
            float g = 2.0 - abs(h * 6.0 - 2.0);
            float b = 2.0 - abs(h * 6.0 - 4.0);
            return saturate(float3(r, g, b));
        }

        // Returns the perceptual brightness of an RGB color using standard luminance weights.
        float GetLuma(float3 rgb)
        {
            return dot(rgb, float3(0.299, 0.587, 0.114));
        }

        // Performs a three-way linear interpolation across a color gradient (a to b to c).
        float3 Lerp3(float3 a, float3 b, float3 c, float t)
        {
            if (t < 0.5) return lerp(a, b, t * 2.0);
            return lerp(b, c, (t - 0.5) * 2.0);
        }

        // --- Other helpers ---

        // ditherValue: 0 to 100 (0 = fully opaque, 100 = fully transparent)
        void transparentDither(uint2 pixelPos, float transparency)
        {
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

        float2 GetWindDir()
        {
            float s, c;
            // Using sincos to transform your scalar angle into a 2D vector
            sincos(GFXInjected[0].g_WindAngle, s, c);
            return float2(c, s);
        }

        float2 GetWindFlow(float speedMult)
        {
            return GetWindDir() * (GFXInjected[0].g_Time * GFXInjected[0].g_WindSpeed * speedMult);
        }

        // --- Math Helpers ---

        // Produces a static, deterministic pseudo-random value based strictly on UV coordinates.
        float RandomHash(float2 uv)
        {
            return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
        }

        // Produces a dynamic pseudo-random value that changes every frame using the injected random seed.
        float RandomTemporal(float2 uv)
        {
            return frac(sin(dot(uv + GFXInjected[0].g_Random, float2(12.9898, 78.233))) * 43758.5453);
        }

        // Maps a numeric value from an input range [min1, max1] to an output range [min2, max2].
        float Remap(float value, float min1, float max1, float min2, float max2)
        {
            return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
        }

        // Converts a non-linear raw depth buffer value into a linear 0.0 to 1.0 distance.
        float GetLinearDepth(float rawDepth) {
            // We reconstruct a point at the center of the screen at the given depth
            float4 clipPos = float4(0, 0, rawDepth, 1.0);
            float4x4 invProj = float4x4(
                GFXInjected[0].g_InvProjRow0,
                GFXInjected[0].g_InvProjRow1,
                GFXInjected[0].g_InvProjRow2,
                GFXInjected[0].g_InvProjRow3
            );
            float4 viewPos = mul(clipPos, invProj);
            // The W component after inverse projection is actually the Linear Eye Depth!
            return viewPos.w; 
        }

        // Calculates UV offsets to simulate surface depth/relief without requiring extra geometry.
        float2 ApplyParallax(float2 uv, float3 viewDirTS, float3 worldPos, float scale, Texture2D heightMap, SamplerState sam) {
            // Distance Culling
            float dist = distance(worldPos, GFXInjected[0].g_CameraPos);
            if (dist > 2000.0) return uv; 

            // Initial Height
            float h = heightMap.SampleLevel(sam, uv, 0.5).y;
            
            // Anchor the scale to the angle
            // We use (1 - viewDirTS.z) to make sure top-down views have ZERO shift.
            float angleFactor = saturate(1.0 - viewDirTS.z);
            float finalScale = scale * angleFactor * saturate((h - 0.2) / 0.5);

            // THE PROJECTED OFFSET
            // We multiply by xy and ignore the division.
            float2 maxOffset = viewDirTS.xy * finalScale;

            // The Search Loop
            const float numSteps = 12.0;
            float2 uvStep = maxOffset / numSteps;
            float stepSize = 1.0 / numSteps;
            
            float2 currentUV = uv;
            float currentLayerDepth = 0.0;
            float prevHeight = h;

            [loop]
            for(int i = 0; i < 12; i++) {
                // Sample at Mip 1.0 to smoothen the turret edges
                float currentHeight = heightMap.SampleLevel(sam, currentUV, 1.0).y;
                if(currentLayerDepth < currentHeight) {
                    prevHeight = currentHeight;
                    currentUV -= uvStep;
                    currentLayerDepth += stepSize;
                } else {
                    float nextHeight = currentHeight;
                    float prevLayerDepth = currentLayerDepth - stepSize;
                    float weight = (currentLayerDepth - nextHeight) / (abs((nextHeight - prevHeight) + (prevLayerDepth - currentLayerDepth)) + 1e-6);
                    return lerp(currentUV, currentUV + uvStep, weight);
                }
            }

            return currentUV;
        }
        )");
}

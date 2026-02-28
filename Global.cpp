#include <PCH.h>
#include <Global.h>

const char* defaultIni = R"(
; Enable/disable debugging of the plugin
DEBUGGING=false
; Enable/disable custom CBuffer updates and bindings each frame
CUSTOMBUFFER=true
; --- DEVELOPMENT SETTINGS ---
; Enable/disable development features like dump/log shaders
DEVELOPMENT=false
; Enable/disable Development GUI ingame
DEVGUI=false
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
;hash=0x8D118ECC             ; vector of exact match of expected hash of the original shader bytecode for detection (can be obtained from logs or dumps)
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
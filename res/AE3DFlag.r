/*
 *  AE3DFlag.r  —  PiPL resource for the 3D Flag After Effects plug-in
 *
 *  Compile on macOS with Rez (included in the AE SDK) or the AE SDK's
 *  PiPLtool utility (generates compilable C code from this file).
 *
 *  AE SDK path:  AESDK/Resources/
 */

#include "AEConfig.h"
#include "AE_General.r"
#include "AE_EffectVers.h"

#define plugInName      "3D Flag"
#define plugInCategory  "Bewegtbildgrafik"
#define matchName       "BBGF_3DFlag"
#define plugInDescription "Realistic looping 3-D flag simulation"

#define MAJOR_VERSION   1
#define MINOR_VERSION   0
#define BUG_VERSION     0
#define STAGE_VERSION   PF_Stage_DEVELOP
#define BUILD_VERSION   1

resource 'PiPL' (16000) {
    {
        Kind { AEGP },
        Name { plugInName },
        Category { plugInCategory },

        /* ---- Platform-specific code resources ---- */
#ifdef AE_OS_WIN
    #ifdef AE_PROC_INTELx64
        CodeWin64X86 { "EffectMain" },
    #endif
#else
    #ifdef AE_PROC_INTELx64
        CodeMacIntel64 { "EffectMain" },
    #endif
    #ifdef AE_PROC_ARM64
        CodeMacARM64 { "EffectMain" },
    #endif
#endif

        AE_PiPL_Version { 2, 0 },
        AE_Effect_Spec_Version { PF_PLUG_IN_VERSION, PF_PLUG_IN_SUBVERS },
        AE_Effect_Version {
            MAJOR_VERSION, MINOR_VERSION, BUG_VERSION,
            STAGE_VERSION, BUILD_VERSION
        },
        AE_Effect_Info_Flags { 0 },
        AE_Effect_Global_InFlags { 0 },
        AE_Effect_Global_InFlags2 {
            PF_InFlag2_SUPPORTS_SMART_RENDER |
            PF_InFlag2_FLOAT_COLOR_AWARE     |
            PF_InFlag2_SUPPORTS_THREADED_RENDERING
        },
        AE_Effect_Match_Name { matchName },
        AE_Reserved_Info { 8 },
        AE_Effect_Support_URL { "https://bewegtbildgrafik.de" },
    }
};

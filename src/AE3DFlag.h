#pragma once

#include "AEConfig.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "AE_EffectCBSuites.h"
#include "AE_GeneralPlug.h"
#include "Param_Utils.h"

#define PLUGIN_NAME         "3D Flag"
#define PLUGIN_MATCH_NAME   "BBGF_3DFlag"
#define PLUGIN_CATEGORY     "Bewegtbildgrafik"

#define MAJOR_VERSION   1
#define MINOR_VERSION   0
#define BUG_VERSION     0
#define STAGE_VERSION   PF_Stage_DEVELOP
#define BUILD_VERSION   1

// Parameter indices — must be in the same order they are registered in ParamsSetup
enum ParamID {
    PARAM_INPUT = 0,

    PARAM_TEXTURE,

    PARAM_GROUP_DIM_START,
    PARAM_ASPECT_RATIO,
    PARAM_FILL_AMOUNT,
    PARAM_GROUP_DIM_END,

    PARAM_GROUP_WAVE_START,
    PARAM_AMPLITUDE,
    PARAM_FREQUENCY,
    PARAM_SPEED,
    PARAM_COMPLEXITY,
    PARAM_GROUP_WAVE_END,

    PARAM_GROUP_NOISE_START,
    PARAM_NOISE_AMOUNT,
    PARAM_NOISE_SCALE,
    PARAM_NOISE_SPEED,
    PARAM_GROUP_NOISE_END,

    PARAM_GROUP_LIGHT_START,
    PARAM_LIGHT_ANGLE,
    PARAM_LIGHT_ELEVATION,
    PARAM_LIGHT_INTENSITY,
    PARAM_AMBIENT,
    PARAM_GROUP_LIGHT_END,

    PARAM_MESH_QUALITY,

    NUM_PARAMS
};

extern "C" {
PF_Err EffectMain(
    PF_Cmd          cmd,
    PF_InData*      in_data,
    PF_OutData*     out_data,
    PF_ParamDef*    params[],
    PF_LayerDef*    output,
    void*           extra);
}

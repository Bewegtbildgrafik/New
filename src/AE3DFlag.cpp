#include "AE3DFlag.h"
#include "FlagRenderer.h"

// ---------------------------------------------------------------------------
// About box
// ---------------------------------------------------------------------------

static PF_Err About(PF_InData* in_data, PF_OutData* out_data)
{
    PF_SPRINTF(out_data->return_msg,
               "%s  v%d.%d\n"
               "Bewegtbildgrafik\n"
               "Realistic looping 3-D flag simulation.\n"
               "Place a texture layer in the 'Flag Texture' slot.",
               PLUGIN_NAME, MAJOR_VERSION, MINOR_VERSION);
    return PF_Err_NONE;
}

// ---------------------------------------------------------------------------
// Global setup — tell AE we support SmartRender and threaded rendering
// ---------------------------------------------------------------------------

static PF_Err GlobalSetup(PF_InData* in_data, PF_OutData* out_data)
{
    out_data->my_version = PF_VERSION(MAJOR_VERSION, MINOR_VERSION,
                                      BUG_VERSION,   STAGE_VERSION, BUILD_VERSION);

    out_data->out_flags  = PF_OutFlag_USE_OUTPUT_EXTENT
                         | PF_OutFlag_SEND_UPDATE_PARAMS_UI;

    out_data->out_flags2 = PF_OutFlag2_SUPPORTS_SMART_RENDER
                         | PF_OutFlag2_FLOAT_COLOR_AWARE
                         | PF_OutFlag2_SUPPORTS_THREADED_RENDERING;

    return PF_Err_NONE;
}

// ---------------------------------------------------------------------------
// Parameter registration
// ---------------------------------------------------------------------------

static PF_Err ParamsSetup(PF_InData* in_data, PF_OutData* out_data,
                           PF_ParamDef* params[], PF_LayerDef* output)
{
    PF_Err    err = PF_Err_NONE;
    PF_ParamDef def;

    // 1 — Flag texture layer
    AEFX_CLR_STRUCT(def);
    PF_ADD_LAYER("Flag Texture", PF_LayerDefault_NONE, PARAM_TEXTURE);

    // 2 — Dimensions group
    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Dimensions", PARAM_GROUP_DIM_START);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Aspect Ratio (W/H)",
        0.25, 4.0, 0.25, 4.0, 1.5,
        PF_Precision_HUNDREDTHS, 0, 0, PARAM_ASPECT_RATIO);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Fill Frame",
        0.1, 1.0, 0.1, 1.0, 0.85,
        PF_Precision_HUNDREDTHS, 0, 0, PARAM_FILL_AMOUNT);

    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(PARAM_GROUP_DIM_END);

    // 3 — Wave group
    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Wave", PARAM_GROUP_WAVE_START);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Amplitude",
        0.0, 400.0, 0.0, 400.0, 60.0,
        PF_Precision_INTEGER, 0, 0, PARAM_AMPLITUDE);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Frequency",
        0.25, 8.0, 0.25, 8.0, 2.0,
        PF_Precision_HUNDREDTHS, 0, 0, PARAM_FREQUENCY);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Speed (cycles/s)",
        0.05, 8.0, 0.05, 8.0, 0.8,
        PF_Precision_HUNDREDTHS, 0, 0, PARAM_SPEED);

    // Complexity popup: Low | Medium | High | Ultra
    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP("Wave Complexity", 4, 2,
                 "Low (1 harmonic)|"
                 "Medium (2 harmonics)|"
                 "High (3 harmonics)|"
                 "Ultra (4 harmonics)",
                 PARAM_COMPLEXITY);

    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(PARAM_GROUP_WAVE_END);

    // 4 — Organic noise group
    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Organic Noise", PARAM_GROUP_NOISE_START);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Noise Amount",
        0.0, 1.0, 0.0, 1.0, 0.35,
        PF_Precision_HUNDREDTHS, 0, 0, PARAM_NOISE_AMOUNT);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Noise Scale",
        0.1, 6.0, 0.1, 6.0, 1.5,
        PF_Precision_HUNDREDTHS, 0, 0, PARAM_NOISE_SCALE);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Noise Speed (cycles/s)",
        0.05, 4.0, 0.05, 4.0, 0.55,
        PF_Precision_HUNDREDTHS, 0, 0, PARAM_NOISE_SPEED);

    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(PARAM_GROUP_NOISE_END);

    // 5 — Lighting group
    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Lighting", PARAM_GROUP_LIGHT_START);

    AEFX_CLR_STRUCT(def);
    PF_ADD_ANGLE("Light Azimuth", 30, PARAM_LIGHT_ANGLE);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Light Elevation",
        0.0, 90.0, 0.0, 90.0, 45.0,
        PF_Precision_INTEGER, 0, 0, PARAM_LIGHT_ELEVATION);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Intensity",
        0.0, 2.0, 0.0, 2.0, 1.0,
        PF_Precision_HUNDREDTHS, 0, 0, PARAM_LIGHT_INTENSITY);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Ambient",
        0.0, 1.0, 0.0, 1.0, 0.25,
        PF_Precision_HUNDREDTHS, 0, 0, PARAM_AMBIENT);

    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(PARAM_GROUP_LIGHT_END);

    // 5 — Mesh quality
    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP("Mesh Quality", 4, 2,
                 "Low (16)|Medium (32)|High (64)|Ultra (128)",
                 PARAM_MESH_QUALITY);

    out_data->num_params = NUM_PARAMS;
    return err;
}

// ---------------------------------------------------------------------------
// Collect all parameters into a FlagParams struct
// ---------------------------------------------------------------------------

static FlagParams CollectParams(PF_InData* in_data, PF_ParamDef* params[])
{
    FlagParams fp;
    fp.time_seconds  = static_cast<float>(in_data->current_time)
                       / static_cast<float>(in_data->time_scale);

    fp.aspect_ratio  = static_cast<float>(params[PARAM_ASPECT_RATIO]->u.fs_d.value);
    fp.fill_amount   = static_cast<float>(params[PARAM_FILL_AMOUNT]->u.fs_d.value);

    fp.amplitude     = static_cast<float>(params[PARAM_AMPLITUDE]->u.fs_d.value);
    fp.frequency     = static_cast<float>(params[PARAM_FREQUENCY]->u.fs_d.value);
    fp.speed         = static_cast<float>(params[PARAM_SPEED]->u.fs_d.value);
    fp.complexity    = static_cast<int>  (params[PARAM_COMPLEXITY]->u.pd.value);  // 1-based

    fp.noise_amount  = static_cast<float>(params[PARAM_NOISE_AMOUNT]->u.fs_d.value);
    fp.noise_scale   = static_cast<float>(params[PARAM_NOISE_SCALE]->u.fs_d.value);
    fp.noise_speed   = static_cast<float>(params[PARAM_NOISE_SPEED]->u.fs_d.value);

    // Angle param stores value in degrees * 65536 (fixed-point)
    fp.light_angle     = static_cast<float>(params[PARAM_LIGHT_ANGLE]->u.ad.value)
                         / 65536.0f;
    fp.light_elevation = static_cast<float>(params[PARAM_LIGHT_ELEVATION]->u.fs_d.value);
    fp.light_intensity = static_cast<float>(params[PARAM_LIGHT_INTENSITY]->u.fs_d.value);
    fp.ambient         = static_cast<float>(params[PARAM_AMBIENT]->u.fs_d.value);

    // Mesh quality: popup value 1-4 → grid divisions 16/32/64/128
    const int qpop = static_cast<int>(params[PARAM_MESH_QUALITY]->u.pd.value);
    const int mesh_table[4] = {16, 32, 64, 128};
    fp.mesh_quality = mesh_table[std::max(1, std::min(4, qpop)) - 1];

    return fp;
}

// ---------------------------------------------------------------------------
// SmartRender — pre-render (register layer checkout)
// ---------------------------------------------------------------------------

static PF_Err SmartPreRender(PF_InData* in_data, PF_OutData* out_data,
                              PF_PreRenderExtra* extra)
{
    PF_Err err = PF_Err_NONE;

    PF_RenderRequest req = extra->input->output_request;
    req.field = PF_Field_FRAME;

    PF_CheckoutResult layer_result;
    ERR(extra->cb->checkout_layer(
            in_data->effect_ref,
            PARAM_TEXTURE,
            PARAM_TEXTURE,
            &req,
            in_data->current_time,
            in_data->time_step,
            in_data->time_scale,
            &layer_result));

    if (!err) {
        auto unionR = [](const PF_LRect& src, PF_LRect& dst) {
            if (src.left   < dst.left)   dst.left   = src.left;
            if (src.top    < dst.top)    dst.top    = src.top;
            if (src.right  > dst.right)  dst.right  = src.right;
            if (src.bottom > dst.bottom) dst.bottom = src.bottom;
        };
        unionR(layer_result.result_rect,     extra->output->result_rect);
        unionR(layer_result.max_result_rect, extra->output->max_result_rect);
        extra->output->solid           = FALSE;
        extra->output->pre_render_data = nullptr;
    }
    return err;
}

// ---------------------------------------------------------------------------
// SmartRender — render
// ---------------------------------------------------------------------------

static PF_Err SmartRender(PF_InData* in_data, PF_OutData* out_data,
                           PF_SmartRenderExtra* extra)
{
    PF_Err err = PF_Err_NONE;

    // Check out the texture layer pixels
    PF_EffectWorld* texture_world = nullptr;
    ERR(extra->cb->checkout_layer_pixels(in_data->effect_ref,
                                          PARAM_TEXTURE,
                                          &texture_world));

    // Check out the output buffer
    PF_EffectWorld* output_world = nullptr;
    ERR(extra->cb->checkout_output(in_data->effect_ref, &output_world));

    if (!err && texture_world && output_world) {
        // Check out all scalar parameters at the current time
        PF_ParamDef params_local[NUM_PARAMS];
        for (int i = 1; i < NUM_PARAMS; ++i) {
            AEFX_CLR_STRUCT(params_local[i]);
        }

        // Build a pointer array that CollectParams can use
        PF_ParamDef* p[NUM_PARAMS] = {};
        for (int i = 1; i < NUM_PARAMS; ++i) p[i] = &params_local[i];

#define CKOUT(idx) ERR(PF_CHECKOUT_PARAM(in_data, idx, \
    in_data->current_time, in_data->time_step, in_data->time_scale, &params_local[idx]))

        CKOUT(PARAM_ASPECT_RATIO);
        CKOUT(PARAM_FILL_AMOUNT);
        CKOUT(PARAM_AMPLITUDE);
        CKOUT(PARAM_FREQUENCY);
        CKOUT(PARAM_SPEED);
        CKOUT(PARAM_COMPLEXITY);
        CKOUT(PARAM_NOISE_AMOUNT);
        CKOUT(PARAM_NOISE_SCALE);
        CKOUT(PARAM_NOISE_SPEED);
        CKOUT(PARAM_LIGHT_ANGLE);
        CKOUT(PARAM_LIGHT_ELEVATION);
        CKOUT(PARAM_LIGHT_INTENSITY);
        CKOUT(PARAM_AMBIENT);
        CKOUT(PARAM_MESH_QUALITY);
#undef CKOUT

        if (!err) {
            FlagParams fp = CollectParams(in_data, p);

            FlagRenderer renderer;
            renderer.Render(output_world, texture_world, fp);
        }

        // Check parameters back in
#define CKIN(idx) PF_CHECKIN_PARAM(in_data, &params_local[idx])
        CKIN(PARAM_ASPECT_RATIO);
        CKIN(PARAM_FILL_AMOUNT);
        CKIN(PARAM_AMPLITUDE);
        CKIN(PARAM_FREQUENCY);
        CKIN(PARAM_SPEED);
        CKIN(PARAM_COMPLEXITY);
        CKIN(PARAM_NOISE_AMOUNT);
        CKIN(PARAM_NOISE_SCALE);
        CKIN(PARAM_NOISE_SPEED);
        CKIN(PARAM_LIGHT_ANGLE);
        CKIN(PARAM_LIGHT_ELEVATION);
        CKIN(PARAM_LIGHT_INTENSITY);
        CKIN(PARAM_AMBIENT);
        CKIN(PARAM_MESH_QUALITY);
#undef CKIN
    }

    if (texture_world)
        ERR(extra->cb->checkin_layer_pixels(in_data->effect_ref, PARAM_TEXTURE));

    return err;
}

// ---------------------------------------------------------------------------
// Plugin entry point
// ---------------------------------------------------------------------------

PF_Err EffectMain(
    PF_Cmd       cmd,
    PF_InData*   in_data,
    PF_OutData*  out_data,
    PF_ParamDef* params[],
    PF_LayerDef* output,
    void*        extra)
{
    PF_Err err = PF_Err_NONE;

    switch (cmd) {
        case PF_Cmd_ABOUT:
            err = About(in_data, out_data);
            break;

        case PF_Cmd_GLOBAL_SETUP:
            err = GlobalSetup(in_data, out_data);
            break;

        case PF_Cmd_PARAMS_SETUP:
            err = ParamsSetup(in_data, out_data, params, output);
            break;

        case PF_Cmd_SMART_PRE_RENDER:
            err = SmartPreRender(in_data, out_data,
                                 reinterpret_cast<PF_PreRenderExtra*>(extra));
            break;

        case PF_Cmd_SMART_RENDER:
            err = SmartRender(in_data, out_data,
                              reinterpret_cast<PF_SmartRenderExtra*>(extra));
            break;

        default:
            break;
    }

    return err;
}

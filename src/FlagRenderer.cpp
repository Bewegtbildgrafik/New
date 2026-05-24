#include "FlagRenderer.h"

#include <cstring>
#include <cstdint>
#include <limits>

// ---------------------------------------------------------------------------
// Loopable 4-D value noise
//
// Time is mapped to a 2-D circle:  (cos(2π·speed·t), sin(2π·speed·t))
// This guarantees the noise returns to its start after exactly 1/speed seconds,
// giving a seamless temporal loop regardless of the spatial parameters.
// ---------------------------------------------------------------------------

static inline float smooth(float t) { return t * t * (3.0f - 2.0f * t); }

static uint32_t hash4i(int32_t x, int32_t y, int32_t z, int32_t w)
{
    uint32_t h = (static_cast<uint32_t>(x) * 1640531527u)
               ^ (static_cast<uint32_t>(y) * 2654435789u)
               ^ (static_cast<uint32_t>(z) * 805459861u)
               ^ (static_cast<uint32_t>(w) * 3266489917u);
    h ^= h >> 16; h *= 0x45d9f3bu; h ^= h >> 16;
    return h;
}

static float hash4f(int x, int y, int z, int w)
{
    return static_cast<float>(hash4i(x, y, z, w) >> 8) * (1.0f / 16777216.0f);
}

// 4-D value noise in [0, 1]
static float noise4D(float x, float y, float z, float w)
{
    const int ix = static_cast<int>(std::floor(x));
    const int iy = static_cast<int>(std::floor(y));
    const int iz = static_cast<int>(std::floor(z));
    const int iw = static_cast<int>(std::floor(w));

    const float fx = x - ix, fy = y - iy, fz = z - iz, fw = w - iw;
    const float ux = smooth(fx), uy = smooth(fy), uz = smooth(fz), uw = smooth(fw);

    float n = 0.0f;
    for (int dw = 0; dw <= 1; ++dw) {
        const float lw = dw ? uw : 1.0f - uw;
        for (int dz = 0; dz <= 1; ++dz) {
            const float lz = dz ? uz : 1.0f - uz;
            for (int dy = 0; dy <= 1; ++dy) {
                const float ly = dy ? uy : 1.0f - uy;
                for (int dx = 0; dx <= 1; ++dx) {
                    const float lx = dx ? ux : 1.0f - ux;
                    n += hash4f(ix+dx, iy+dy, iz+dz, iw+dw) * lx * ly * lz * lw;
                }
            }
        }
    }
    return n;
}

// 3-octave fBm — returns value in approximately [-1, 1] (centred)
static float fbm4D(float x, float y, float z, float w)
{
    float v = 0.0f, amp = 0.5f, total_amp = 0.0f;
    for (int i = 0; i < 3; ++i) {
        v         += noise4D(x, y, z, w) * amp;
        total_amp += amp;
        amp *= 0.5f; x *= 2.0f; y *= 2.0f; z *= 2.0f; w *= 2.0f;
    }
    return (v / total_amp) * 2.0f - 1.0f;   // remap [0,1] → [-1,1]
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static inline A_u_char Clamp255(float v)
{
    return static_cast<A_u_char>(v < 0.0f ? 0 : v > 255.0f ? 255 : v);
}

static inline float Lerpf(float a, float b, float t) { return a + (b - a) * t; }

static inline PF_Pixel8 GetPixel8(PF_EffectWorld* w, int x, int y)
{
    const A_u_char* row = reinterpret_cast<const A_u_char*>(w->data)
                          + y * static_cast<ptrdiff_t>(w->rowbytes);
    const PF_Pixel8* p  = reinterpret_cast<const PF_Pixel8*>(row);
    return p[x];
}

static inline void SetPixel8(PF_EffectWorld* w, int x, int y, PF_Pixel8 c)
{
    A_u_char* row = reinterpret_cast<A_u_char*>(w->data)
                    + y * static_cast<ptrdiff_t>(w->rowbytes);
    PF_Pixel8* p  = reinterpret_cast<PF_Pixel8*>(row);
    p[x] = c;
}

// ---------------------------------------------------------------------------
// Wave function — seamless loop guaranteed because all harmonics are integer
// multiples of the fundamental.  Loop period T = 1/speed seconds.
// ---------------------------------------------------------------------------

float FlagRenderer::WaveZ(float u, float phase_base, float amplitude, int complexity) const
{
    // Attachment at u=0 (pole), maximum sway at u=1 (free end)
    float dz = amplitude * u * std::sin(phase_base);

    if (complexity >= 2)
        dz += amplitude * u * 0.40f * std::sin(2.0f * phase_base + 0.9f);
    if (complexity >= 3)
        dz += amplitude * u * 0.20f * std::sin(3.0f * phase_base + 1.8f);
    if (complexity >= 4)
        dz += amplitude * u * 0.10f * std::sin(4.0f * phase_base + 2.7f);

    return dz;
}

// ---------------------------------------------------------------------------
// Mesh construction
// ---------------------------------------------------------------------------

void FlagRenderer::BuildMesh(const FlagParams& p,
                              float flag_w, float flag_h,
                              int nx, int ny)
{
    const int rows = ny + 1;
    const int cols = nx + 1;
    m_mesh.resize(rows * cols);

    const float amp = p.amplitude;
    const float dU  = 1.0f / static_cast<float>(nx);
    const float dV  = 1.0f / static_cast<float>(ny);

    // Pre-compute circular time coordinates for loopable 4-D noise.
    // The circle has radius 0.5 in noise space; one full revolution per 1/noise_speed seconds.
    const float n_angle = static_cast<float>(2.0 * M_PI) * p.noise_speed * p.time_seconds;
    const float tz = std::cos(n_angle) * 0.5f;
    const float tw = std::sin(n_angle) * 0.5f;

    for (int iy = 0; iy < rows; ++iy) {
        for (int ix = 0; ix < cols; ++ix) {
            const float u = ix * dU;   // 0 = pole, 1 = free end
            const float v = iy * dV;   // 0 = top,  1 = bottom

            // ---- Primary sine wave ----------------------------------------
            const float phase = static_cast<float>(2.0 * M_PI)
                                 * (p.frequency * u - p.speed * p.time_seconds);
            float z = WaveZ(u, phase, amp, p.complexity);

            // ---- Loopable organic noise overlay ----------------------------
            if (p.noise_amount > 0.001f) {
                const float ns = std::max(0.1f, p.noise_scale);

                // Z-noise: modulates the wave amplitude locally — breaks
                // the mechanical uniformity of the sine pattern
                float nz = fbm4D(u * ns, v * ns * 0.6f, tz, tw);
                z += nz * amp * p.noise_amount * u;

                // Y-noise: small vertical perturbation (cross-waves) —
                // the corners and edges move slightly independently
                float ny_val = fbm4D(u * ns * 0.7f + 5.3f, v * ns + 2.1f, tz, tw);
                const float cross_wave = ny_val * flag_h * 0.06f * p.noise_amount * u;

                // Store a temporary Y offset in z for now; applied below
                FlagVertex& vert = m_mesh[iy * cols + ix];
                const float sag = 0.04f * flag_h * u * std::sin(v * static_cast<float>(M_PI));
                vert.world = {(u - 0.5f) * flag_w,
                              -(v - 0.5f) * flag_h - sag + cross_wave,
                              z};
                vert.u = u;
                vert.v = v;
                continue;
            }

            // ---- No noise — pure sine path --------------------------------
            const float sag = 0.04f * flag_h * u * std::sin(v * static_cast<float>(M_PI));
            FlagVertex& vert = m_mesh[iy * cols + ix];
            vert.world = {(u - 0.5f) * flag_w,
                          -(v - 0.5f) * flag_h - sag,
                          z};
            vert.u = u;
            vert.v = v;
        }
    }

    // Compute normals via central finite differences
    for (int iy = 0; iy < rows; ++iy) {
        for (int ix = 0; ix < cols; ++ix) {
            const Vec3& left  = m_mesh[iy * cols + (ix > 0      ? ix-1 : ix)].world;
            const Vec3& right = m_mesh[iy * cols + (ix < nx     ? ix+1 : ix)].world;
            const Vec3& up    = m_mesh[(iy > 0  ? iy-1 : iy) * cols + ix].world;
            const Vec3& down  = m_mesh[(iy < ny ? iy+1 : iy) * cols + ix].world;

            Vec3 du = right - left;
            Vec3 dv = down  - up;

            // Cross product; negate so the normal points toward the camera (+Z)
            Vec3 n = du.cross(dv).normalized() * (-1.0f);
            m_mesh[iy * cols + ix].normal = n;
        }
    }
}

// ---------------------------------------------------------------------------
// Perspective projection
// ---------------------------------------------------------------------------

void FlagRenderer::ProjectMesh(float cx, float cy, float focal_length)
{
    for (FlagVertex& vert : m_mesh) {
        const float d = focal_length - vert.world.z;
        const float safe_d = d > 1.0f ? d : 1.0f;

        vert.pw = focal_length / safe_d;
        vert.sx = vert.world.x * vert.pw + cx;
        vert.sy = -vert.world.y * vert.pw + cy;  // Y axis flipped for screen
    }
}

// ---------------------------------------------------------------------------
// Texture sampling — bilinear interpolation with UV clamping
// ---------------------------------------------------------------------------

PF_Pixel8 FlagRenderer::SampleTextureBilinear(PF_EffectWorld* tex, float u, float v) const
{
    const float x = std::max(0.0f, std::min(1.0f, u)) * (tex->width  - 1);
    const float y = std::max(0.0f, std::min(1.0f, v)) * (tex->height - 1);

    const int x0 = static_cast<int>(x);
    const int y0 = static_cast<int>(y);
    const int x1 = std::min(x0 + 1, tex->width  - 1);
    const int y1 = std::min(y0 + 1, tex->height - 1);

    const float fx = x - x0;
    const float fy = y - y0;

    PF_Pixel8 p00 = GetPixel8(tex, x0, y0);
    PF_Pixel8 p10 = GetPixel8(tex, x1, y0);
    PF_Pixel8 p01 = GetPixel8(tex, x0, y1);
    PF_Pixel8 p11 = GetPixel8(tex, x1, y1);

    PF_Pixel8 result;
    result.alpha = Clamp255(Lerpf(Lerpf(p00.alpha, p10.alpha, fx), Lerpf(p01.alpha, p11.alpha, fx), fy));
    result.red   = Clamp255(Lerpf(Lerpf(p00.red,   p10.red,   fx), Lerpf(p01.red,   p11.red,   fx), fy));
    result.green = Clamp255(Lerpf(Lerpf(p00.green, p10.green, fx), Lerpf(p01.green, p11.green, fx), fy));
    result.blue  = Clamp255(Lerpf(Lerpf(p00.blue,  p10.blue,  fx), Lerpf(p01.blue,  p11.blue,  fx), fy));
    return result;
}

// ---------------------------------------------------------------------------
// Phong lighting
// ---------------------------------------------------------------------------

PF_Pixel8 FlagRenderer::ApplyLighting(PF_Pixel8 texel, const Vec3& normal,
                                       const Vec3& light_dir,
                                       const FlagParams& p) const
{
    // Diffuse (Lambertian)
    const float NdotL   = normal.dot(light_dir);
    const float diffuse = std::max(0.0f, NdotL);

    // Blinn-Phong specular
    const Vec3  view    = Vec3(0, 0, -1);   // camera looks toward -Z in our setup
    const Vec3  halfway = (light_dir + view).normalized();
    const float NdotH   = std::max(0.0f, normal.dot(halfway));
    const float specular = 0.25f * std::pow(NdotH, 48.0f);

    const float factor = std::min(2.0f, p.ambient
                                        + (diffuse * p.light_intensity)
                                        + specular);

    PF_Pixel8 out;
    out.alpha = texel.alpha;
    out.red   = Clamp255(texel.red   * factor);
    out.green = Clamp255(texel.green * factor);
    out.blue  = Clamp255(texel.blue  * factor);
    return out;
}

// ---------------------------------------------------------------------------
// Triangle rasterizer
// ---------------------------------------------------------------------------

void FlagRenderer::RasterizeTriangle(
    PF_EffectWorld*   output,
    PF_EffectWorld*   texture,
    const FlagVertex& v0,
    const FlagVertex& v1,
    const FlagVertex& v2,
    const Vec3&       light_dir,
    const FlagParams& p,
    bool              backface)
{
    // Bounding box clipped to output
    int minX = static_cast<int>(std::max(0.0f,   std::min({v0.sx, v1.sx, v2.sx})));
    int maxX = static_cast<int>(std::min(static_cast<float>(output->width  - 1),
                                         std::max({v0.sx, v1.sx, v2.sx})));
    int minY = static_cast<int>(std::max(0.0f,   std::min({v0.sy, v1.sy, v2.sy})));
    int maxY = static_cast<int>(std::min(static_cast<float>(output->height - 1),
                                         std::max({v0.sy, v1.sy, v2.sy})));

    if (minX > maxX || minY > maxY) return;

    // Signed area (determines winding)
    const float ax  = v1.sx - v0.sx, ay = v1.sy - v0.sy;
    const float bx  = v2.sx - v0.sx, by = v2.sy - v0.sy;
    const float area = ax * by - ay * bx;
    if (std::abs(area) < 0.5f) return;   // degenerate

    const float inv_area = 1.0f / area;

    for (int py = minY; py <= maxY; ++py) {
        for (int px = minX; px <= maxX; ++px) {
            const float cx = px + 0.5f - v0.sx;
            const float cy = py + 0.5f - v0.sy;

            // Barycentric weights
            const float w1 = (cx * by - cy * bx) * inv_area;
            const float w2 = (ax * cy - ay * cx) * inv_area;
            const float w0 = 1.0f - w1 - w2;

            if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f) continue;

            // Perspective-correct attribute interpolation
            const float pw = w0 * v0.pw + w1 * v1.pw + w2 * v2.pw;
            if (pw < 1e-6f) continue;

            const float u = (w0 * v0.pw * v0.u + w1 * v1.pw * v1.u + w2 * v2.pw * v2.u) / pw;
            const float v = (w0 * v0.pw * v0.v + w1 * v1.pw * v1.v + w2 * v2.pw * v2.v) / pw;

            Vec3 normal = (v0.normal * (w0 * v0.pw) +
                           v1.normal * (w1 * v1.pw) +
                           v2.normal * (w2 * v2.pw)) * (1.0f / pw);
            normal = normal.normalized();

            // Flip normal for the back face so lighting still looks good
            if (backface) normal = normal * (-1.0f);

            PF_Pixel8 texel = SampleTextureBilinear(texture, u, v);
            if (texel.alpha == 0) continue;

            // Darken the back slightly so front vs back is distinguishable
            FlagParams lp = p;
            if (backface) lp.light_intensity *= 0.4f;

            SetPixel8(output, px, py, ApplyLighting(texel, normal, light_dir, lp));
        }
    }
}

// ---------------------------------------------------------------------------
// Quad → two triangles, back-face detection per triangle
// ---------------------------------------------------------------------------

void FlagRenderer::RasterizeQuad(
    PF_EffectWorld* output,
    PF_EffectWorld* texture,
    int qi, int qj, int nx,
    const Vec3& light_dir,
    const FlagParams& p)
{
    const int cols = nx + 1;
    const FlagVertex& tl = m_mesh[ qj      * cols + qi    ];
    const FlagVertex& tr = m_mesh[ qj      * cols + qi + 1];
    const FlagVertex& bl = m_mesh[(qj + 1) * cols + qi    ];
    const FlagVertex& br = m_mesh[(qj + 1) * cols + qi + 1];

    // Split quad into two triangles; render both front and back faces
    auto renderTri = [&](const FlagVertex& a, const FlagVertex& b, const FlagVertex& c)
    {
        float ax = b.sx - a.sx, ay = b.sy - a.sy;
        float bx = c.sx - a.sx, by = c.sy - a.sy;
        bool back = (ax * by - ay * bx) < 0.0f;
        RasterizeTriangle(output, texture, a, b, c, light_dir, p, back);
    };

    renderTri(tl, tr, bl);
    renderTri(tr, br, bl);
}

// ---------------------------------------------------------------------------
// Main render entry point
// ---------------------------------------------------------------------------

void FlagRenderer::Render(
    PF_EffectWorld*   output,
    PF_EffectWorld*   texture,
    const FlagParams& p)
{
    if (!output || !texture) return;

    // Clear output to fully transparent
    for (int y = 0; y < output->height; ++y) {
        A_u_char* row = reinterpret_cast<A_u_char*>(output->data)
                        + y * static_cast<ptrdiff_t>(output->rowbytes);
        std::memset(row, 0, output->width * sizeof(PF_Pixel8));
    }

    // Determine flag dimensions in pixels
    const float out_w = static_cast<float>(output->width);
    const float out_h = static_cast<float>(output->height);

    // Fit the flag inside the output frame respecting aspect ratio
    const float fill = std::max(0.1f, std::min(1.0f, p.fill_amount));
    float flag_w, flag_h;
    if (p.aspect_ratio >= 1.0f) {
        flag_w = out_w * fill;
        flag_h = flag_w / p.aspect_ratio;
        if (flag_h > out_h * fill) { flag_h = out_h * fill; flag_w = flag_h * p.aspect_ratio; }
    } else {
        flag_h = out_h * fill;
        flag_w = flag_h * p.aspect_ratio;
        if (flag_w > out_w * fill) { flag_w = out_w * fill; flag_h = flag_w / p.aspect_ratio; }
    }

    // Mesh grid resolution: long axis = mesh_quality, short axis proportional
    const int nx = std::max(4, p.mesh_quality);
    const int ny = std::max(4, static_cast<int>(nx / std::max(0.5f, p.aspect_ratio)));

    BuildMesh(p, flag_w, flag_h, nx, ny);

    // Focal length: gives a natural perspective — roughly equivalent to 70 mm
    const float focal = out_w * 0.9f;
    const float cx    = out_w * 0.5f;
    const float cy    = out_h * 0.5f;

    ProjectMesh(cx, cy, focal);

    // Light direction vector from azimuth and elevation
    const float az  = static_cast<float>(p.light_angle     * M_PI / 180.0);
    const float el  = static_cast<float>(p.light_elevation  * M_PI / 180.0);
    Vec3 light_dir  = Vec3(std::cos(el) * std::sin(az),
                           std::sin(el),
                           -std::cos(el) * std::cos(az)).normalized();

    // Rasterize all quads
    for (int qj = 0; qj < ny; ++qj)
        for (int qi = 0; qi < nx; ++qi)
            RasterizeQuad(output, texture, qi, qj, nx, light_dir, p);
}

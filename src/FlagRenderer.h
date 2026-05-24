#pragma once

#include "AE_Effect.h"

#include <cmath>
#include <vector>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------------------------------------------------------------------
// Math primitives
// ---------------------------------------------------------------------------

struct Vec3 {
    float x, y, z;

    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s)       const { return {x * s,   y * s,   z * s}; }

    float dot(const Vec3& o)  const { return x*o.x + y*o.y + z*o.z; }
    Vec3  cross(const Vec3& o) const {
        return {y*o.z - z*o.y, z*o.x - x*o.z, x*o.y - y*o.x};
    }
    float length() const { return std::sqrt(x*x + y*y + z*z); }
    Vec3  normalized() const {
        float l = length();
        return l > 1e-6f ? *this * (1.0f / l) : Vec3(0, 0, 1);
    }
};

inline Vec3 operator*(float s, const Vec3& v) { return v * s; }

// ---------------------------------------------------------------------------
// Per-vertex data stored in the mesh
// ---------------------------------------------------------------------------

struct FlagVertex {
    Vec3  world;   // 3-D world position
    Vec3  normal;  // outward surface normal
    float u, v;    // texture coordinates  [0,1]
    float sx, sy;  // projected screen position
    float pw;      // perspective weight (focal / (focal - z))
};

// ---------------------------------------------------------------------------
// All parameters needed for one render call
// ---------------------------------------------------------------------------

struct FlagParams {
    float time_seconds;     // current playback time → drives the wave phase

    // Geometry
    float aspect_ratio;     // flag width / flag height  (default 1.5 for 3:2)
    float fill_amount;      // fraction of the shorter output dimension used (0.1-1.0)

    // Wave
    float amplitude;        // displacement in pixels at the free end
    float frequency;        // number of full wave cycles across the flag width
    float speed;            // wave cycles per second → loop period = 1/speed
    int   complexity;       // number of harmonic modes: 1-4

    // Lighting
    float light_angle;      // horizontal light direction in degrees (0 = front-left)
    float light_elevation;  // elevation above horizon in degrees (0-90)
    float light_intensity;  // diffuse+specular strength  (0-2)
    float ambient;          // minimum brightness factor  (0-1)

    // Quality
    int   mesh_quality;     // number of quads along the long axis (16/32/64/128)
};

// ---------------------------------------------------------------------------
// The renderer — create one per render call (no shared state)
// ---------------------------------------------------------------------------

class FlagRenderer {
public:
    void Render(
        PF_EffectWorld*   output,
        PF_EffectWorld*   texture,
        const FlagParams& p);

private:
    // Build the NX×NY vertex grid in world space and compute normals
    void BuildMesh(const FlagParams& p, float flag_w, float flag_h, int nx, int ny);

    // Project every vertex from world space to screen space
    void ProjectMesh(float cx, float cy, float focal_length);

    // Rasterize one quad (two triangles)
    void RasterizeQuad(
        PF_EffectWorld* output,
        PF_EffectWorld* texture,
        int qi, int qj, int nx,
        const Vec3& light_dir,
        const FlagParams& p);

    void RasterizeTriangle(
        PF_EffectWorld*   output,
        PF_EffectWorld*   texture,
        const FlagVertex& v0,
        const FlagVertex& v1,
        const FlagVertex& v2,
        const Vec3&       light_dir,
        const FlagParams& p,
        bool              backface);

    PF_Pixel8 SampleTextureBilinear(PF_EffectWorld* tex, float u, float v) const;
    PF_Pixel8 ApplyLighting(PF_Pixel8 texel, const Vec3& normal,
                            const Vec3& light_dir, const FlagParams& p) const;

    // Wave displacement Z for a vertex at horizontal position u in [0,1]
    float WaveZ(float u, float phase_base, float amplitude, int complexity) const;

    std::vector<FlagVertex> m_mesh;
};

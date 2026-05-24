# AE 3D Flag — After Effects native plug-in

Realistic, seamlessly looping 3-D flag simulation for Adobe After Effects (macOS).

---

## Features

| Feature | Detail |
|---|---|
| **3-D mesh** | Software-rendered perspective-correct triangle mesh |
| **Texture** | Any AE layer — footage, pre-comp, solids |
| **Seamless loop** | Wave function uses integer harmonics → perfect loop at any speed |
| **Dimensions** | Aspect ratio + frame fill fraction |
| **Wave controls** | Amplitude · Frequency · Speed · Complexity (1–4 harmonics) |
| **Lighting** | Diffuse + Blinn-Phong specular, configurable azimuth + elevation |
| **Mesh quality** | Low 16 / Medium 32 / High 64 / Ultra 128 quads on long axis |
| **Platform** | macOS universal (arm64 + x86_64) |

---

## Requirements

| Tool | Version |
|---|---|
| macOS | 11 Big Sur or later |
| Xcode | 14 or later (Command Line Tools sufficient) |
| CMake | 3.21 or later |
| Adobe After Effects SDK | CC 2019 or later |
| After Effects | CC 2019 or later |

---

## Getting the AE SDK

1. Sign up for an [Adobe Developer account](https://developer.adobe.com/).
2. Download the **After Effects SDK** from the developer console.
3. Unzip it — the root folder contains `Headers/`, `Resources/`, `Examples/`.

---

## Build instructions

```bash
# Clone the repository
git clone https://github.com/bewegtbildgrafik/new.git
cd new

# Create a build directory
mkdir build && cd build

# Configure — replace the path with your actual AE SDK location
cmake .. \
  -G Xcode \
  -DAE_SDK_DIR="/path/to/AfterEffectsSDK" \
  -DAE_PLUGIN_DIR="/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore"

# Build (Release)
cmake --build . --config Release

# Install to AE plug-ins folder (optional, path set above)
cmake --install . --config Release
```

### Tip — fast rebuild without Xcode

```bash
cmake --build . --config Release -- -jobs $(sysctl -n hw.logicalcpu)
```

---

## Installation (manual)

Copy `AE3DFlag.aex` (or the `AE3DFlag.aex` bundle folder) to:

```
/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore/
```

or the user-level folder:

```
~/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore/
```

Restart After Effects.

---

## Usage in After Effects

1. Create a new composition.
2. Import your flag image / video → drag it into the comp (can be hidden/shy).
3. Create a new **Solid** layer the size of the comp.
4. Apply **Effect → Bewegtbildgrafik → 3D Flag** to the Solid.
5. In the Effect Controls panel, set **Flag Texture** to the footage layer.
6. Adjust **Dimensions**, **Wave**, and **Lighting** parameters.
7. Enable **Motion Blur** on the solid for extra realism.

### Seamless loop

The wave period is `1 / Speed` seconds. Set your composition duration to any
multiple of that value and the animation will loop perfectly.

Example: Speed = 0.8 cycles/s → period = 1.25 s → a 5-second comp loops 4 times.

---

## Parameter reference

### Dimensions
| Parameter | Range | Default | Description |
|---|---|---|---|
| Aspect Ratio (W/H) | 0.25 – 4.0 | 1.5 | Flag width ÷ height |
| Fill Frame | 0.1 – 1.0 | 0.85 | Fraction of the frame the flag occupies |

### Wave
| Parameter | Range | Default | Description |
|---|---|---|---|
| Amplitude | 0 – 400 px | 60 | Max Z-displacement at the free end |
| Frequency | 0.25 – 8.0 | 2.0 | Wave cycles across the flag width |
| Speed (cycles/s) | 0.05 – 8.0 | 0.8 | How fast the wave travels |
| Wave Complexity | 1 – 4 modes | 2 | More harmonics → more natural look |

### Lighting
| Parameter | Range | Default | Description |
|---|---|---|---|
| Light Azimuth | –180° – 180° | 30° | Horizontal rotation of the light |
| Light Elevation | 0° – 90° | 45° | Vertical elevation above the flag plane |
| Intensity | 0 – 2 | 1.0 | Diffuse + specular strength |
| Ambient | 0 – 1 | 0.25 | Minimum brightness (shadow fill) |

### Render
| Parameter | Values | Default | Description |
|---|---|---|---|
| Mesh Quality | Low / Medium / High / Ultra | Medium | Triangle count (16/32/64/128 quads on long axis) |

---

## Architecture

```
src/
  AE3DFlag.h          Plugin constants and parameter IDs
  AE3DFlag.cpp        AE entry point, parameter registration, SmartRender
  FlagRenderer.h      Math structs, FlagParams, FlagRenderer interface
  FlagRenderer.cpp    Mesh build · projection · rasterizer · lighting
res/
  AE3DFlag.r          PiPL resource (Rez source)
  Info.plist          macOS bundle metadata
CMakeLists.txt        Build configuration
```

### Wave algorithm

All harmonics use **integer multiples** of the fundamental frequency, which
guarantees a perfect seamless loop at period `T = 1 / speed`:

```
phase(u, t) = 2π × (frequency × u  −  speed × t)

Z(u, t) = amplitude × u × [
    sin(1 × phase)           +
    0.40 × sin(2 × phase + 0.9)   +   (complexity ≥ 2)
    0.20 × sin(3 × phase + 1.8)   +   (complexity ≥ 3)
    0.10 × sin(4 × phase + 2.7)       (complexity ≥ 4)
]
```

The phase offsets break the left–right symmetry without affecting loopability.

### Rendering pipeline

1. **BuildMesh** — compute `(nx+1) × (ny+1)` vertex positions with wave + gravity sag;
   normals via central finite differences.
2. **ProjectMesh** — standard perspective divide; focal length ≈ 0.9 × output width.
3. **RasterizeQuad** — each mesh quad → 2 triangles; barycentric rasterization with
   perspective-correct UV interpolation.
4. **SampleTextureBilinear** — bilinear tap on the input layer.
5. **ApplyLighting** — Lambertian diffuse + Blinn-Phong specular.

---

## License

© 2025 Bewegtbildgrafik · All rights reserved.

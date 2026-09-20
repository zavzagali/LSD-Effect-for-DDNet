
# 🍃 DDNet / Teeworlds LSD Effect

**A simple LSD screen effect component with hue-cycling and screen wobble for DDNet clients.**
<img src="https://media.tenor.com/Ylqui-QhuO0AAAAC/pikachu-drool.gif" width="1000">

<div align="center">
  <a href="https://github.com/reiayanami0/LSD-Effect-for-DDNet">
    <img src="https://img.shields.io/badge/For-Teeworlds-98d243?style=for-the-badge" alt="For Teeworlds">
  </a>

  <br/>

  <img src="https://img.shields.io/badge/C++-00599C?style=flat-square&logo=c%2B%2B&logoColor=white" alt="C++">
  <img src="https://img.shields.io/badge/Compatible%20with-DDNet%2020.0-lightgrey?style=flat-square" alt="Compatible with DDNet 20.0">
</div>

## Implementation (DDNet 20.0)

### 1. Drop the folder in

Copy this whole folder to:

    src/game/client/components/lsd/

### 2. Root CMakeLists.txt

Add near where other subdirectories/components are configured:

```cmake
components/lsd/lsd_effect.cpp
components/lsd/lsd_effect.h
```

### 3. game/client/gameclient.h

Add the include near other component includes:

```cpp
#include "components/lsd/lsd_effect.h"
```

Add the member in the `CGameClient` class, next to other components (e.g. after `m_Effects`):

```cpp
CLsdEffect m_LsdEffect;
```

> **Note:** Do NOT place it in `CGameInfo`. It must be in `CGameClient` so that `GameClient()->m_LsdEffect` works.

### 4. game/client/gameclient.cpp

Add near where other components are pushed (e.g. after `&m_Camera`):

```cpp
m_vpAll.push_back(&m_LsdEffect);  // end of wherever &m_Camera etc. are pushed
```

### 5. game/client/components/camera.cpp

At the end of `OnRender()`, just before the closing brace (after `m_WasSpectating = ...`):

```cpp
m_Zoom *= GameClient()->m_LsdEffect.ZoomModifier();
```

Add a new function:

```cpp
float CCamera::EffectiveZoom() const
{
    return m_Zoom * GameClient()->m_LsdEffect.ZoomModifier();
}
```

### 6. game/client/components/camera.h

Add in the `public:` section of `CCamera`:

```cpp
float EffectiveZoom() const;
```

## Build and test

```bash
cmake -Bbuild -GNinja && cmake --build build
```

## Usage

```
lsd_toggle              # turn the effect on/off
lsd_speed 1.0           # faster hue cycling
lsd_intensity 0.15      # stronger tint
lsd_wobble 0.08         # screen zoom breathing
lsd_wobble_speed 0.02   # screen zoom breathing speed
lsd_breathe 0.2         # effect zoom breathing
```

Bind a key for convenience: `bind l lsd_toggle`

# Axion (formerly Client-Mod) [![Build Status](https://github.com/Elinsrc/Axion/actions/workflows/build.yml/badge.svg)](https://github.com/Elinsrc/Axion/actions) <img align="right" src="https://raw.githubusercontent.com/Elinsrc/Axion/refs/heads/master/android/app/src/main/res/mipmap-xxxhdpi/ic_launcher.png" alt="Axion" />

 - **Axion** is a modification of the **Half-Life** client for **Xash3D FWGS** and **GoldSource**, based on [hlsdk-portable](https://github.com/FWGS/hlsdk-portable) and [PrimeXT](https://github.com/SNMetamorph/PrimeXT).

## Configuration (CVars)
| CVar | Default | Min | Max | Description |
|------|--------|-----|-----|-------------|
| cl_autojump | 1 | 0 | 1 | Automatically jumps when landing while holding +jump |
| +ducktap | - | - | - | Performs a duck tap on landing (1-frame +duck) |
| hud_color | "255 160 0" | - | - | Sets HUD color (RGB) |
| hud_weapon | 0 | 0 | 1 | Displays current weapon in HUD |
| vis_reload | 1 | 0 | 1 | Highlights weapon while reloading |
| vis_reload_color | "250 250 250" | - | - | Color used for reload highlight |
| cl_logchat | 0 | 0 | 2 | Chat logging mode (0 off, 1 players, 2 system + players) |
| cl_chatsound | 1 | 0 | 2 | Chat sound mode (0 off, 1 all, 2 players only) |
| cl_chatsound_path | "misc/talk.wav" | - | - | Path to chat notification sound |
| hud_vis | 0 | 0 | 1 | Enables dynamic health and armor coloring |
| vis_battery100 | "0 250 0" | - | - | Battery indicator color at 100% |
| vis_battery80 | "0 250 0" | - | - | Battery indicator color at 80% |
| vis_battery60 | "250 250 0" | - | - | Battery indicator color at 60% |
| vis_battery40 | "250 100 0" | - | - | Battery indicator color at 40% |
| vis_battery20 | "250 0 0" | - | - | Battery indicator color at 20% |
| vis_health100 | "0 250 0" | - | - | Health indicator color at 100% |
| vis_health80 | "0 250 0" | - | - | Health indicator color at 80% |
| vis_health60 | "250 250 0" | - | - | Health indicator color at 60% |
| vis_health40 | "250 100 0" | - | - | Health indicator color at 40% |
| vis_health20 | "250 0 0" | - | - | Health indicator color at 20% |
| cl_weaponlowering | 1 | 0 | 1 | Lowers weapon based on movement speed |
| cl_weaponsway | 1 | 0 | 1 | Adds weapon sway while moving |
| cl_weaponlag | 1 | 0 | 1 | Adds weapon lag when turning |
| hud_speedometer | 0 | 0 | 1 | Displays player movement speed |
| hud_speedometer_below_cross | 0 | 0 | 1 | Positions speedometer below crosshair |
| hud_jumpspeed | 0 | 0 | 1 | Displays speed at jump moment |
| hud_jumpspeed_below_cross | 0 | 0 | 1 | Positions jumpspeed below crosshair |
| hud_jumpspeed_height | 0 | - | - | Adjusts jumpspeed vertical position |
| hud_strafeguide | 0 | 0 | 1 | Displays strafing guide |
| hud_strafeguide_zoom | 1 | - | - | Strafeguide zoom level |
| hud_strafeguide_height | 0 | - | - | Strafeguide vertical position |
| hud_strafeguide_size | 0 | - | - | Strafeguide size |
| hud_watermark | 1 | - | - | Displays client watermark |
| hud_rainbow | 0 | 0 | 1 | Enables rainbow HUD effect |
| hud_rainbow_sat | 100 | - | - | Rainbow saturation |
| hud_rainbow_val | 100 | - | - | Rainbow brightness |
| hud_rainbow_speed | 40 | - | - | Rainbow animation speed |
| hud_rainbow_xphase | 0.4 | - | - | Rainbow X phase offset |
| hud_rainbow_yphase | 0.7 | - | - | Rainbow Y phase offset |
| hud_deathnotice_bg | 1 | 0 | 1 | Enables death notice background |
| cl_gauss_balls | 1 | 0 | 1 | Enables Gauss impact particles |
| cl_gauss_hits | 1 | 0 | 1 | Enables Gauss impact sprite |
| cl_hidecorpses | 0 | 0 | 1 | Hides player corpses |
| cl_killsound | 1 | - | - | Plays sound when killing a player |
| cl_killsound_path | "buttons/bell1.wav" | - | - | Path to kill sound |
| cl_cross | 0 | 0 | 1 | Enables custom crosshair |
| cl_cross_color | "0 255 0" | - | - | Crosshair color (RGB) |
| cl_cross_alpha | 255 | - | - | Crosshair transparency |
| cl_cross_size | 10 | - | - | Crosshair size |
| cl_cross_gap | 3 | - | - | Crosshair gap from center |
| cl_cross_thickness | 2 | - | - | Crosshair line thickness |
| cl_cross_outline | 1 | 0 | 1 | Enables crosshair outline |
| cl_cross_top_line | 1 | 0 | 1 | Enables top crosshair line |
| cl_cross_bottom_line | 1 | 0 | 1 | Enables bottom crosshair line |
| cl_cross_left_line | 1 | 0 | 1 | Enables left crosshair line |
| cl_cross_right_line | 1 | 0 | 1 | Enables right crosshair line |
| cl_cross_dot_size | 1 | - | - | Crosshair center dot size (0 disables) |
| cl_cross_dot_color | "0 255 0" | - | - | Crosshair dot color |
| cl_cross_circle_radius | 0 | - | - | Crosshair circle radius (ImGui only) |
| cl_cross_circle_gap | 3 | - | - | Crosshair circle gap (ImGui only) |
| cl_cross_circle_color | "0 255 0" | - | - | Crosshair circle color (ImGui only) |
| cl_viewmodel_ofs_right | 0 | - | - | Viewmodel right offset |
| cl_viewmodel_ofs_forward | 0 | - | - | Viewmodel forward offset |
| cl_viewmodel_ofs_up | 0 | - | - | Viewmodel vertical offset |
| cl_debug | 0 | 0 | 3 | Debug information level |
| cl_debug_showfps | 1 | 0 | 1 | Shows FPS counter |
| hud_health_divider | 1 | 0 | 1 | Shows health separator bar |
| hud_allow_hd | 1 | 0 | 1 | Enables HL25-style HUD |
| hud_timer | 1 | 0 | 3 | HUD timer mode (0 off / 1 time left / 2 time passed / 3 clock) |
| hud_nextmap | 1 | 0 | 1 | Displays next map info |
| ui_imgui_demo | - | - | - | Opens ImGui demo window |
| ui_commands | - | - | - | Opens Axion settings menu |
| ui_imgui_scale | 0 | 1 | * | UI scaling factor |

## Building
Clone the source code:
```
git clone https://github.com/Elinsrc/Axion --recursive
```
### Windows
```
cmake -A Win32 -S . -B build
cmake --build build --config Release
```
### Linux
```
cmake -S . -B build
cmake --build build --config Release
```
### Android
```
./gradlew assembleDebug
```

## Thanks
 - Thanks To Valve for [Half-Life](https://store.steampowered.com/app/70/HalfLife/) and [hlsdk](https://github.com/ValveSoftware/halflife)
 - Thanks To [FWGS](https://github.com/FWGS) for [hlsdk-portable](https://github.com/FWGS/hlsdk-portable)
 - Thanks To [YaLTeR](https://github.com/YaLTeR) for [OpenAG](https://github.com/YaLTeR/OpenAG)
 - Thanks To [Velaron](https://github.com/Velaron) for [AndroidLauncher](https://github.com/Elinsrc/Axion/tree/master/android)
 - Thanks To [SNMetamorph](https://github.com/SNMetamorph) for [BuildInfo](https://github.com/Elinsrc/Axion/blob/master/public/build_info.h), [goldsrc-monitor](https://github.com/SNMetamorph/goldsrc-monitor) and [imgui_manager](https://github.com/SNMetamorph/PrimeXT/tree/master/client/ui)

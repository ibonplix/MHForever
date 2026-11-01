#pragma once

// OpenGL 3.3 drawing for the GE (docs/05-runtime-hle.md §6, "hardware renderer").
//
// Runs on the GE thread: it owns the OpenGL context of the game window, draws every framebuffer the game renders
// into an upscaled render target, and presents the displayed one to the window.

#include "backend.hpp"

#include "psprecomp/guest_memory.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct SDL_Window;

namespace mhf::psp {

// An image drawn over the game (system dialogs), handed from the emulation thread to the GE thread. Premultiplied
// RGBA covering the letterboxed game area; `generation` changes whenever the image does.
struct OverlayImage {
    std::mutex mutex;
    std::vector<std::uint32_t> pixels;
    int width = 0, height = 0;
    bool visible = false;
    // The image is the in-game menu: the game runs on behind it, blurred, and the backend draws it at least as often as the
    // display refreshes, also when the game shows nothing new.
    bool continuous = false;
    std::uint64_t generation = 0;
};

// What the player can change while the game runs (read by the GE thread).
struct LiveSettings {
    std::atomic<bool> smooth_textures{false};
    // Requests for the drawing thread, taken at the start of the next picture (0 = none): the internal resolution
    // multiplier (1..10) and the output rate in fps (a multiple of 30 up to 180).
    std::atomic<int> scale{0};
    std::atomic<int> fps{0};
    // Texture pack (frontend/texture_pack.hpp): drawn or not while the game runs; and, set before the backend is made,
    // the pack's folder (empty: none, or MHF_TEXTURE_PACK) and the release it is drawn for ("ULES01213").
    std::atomic<bool> texture_pack{true};
    std::string texture_pack_folder, texture_pack_game;
    // Screen-space ambient occlusion (enhancement): on or off while the game runs, its strength (about 0 to 10) and its
    // radius in PSP pixels. MHF_SSAO=1, MHF_SSAO_STRENGTH and MHF_SSAO_RADIUS set the values the game starts with.
    std::atomic<bool> ssao{false};
    std::atomic<float> ssao_strength{10.0f};
    std::atomic<float> ssao_radius{10.0f};
    // Beyond this distance from the camera (in the game's own units; 0 = no limit) the occlusion fades out, so far layers
    // such as the clouds are left alone.
    std::atomic<float> ssao_distance{45000.0f};
    // Surfaces almost flat to each other do not occlude (bias, higher: fewer lines on flat walls), and how far in front of
    // an occluder it still counts, as a fraction of its distance to the camera (range, lower: less reach).
    std::atomic<float> ssao_bias{0.36f};
    std::atomic<float> ssao_range{0.15f};
    // Steps lower than this fraction of their distance to the camera do not occlude (thin ledges, ground tiles and decals
    // lying a little above the terrain).
    std::atomic<float> ssao_thin{0.025f};
};

// Creates the context on the calling thread. Returns nullptr (after logging why) when OpenGL 3.3 is unavailable.
// `scale` is the internal resolution multiplier (1 = the original 480x272). `smooth_textures` (enhancement) forces
// bilinear filtering on 3D textures. `target_fps` (enhancement) is the output rate: a whole multiple of the game's 30 fps up to 180 (rounded), with
// in-between pictures drawn for every flip; 30 shows the game as it is.
// `live` (optional) is read every draw and starts with the caller's value; without it `smooth_textures` is fixed.
std::unique_ptr<gpu::Backend> make_gl_backend(SDL_Window *window, int scale, psprecomp::GuestMemory &memory,
                                              bool smooth_textures = false, OverlayImage *overlay = nullptr,
                                              int target_fps = 30, LiveSettings *live = nullptr);

} // namespace mhf::psp

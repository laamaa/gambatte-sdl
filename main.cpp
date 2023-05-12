#include "SDL_audio.h"
#include "SDL_events.h"
#include "SDL_keycode.h"
#include "SDL_log.h"
#include "SDL_pixels.h"
#include "SDL_render.h"
#include "SDL_stdinc.h"
#include "SDL_video.h"
#include "gambatte.h"
#include "gbint.h"

#include <SDL.h>
#include <cstddef>
#include <stdlib.h>
#include <vector>
#include <unistd.h>

using namespace gambatte;

SDL_Window *win;
SDL_Renderer *rend;
SDL_Texture *maintexture;
SDL_Texture *target_texture;

const int texture_width = 160;
const int texture_height = 144;

gambatte::uint_least32_t *videoBuf =
    new uint32_t[texture_width * texture_height];
uint32_t *audiobuffer;
std::size_t samples = 512;
std::ptrdiff_t pitch = 160;

SDL_AudioSpec want_out, have_out;

uint32_t *framebuffer;

gambatte::GB gb_;

static SDL_AudioDeviceID devid_out = 0;

int render_sdl() {
  SDL_SetRenderTarget(rend, NULL);
  SDL_SetRenderDrawColor(rend, 0, 0, 0, 0);
  SDL_RenderClear(rend);
  SDL_RenderCopy(rend, maintexture, NULL, NULL);
  SDL_RenderPresent(rend);
  return 0;
}

void audio_callback(void *userdata, Uint8 *stream, int len) {
  uint *bytes = nullptr;
  int maintexture_pitch = 0;
  std::ptrdiff_t smps;
  SDL_zero(stream);

  smps = gb_.runFor(videoBuf, pitch, audiobuffer, samples);
  if (smps != -1) {
  }
}

int audio_init(int audio_buffer_size, const char *output_device_name) {

  SDL_Log("Initialize audio");

  // Open output device first to avoid possible Directsound errors
  SDL_zero(want_out);
  want_out.freq = 44100;
  want_out.format = AUDIO_S16;
  want_out.channels = 2;
  want_out.samples = audio_buffer_size;
  devid_out = SDL_OpenAudioDevice(output_device_name, 0, &want_out, &have_out,
                                  SDL_AUDIO_ALLOW_ANY_CHANGE);
  SDL_Log("Opened device %s", SDL_GetAudioDeviceName(0, 0));
  if (devid_out == 0) {
    SDL_Log("Failed to open output: %s", SDL_GetError());
    return 0;
  }

  // Start audio processing

  SDL_PauseAudioDevice(devid_out, 0);

  return 1;
}

void audio_destroy() {
  SDL_Log("Closing audio devices");
  SDL_PauseAudioDevice(devid_out, 1);
  SDL_CloseAudioDevice(devid_out);
}

int initialize_sdl() {
  const int window_width = 640;  // SDL window width
  const int window_height = 480; // SDL window height

  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL_Init: %s\n", SDL_GetError());
    return -1;
  }

  // SDL documentation recommends this
  atexit(SDL_Quit);

  win = SDL_CreateWindow("worlds no1 bestest emulator", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, window_width, window_height,
                         SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL |
                             SDL_WINDOW_RESIZABLE);

  rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

  SDL_RenderSetLogicalSize(rend, texture_width, texture_height);

  // Initialize an SDL texture and a raw framebuffer
  maintexture = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGB888,
                                  SDL_TEXTUREACCESS_STREAMING, texture_width,
                                  texture_height);

  audiobuffer = new gambatte::uint_least32_t[1024 * 10];

  // clear the main texture
  SDL_SetRenderTarget(rend, maintexture);
  SDL_SetRenderDrawColor(rend, 0, 0, 0, 1);
  SDL_RenderClear(rend);

  // Initialize audio using system default output
  audio_init(512, NULL);

  SDL_LogSetAllPriority(SDL_LOG_PRIORITY_DEBUG);
  return 0;
}

int destroy_sdl() {
  audio_destroy();
  SDL_DestroyTexture(maintexture);
  SDL_DestroyRenderer(rend);
  SDL_DestroyWindow(win);
  SDL_Quit();
  return 0;
}

std::ptrdiff_t run(uint_least32_t *pixels, std::ptrdiff_t pitch,
                   uint_least32_t *soundBuf, std::size_t &samples) {
  std::size_t targetSamples = samples;
  std::size_t actualSamples = 0;
  std::ptrdiff_t vidFrameSampleNo = -1;

  while (actualSamples < targetSamples && vidFrameSampleNo < 0) {
    samples = targetSamples - actualSamples;
    std::ptrdiff_t const vfsn =
        gb_.runFor(pixels, pitch, soundBuf + actualSamples, samples);

    if (vfsn >= 0)
      vidFrameSampleNo = actualSamples + vfsn;

    actualSamples += samples;
  }
  samples = actualSamples;
  return vidFrameSampleNo;
}

int main(int argc, char *argv[]) {

  SDL_Event e;
  int err = 0;

  err = initialize_sdl();

  if (err != 0) {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Could not initialize SDL.");
    destroy_sdl();
    return 1;
  }

  err = gb_.loadBios("gbc_bios.bin", 0, 0);
  if (err != 0) {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Could not load BIOS");
    destroy_sdl();
    return 1;
  }

  err = gb_.load("lsdj.gb", GB::CGB_MODE);
  if (err != 0) {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Could not load ROM");
    destroy_sdl();
    return 1;
  }

  for (;;) {

    uint *bytes = nullptr;
    int maintexture_pitch = 0;
    std::ptrdiff_t smps;

    while (SDL_PollEvent(&e) != 0) {
      switch (e.type) {
      case SDL_QUIT:
        destroy_sdl();
        return 0;
      }
    }

    smps = run(videoBuf, pitch, audiobuffer, samples);
    SDL_QueueAudio(devid_out, audiobuffer, samples);
    if (smps >= 0) {
      
      SDL_LockTexture(maintexture, nullptr, reinterpret_cast<void **>(&bytes),
                      &maintexture_pitch);
      SDL_memcpy(bytes, videoBuf,
                 texture_width * texture_height * sizeof(uint32_t));
      SDL_UnlockTexture(maintexture);
      render_sdl();
      const int ft = (16743ul - 16743 / 1024);
      usleep(ft);
    }
  }

  // run(bytes, pitch, audiobuffer, samples);

  destroy_sdl();

  return 0;
}
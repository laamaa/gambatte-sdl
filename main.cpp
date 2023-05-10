#include "SDL_events.h"
#include "SDL_keycode.h"
#include "SDL_log.h"
#include "SDL_video.h"
#include "gambatte.h"

#include <SDL.h>
#include <stdlib.h>
#include <vector>

using namespace gambatte;

SDL_Window *win;
SDL_Renderer *rend;
SDL_Texture *maintexture;

gambatte::uint_least32_t *videoBuf;
std::ptrdiff_t pitch;
gambatte::uint_least32_t *audioBuf;
std::size_t samples;

const int texture_width = 160;
const int texture_height = 144;

uint32_t *framebuffer;

gambatte::GB gb_;

static SDL_AudioDeviceID devid_out = 0;

int audio_init(int audio_buffer_size, const char *output_device_name) {

  SDL_AudioSpec want_out, have_out;

  // Open output device first to avoid possible Directsound errors
  SDL_zero(want_out);
  want_out.freq = 44100;
  want_out.format = AUDIO_S16;
  want_out.channels = 2;
  want_out.samples = audio_buffer_size;
  devid_out = SDL_OpenAudioDevice(output_device_name, 0, &want_out, &have_out,
                                  SDL_AUDIO_ALLOW_ANY_CHANGE);
  if (devid_out == 0) {
    SDL_Log("Failed to open output: %s", SDL_GetError());
    return 0;
  }

  // Start audio processing
  SDL_Log("Opening audio devices");
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

  win = SDL_CreateWindow("m8c", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         window_width, window_height,
                         SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL |
                             SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN);

  rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

  SDL_RenderSetLogicalSize(rend, texture_width, texture_height);

  // Initialize an SDL texture and a raw framebuffer
  maintexture = SDL_CreateTexture(rend, SDL_PIXELFORMAT_ARGB8888,
                                  SDL_TEXTUREACCESS_TARGET, texture_width,
                                  texture_height);

  framebuffer = new gambatte::uint_least32_t[texture_width * texture_height];
  SDL_memset4(framebuffer, 0xA0000000, texture_width * texture_height);

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
  SDL_DestroyTexture(maintexture);
  SDL_DestroyRenderer(rend);
  SDL_DestroyWindow(win);
  SDL_Quit();
  return 0;
}

int render_sdl() {
  SDL_SetRenderTarget(rend, NULL);
  SDL_SetRenderDrawColor(rend, 0, 0, 0, 0);
  SDL_RenderClear(rend);
  SDL_RenderCopy(rend, maintexture, NULL, NULL);
  SDL_RenderPresent(rend);
  SDL_SetRenderTarget(rend, maintexture);
  return 0;
}

int main(int argc, char *argv[]) {

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

  err = gb_.load("lsdj.gb", 0);
  if (err != 0) {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Could not load ROM");
    destroy_sdl();
    return 1;
  }

  for (;;) {
    gb_.runFor(framebuffer, 160, gambatte::uint_least32_t *audioBuf, std::size_t &samples)
    render_sdl();
    SDL_Delay(100);
  }

  //  SDL_UpdateTexture(maintexture, NULL, framebuffer, texture_width *
  //  sizeof(framebuffer[0]));

  destroy_sdl();

  return 0;
}
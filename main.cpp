#include "audioout.h"
#include "audiosink.h"
#include "framewait.h"
#include "gambatte.h"
#include "gbint.h"
#include "input.h"
#include "resample/resamplerinfo.h"
#include "skipsched.h"
#include "usec.h"

#include <SDL.h>
#include <cstddef>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>

using namespace gambatte;

SDL_Window *win;
SDL_Renderer *rend;
SDL_Texture *maintexture;
SDL_Texture *target_texture;

const int texture_width = 160;
const int texture_height = 144;

static uint32_t *videoBuf = new uint32_t[texture_width * texture_height];

static gambatte::GB gb_;

static std::size_t const gb_samples_per_frame = 35112;
static std::size_t const gambatte_max_overproduction = 2064;

int render_sdl() {
  SDL_SetRenderTarget(rend, NULL);
  SDL_SetRenderDrawColor(rend, 0, 0, 0, 0);
  SDL_RenderClear(rend);
  SDL_RenderCopy(rend, maintexture, NULL, NULL);
  SDL_RenderPresent(rend);
  return 0;
}

void destroy_sdl() {
  SDL_Log("Shutting down");
  SDL_PauseAudio(1);
  SDL_CloseAudio();
  SDL_DestroyTexture(maintexture);
  SDL_DestroyRenderer(rend);
  SDL_DestroyWindow(win);
  SDL_Quit();
}

// Handles CTRL+C / SIGINT
void intHandler(int dummy) { exit(0); }

int initialize_sdl() {
  const int window_width = 640;  // SDL window width
  const int window_height = 480; // SDL window height

  SDL_Log("Initializing SDL");
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL_Init: %s\n", SDL_GetError());
    return -1;
  }

  // SDL documentation recommends this
  atexit(destroy_sdl);

  SDL_Log("Creating window");
  win = SDL_CreateWindow("worlds no1 bestest emulator", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, window_width, window_height,
                         SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL |
                             SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN);

  SDL_Log("Creating renderer");
  rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

  SDL_RenderSetLogicalSize(rend, texture_width, texture_height);

  // Initialize an SDL texture and a raw framebuffer
  maintexture = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGB888,
                                  SDL_TEXTUREACCESS_STREAMING, texture_width,
                                  texture_height);

  // clear the main texture
  SDL_SetRenderTarget(rend, maintexture);
  SDL_SetRenderDrawColor(rend, 0, 0, 0, 1);
  SDL_RenderClear(rend);

  SDL_LogSetAllPriority(SDL_LOG_PRIORITY_DEBUG);
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

  const int sampleRate = 48000;
  const int latency = 133;
  const int periods = 4;
  const char *rom_filename;

  if (argc > 1) {
    rom_filename = argv[1];
  } else {
    rom_filename = "";
  }

  signal(SIGINT, intHandler);
  signal(SIGTERM, intHandler);
#ifdef SIGQUIT
  signal(SIGQUIT, intHandler);
#endif

  int err = 0;

  err = initialize_sdl();

  if (err != 0) {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Could not initialize SDL.");
    destroy_sdl();
    return 1;
  }

  // initial scan for (existing) game controllers
  initialize_game_controllers();

  std::size_t bufsamples = 0;
  uint *bytes = nullptr;
  Array<Uint32> const audioBuf(gb_samples_per_frame +
                               gambatte_max_overproduction);
  AudioOut aout(sampleRate, latency, periods, ResamplerInfo::get(1),
                audioBuf.size());
  FrameWait frameWait;
  SkipSched skipSched;
  bool audioOutBufLow = false;
  int maintexture_pitch = 0;

  gb_.setInputGetter((gambatte::InputGetter *)&get_input, NULL);

  SDL_PauseAudio(0);

  err = gb_.loadBios("gbc_bios.bin", 0, 0);
  if (err != 0) {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Could not load BIOS");
    return 1;
  }

  err = gb_.load(rom_filename, GB::CGB_MODE);
  if (err != 0) {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Could not load ROM");
    return 1;
  }

  for (;;) {

    std::size_t runsamples = gb_samples_per_frame - bufsamples;
    std::ptrdiff_t const vidFrameDoneSampleCnt =
        gb_.runFor(videoBuf, texture_width, audioBuf + bufsamples, runsamples);
    std::size_t const outsamples = vidFrameDoneSampleCnt >= 0
                                       ? bufsamples + vidFrameDoneSampleCnt
                                       : bufsamples + runsamples;
    bufsamples += runsamples;
    bufsamples -= outsamples;

    bool const blit =
        vidFrameDoneSampleCnt >= 0 && !skipSched.skipNext(audioOutBufLow);

    if (blit) {

      SDL_LockTexture(maintexture, nullptr, reinterpret_cast<void **>(&bytes),
                      &maintexture_pitch);
      SDL_memcpy(bytes, videoBuf,
                 texture_width * texture_height * sizeof(uint32_t));
      SDL_UnlockTexture(maintexture);
    }

    AudioOut::Status const &astatus = aout.write(audioBuf, outsamples);
    audioOutBufLow = astatus.low;

    if (blit) {
      usec_t ft = (16743ul - 16743 / 1024) * sampleRate / astatus.rate;
      frameWait.waitForNextFrameTime(ft);
      render_sdl();
    }

    std::memmove(audioBuf, audioBuf + outsamples,
                 bufsamples * sizeof *audioBuf);
  }

  destroy_sdl();

  return 0;
}
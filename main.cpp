#include "audioout.h"
#include "audiosink.h"
#include "framewait.h"
#include "gambatte.h"
#include "gbint.h"
#include "resample/resamplerinfo.h"
#include "skipsched.h"
#include "usec.h"

#include <SDL.h>
#include <cstddef>
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

gambatte::uint_least32_t *videoBuf =
    new uint32_t[texture_width * texture_height];
uint32_t *audiobuffer;

SDL_AudioSpec want_out, have_out;

uint32_t *framebuffer;

gambatte::GB gb_;

FrameWait frameWait;

static std::size_t const gb_samples_per_frame = 35112;
static std::size_t const gambatte_max_overproduction = 2064;

static SDL_AudioDeviceID devid_out = 0;

int render_sdl() {
  SDL_SetRenderTarget(rend, NULL);
  SDL_SetRenderDrawColor(rend, 0, 0, 0, 0);
  SDL_RenderClear(rend);
  SDL_RenderCopy(rend, maintexture, NULL, NULL);
  SDL_RenderPresent(rend);
  return 0;
}

/* int audio_init(int audio_buffer_size, const char *output_device_name) {

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
*/
int initialize_sdl() {
  const int window_width = 640;  // SDL window width
  const int window_height = 480; // SDL window height

  SDL_Log("Initializing SDL");
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL_Init: %s\n", SDL_GetError());
    return -1;
  }

  // SDL documentation recommends this
  atexit(SDL_Quit);

  SDL_Log("Creating window");
  win = SDL_CreateWindow("worlds no1 bestest emulator", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, window_width, window_height,
                         SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL |
                             SDL_WINDOW_RESIZABLE);

  SDL_Log("Creating renderer");
  rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

  SDL_RenderSetLogicalSize(rend, texture_width, texture_height);

  // Initialize an SDL texture and a raw framebuffer
  maintexture = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGB888,
                                  SDL_TEXTUREACCESS_STREAMING, texture_width,
                                  texture_height);

  audiobuffer = new gambatte::uint_least32_t[1024 * 1024];

  // clear the main texture
  SDL_SetRenderTarget(rend, maintexture);
  SDL_SetRenderDrawColor(rend, 0, 0, 0, 1);
  SDL_RenderClear(rend);

  // Initialize audio using system default output
  // audio_init(512, NULL);

  SDL_LogSetAllPriority(SDL_LOG_PRIORITY_DEBUG);
  return 0;
}

int destroy_sdl() {
  // audio_destroy();
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

  const int sampleRate = 48000;
  const int latency = 133;
  const int periods = 4;

  SDL_Event e;
  int err = 0;
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

  err = initialize_sdl();

  if (err != 0) {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Could not initialize SDL.");
    destroy_sdl();
    return 1;
  }  

  err = gb_.loadBios("gbc_bios.bin", 0, 0);
  if (err != 0) {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Could not load BIOS");
    return 1;
  }

  err = gb_.load("lsdj.gb", GB::CGB_MODE);
  if (err != 0) {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Could not load ROM");
    return 1;
  }

  for (;;) {

    while (SDL_PollEvent(&e) != 0) {
      switch (e.type) {
      case SDL_QUIT:
        destroy_sdl();
        return 0;
      }
    }

    std::size_t runsamples = gb_samples_per_frame - bufsamples;
    std::ptrdiff_t const vidFrameDoneSampleCnt = gb_.runFor(
        videoBuf, texture_width, audiobuffer + bufsamples, runsamples);
    std::size_t const outsamples = vidFrameDoneSampleCnt >= 0
                                       ? bufsamples + vidFrameDoneSampleCnt
                                       : bufsamples + runsamples;
    bufsamples += runsamples;
    bufsamples -= outsamples;

    bool const blit =
        vidFrameDoneSampleCnt >= 0; // && !skipSched.skipNext(audioOutBufLow);

    if (blit) {

      SDL_LockTexture(maintexture, nullptr, reinterpret_cast<void **>(&bytes),
                      &maintexture_pitch);
      SDL_memcpy(bytes, videoBuf,
                 texture_width * texture_height * sizeof(uint32_t));
      SDL_UnlockTexture(maintexture);
    }
    
    //AudioOut::Status const &astatus = aout.write(audioBuf, outsamples);
    //audioOutBufLow = astatus.low;

    if (blit) {
      usec_t ft = (16743ul - 16743 / 1024) * 1; //sampleRate / astatus.rate;
      frameWait.waitForNextFrameTime(ft);
      render_sdl();
    }

    std::memmove(audioBuf, audioBuf + outsamples,
                 bufsamples * sizeof *audioBuf);
  }

  destroy_sdl();

  return 0;
}
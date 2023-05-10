#include "SDL_events.h"
#include "SDL_keycode.h"
#include "SDL_log.h"
#include "SDL_video.h"
#include <SDL.h>
#include "gambatte.h"

using namespace gambatte;

SDL_Window *win;
SDL_Renderer *rend;
SDL_Texture *maintexture;

gambatte::GB gb_;

struct GbVidBuf;

GbVidBuf setPixelBuffer(void *pixels, PixelBuffer::PixelFormat format, std::ptrdiff_t pitch);

struct ButtonInfo {
  char const *label;
  char const *category;
  int defaultKey;
  int defaultAltKey;
  unsigned char defaultFpp;
};

static ButtonInfo const buttonInfoGbUp = { "Up", "Game", SDLK_UP, 0, 0 };
static ButtonInfo const buttonInfoGbDown = { "Down", "Game", SDLK_DOWN, 0, 0 };
static ButtonInfo const buttonInfoGbLeft = { "Left", "Game", SDLK_LEFT, 0, 0 };
static ButtonInfo const buttonInfoGbRight = { "Right", "Game", SDLK_RIGHT, 0, 0 };
static ButtonInfo const buttonInfoGbA = { "A", "Game", SDLK_x, 0, 0 };
static ButtonInfo const buttonInfoGbB = { "B", "Game", SDLK_z, 0, 0 };
static ButtonInfo const buttonInfoGbStart = { "Start", "Game", SDLK_s, 0, 0 };
static ButtonInfo const buttonInfoGbSelect = { "Select", "Game", SDLK_a, 0, 0 };



int initialize_sdl() {
  const int window_width = 640;  // SDL window width
  const int window_height = 480; // SDL window height

  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL_Init: %s\n", SDL_GetError());
    return 0;
  }

  // SDL documentation recommends this
  atexit(SDL_Quit);

  win = SDL_CreateWindow("m8c", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         window_width, window_height,
                         SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL |
                             SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN);

  rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

  SDL_RenderSetLogicalSize(rend, 320, 240);

  maintexture = SDL_CreateTexture(rend, SDL_PIXELFORMAT_ARGB8888,
                                  SDL_TEXTUREACCESS_TARGET, 320, 240);

  // clear the main texture
  SDL_SetRenderTarget(rend, maintexture);
  SDL_SetRenderDrawColor(rend, 0, 0, 0, 1);
  SDL_RenderClear(rend);

  SDL_LogSetAllPriority(SDL_LOG_PRIORITY_DEBUG);
  return 1;
}

int destroy_sdl(){
  SDL_DestroyTexture(maintexture);
  SDL_DestroyRenderer(rend);
  SDL_DestroyWindow(win);
  SDL_Quit();
  return 1;
}

int main(int argc, char *argv[]) {

  int result = 0;
  result = initialize_sdl();

  if (result == 0) {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Could not initialize SDL.");
    destroy_sdl();
    return 1;
  }



  return 0;
}
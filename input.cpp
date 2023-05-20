#include "input.h"
#include <SDL.h>
#include <stdio.h>

#define MAX_CONTROLLERS 4

SDL_GameController *game_controllers[MAX_CONTROLLERS];

static bool input_state[INPUT_MAX];
static int num_joysticks = 0;

// Opens available game controllers and returns the amount of opened controllers
int initialize_game_controllers() {

  num_joysticks = SDL_NumJoysticks();
  int controller_index = 0;

  SDL_Log("Looking for game controllers\n");
  SDL_Delay(
      10); // Some controllers like XBone wired need a little while to get ready

  // Try to load the game controller database file
  char db_filename[1024] = {0};
  snprintf(db_filename, sizeof(db_filename), "%sgamecontrollerdb.txt",
           SDL_GetPrefPath("", "gambatte-sdl2"));
  SDL_Log("Trying to open game controller database from %s", db_filename);
  SDL_RWops *db_rw = SDL_RWFromFile(db_filename, "rb");
  if (db_rw == NULL) {
    snprintf(db_filename, sizeof(db_filename), "%sgamecontrollerdb.txt",
             SDL_GetBasePath());
    SDL_Log("Trying to open game controller database from %s", db_filename);
    db_rw = SDL_RWFromFile(db_filename, "rb");
  }

  if (db_rw != NULL) {
    int mappings = SDL_GameControllerAddMappingsFromRW(db_rw, 1);
    if (mappings != -1)
      SDL_Log("Found %d game controller mappings", mappings);
    else
      SDL_LogError(SDL_LOG_CATEGORY_INPUT,
                   "Error loading game controller mappings.");
  } else {
    SDL_LogError(SDL_LOG_CATEGORY_INPUT,
                 "Unable to open game controller database file.");
  }

  // Open all available game controllers
  for (int i = 0; i < num_joysticks; i++) {
    if (!SDL_IsGameController(i))
      continue;
    if (controller_index >= MAX_CONTROLLERS)
      break;
    game_controllers[controller_index] = SDL_GameControllerOpen(i);
    SDL_Log("Controller %d: %s", controller_index + 1,
            SDL_GameControllerName(game_controllers[controller_index]));
    controller_index++;
  }

  return controller_index;
}

// Closes all open game controllers
void close_game_controllers() {

  for (int i = 0; i < MAX_CONTROLLERS; i++) {
    if (game_controllers[i])
      SDL_GameControllerClose(game_controllers[i]);
  }
}

// Handles keyboard keys
static void handle_normal_keys(SDL_Event *event, bool state) {
  switch (event->key.keysym.sym) {
  case SDLK_UP:
    input_state[INPUT_UP] = state;
    break;
  case SDLK_LEFT:
    input_state[INPUT_LEFT] = state;
    break;
  case SDLK_RIGHT:
    input_state[INPUT_RIGHT] = state;
    break;
  case SDLK_DOWN:
    input_state[INPUT_DOWN] = state;
    break;
  case SDLK_z:
  case SDLK_LSHIFT:
    input_state[INPUT_SELECT] = state;
    break;
  case SDLK_x:
  case SDLK_SPACE:
    input_state[INPUT_START] = state;
    break;
  case SDLK_s:
    input_state[INPUT_B] = state;
    break;
  case SDLK_d:
    input_state[INPUT_A] = state;
    break;
  case SDLK_DELETE:
    input_state[INPUT_A] = state;
    input_state[INPUT_B] = state;
    break;
  case SDLK_ESCAPE:
    exit(0);
    break;
  }
}

// Check whether a button is pressed on a gamepad and return 1 if pressed.
static int get_game_controller_button(SDL_GameController *controller,
                                      int button) {

  const SDL_GameControllerButton button_mappings[8] = {
      SDL_CONTROLLER_BUTTON_A,          SDL_CONTROLLER_BUTTON_B,
      SDL_CONTROLLER_BUTTON_BACK,       SDL_CONTROLLER_BUTTON_START,
      SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDL_CONTROLLER_BUTTON_DPAD_LEFT,
      SDL_CONTROLLER_BUTTON_DPAD_UP,    SDL_CONTROLLER_BUTTON_DPAD_DOWN};

  // Check digital buttons
  if (SDL_GameControllerGetButton(controller, button_mappings[button])) {
    return 1;
  } else {
    // If digital button isn't pressed, check the corresponding analog control
    switch (button) {
    case INPUT_UP:
      return SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) <
             -32766;
    case INPUT_DOWN:
      return SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) >
             32766;
    case INPUT_LEFT:
      return SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) <
             -32766;
    case INPUT_RIGHT:
      return SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) >
             32766;
    case INPUT_SELECT:
      return SDL_GameControllerGetAxis(controller,
                                       SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 32766;
    case INPUT_START:
      return SDL_GameControllerGetAxis(
                 controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 32766;
    default:
      return 0;
    }
  }
  return 0;
}

// Handle game controllers, check all buttons and analog axis on every cycle
static void handle_game_controller_buttons() {

  // Cycle through every active game controller
  for (int gc = 0; gc < num_joysticks; gc++) {
    // Cycle through all Gameboy buttons
    for (int button = 0; button < (input_buttons_t)INPUT_MAX; button++) {
      // If the button is active, add the keycode to the active key array
      if (get_game_controller_button(game_controllers[gc], button)) {
        input_state[button] = true;
      } else {
        input_state[button] = false;
      }
    }

    // Magic combo for quitting program: Guide+Back+Start
    if (SDL_GameControllerGetButton(game_controllers[gc],
                                    SDL_CONTROLLER_BUTTON_GUIDE) &&
        SDL_GameControllerGetButton(game_controllers[gc],
                                    SDL_CONTROLLER_BUTTON_BACK) &&
        SDL_GameControllerGetButton(game_controllers[gc],
                                    SDL_CONTROLLER_BUTTON_START)) {
      exit(0);
    }
  }
}

// Handles SDL input events
void handle_sdl_events() {

  SDL_Event event;

  // Read joysticks
  handle_game_controller_buttons();

  SDL_PollEvent(&event);

  switch (event.type) {

  // Reinitialize game controllers on controller add/remove/remap
  case SDL_CONTROLLERDEVICEADDED:
  case SDL_CONTROLLERDEVICEREMOVED:
    initialize_game_controllers();
    break;

  // Keyboard events
  case SDL_KEYDOWN:
    handle_normal_keys(&event, true);
    break;

  case SDL_KEYUP:
    handle_normal_keys(&event, false);
    break;

  // Window close, OS shutdown request etc
  case SDL_QUIT:
    exit(0);
    break;

  default:
    break;
  }
}

// Converts the input state array to something that gambatte-speedrun expects
static unsigned int packedInputState(bool const inputState[],
                                     std::size_t const len) {
  unsigned is = 0;
  for (std::size_t i = 0; i < len; ++i)
    is |= inputState[i] << (i & 7);

  return is;
}

// Queries SDL event status and returns the current controller state
unsigned int get_input() {
  handle_sdl_events();
  return packedInputState(input_state,
                          sizeof input_state / sizeof input_state[0]);
}
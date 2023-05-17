#include "input.h"
#include "SDL_events.h"
#include "SDL_keycode.h"
#include "gambatte.h"
#include <SDL.h>
#include <stdio.h>

using namespace gambatte;

#define MAX_CONTROLLERS 4

SDL_GameController *game_controllers[MAX_CONTROLLERS];

// Bits for input messages
enum keycodes {
  key_a = 1,
  key_b = 1 << 1,
  key_select = 0x04,
  key_start = 0x08,
  key_right = 0x10,
  key_left = 1 << 7,
  key_up = 1 << 6,
  key_down = 1 << 5,
};

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
           SDL_GetPrefPath("", "m8c"));
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
  }
}

/*
// Check whether a button is pressed on a gamepad and return 1 if pressed.
static int get_game_controller_button(SDL_GameController *controller,
                                      int button) {

  const int button_mappings[8] = {SDL_CONTROLLERBUTTONUP, conf->gamepad_down,
                                  conf->gamepad_left,   conf->gamepad_right,
                                  conf->gamepad_opt,    conf->gamepad_edit,
                                  conf->gamepad_select, conf->gamepad_start};

  // Check digital buttons
  if (SDL_GameControllerGetButton(controller, button_mappings[button])) {
    return 1;
  } else {
    // If digital button isn't pressed, check the corresponding analog control
    switch (button) {
    case INPUT_UP:
      return SDL_GameControllerGetAxis(controller,
                                       conf->gamepad_analog_axis_updown) <
             -conf->gamepad_analog_threshold;
    case INPUT_DOWN:
      return SDL_GameControllerGetAxis(controller,
                                       conf->gamepad_analog_axis_updown) >
             conf->gamepad_analog_threshold;
    case INPUT_LEFT:
      return SDL_GameControllerGetAxis(controller,
                                       conf->gamepad_analog_axis_leftright) <
             -conf->gamepad_analog_threshold;
    case INPUT_RIGHT:
      return SDL_GameControllerGetAxis(controller,
                                       conf->gamepad_analog_axis_leftright) >
             conf->gamepad_analog_threshold;
    case INPUT_OPT:
      return SDL_GameControllerGetAxis(controller,
                                       conf->gamepad_analog_axis_opt) >
             conf->gamepad_analog_threshold;
    case INPUT_EDIT:
      return SDL_GameControllerGetAxis(controller,
                                       conf->gamepad_analog_axis_edit) >
             conf->gamepad_analog_threshold;
    case INPUT_SELECT:
      return SDL_GameControllerGetAxis(controller,
                                       conf->gamepad_analog_axis_select) >
             conf->gamepad_analog_threshold;
    case INPUT_START:
      return SDL_GameControllerGetAxis(controller,
                                       conf->gamepad_analog_axis_start) >
             conf->gamepad_analog_threshold;
    default:
      return 0;
    }
  }
  return 0;
}

// Handle game controllers, simply check all buttons and analog axis on every
// cycle
static int handle_game_controller_buttons(config_params_s *conf) {

  const int keycodes[8] = {key_up,  key_down, key_left,   key_right,
                           key_opt, key_edit, key_select, key_start};

  int key = 0;

  // Cycle through every active game controller
  for (int gc = 0; gc < num_joysticks; gc++) {
    // Cycle through all M8 buttons
    for (int button = 0; button < (input_buttons_t)INPUT_MAX; button++) {
      // If the button is active, add the keycode to the variable containing
      // active keys
      if (get_game_controller_button(conf, game_controllers[gc], button)) {
        key |= keycodes[button];
      }
    }
  }

  return key;
}
*/
// Handles SDL input events
void handle_sdl_events() {

  static int prev_key_analog = 0;

  SDL_Event event;
  /*
    // Read joysticks
    int key_analog = handle_game_controller_buttons();
    if (prev_key_analog != key_analog) {
      keycode = key_analog;
      prev_key_analog = key_analog;
    }

    // Read special case game controller buttons quit and reset
    for (int gc = 0; gc < num_joysticks; gc++) {
      if (SDL_GameControllerGetButton(game_controllers[gc], conf->gamepad_quit)
    && (SDL_GameControllerGetButton(game_controllers[gc], conf->gamepad_select)
    || SDL_GameControllerGetAxis(game_controllers[gc],
                                     conf->gamepad_analog_axis_select)))
        key = (input_msg_s){special, msg_quit};
      else if (SDL_GameControllerGetButton(game_controllers[gc],
                                           conf->gamepad_reset) &&
               (SDL_GameControllerGetButton(game_controllers[gc],
                                            conf->gamepad_select) ||
                SDL_GameControllerGetAxis(game_controllers[gc],
                                          conf->gamepad_analog_axis_select)))
        key = (input_msg_s){special, msg_reset_display};
    }
  */
  SDL_PollEvent(&event);

  switch (event.type) {

  // Reinitialize game controllers on controller add/remove/remap
  case SDL_CONTROLLERDEVICEADDED:
  case SDL_CONTROLLERDEVICEREMOVED:
    initialize_game_controllers();
    break;

  // Keyboard events. Special events are handled within SDL_KEYDOWN.
  case SDL_KEYDOWN:
    handle_normal_keys(&event, true);
    break;

  // Normal keyboard inputs
  case SDL_KEYUP:
    handle_normal_keys(&event, false);
    break;

  default:
    break;
  }
}

static unsigned packedInputState(bool const inputState[],
                                 std::size_t const len) {
  unsigned is = 0;
  for (std::size_t i = 0; i < len; ++i)
    is |= inputState[i] << (i & 7);

  return is;
}

unsigned get_input() {
  handle_sdl_events();
  return packedInputState(input_state,
                          sizeof input_state / sizeof input_state[0]);
}
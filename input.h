// Copyright 2021 Jonne Kokkonen
// Released under the MIT licence, https://opensource.org/licenses/MIT

#ifndef INPUT_H_
#define INPUT_H_

#include <stdint.h>

typedef enum input_buttons_t {
  INPUT_A,
  INPUT_B,
  INPUT_SELECT,
  INPUT_START,
  INPUT_RIGHT,
  INPUT_LEFT,
  INPUT_UP,
  INPUT_DOWN,
  INPUT_MAX
} input_buttons_t;

int initialize_game_controllers();
void close_game_controllers();
unsigned get_input();

#endif
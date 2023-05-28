#ifndef MIDI_H_
#define MIDI_H_

#include "gambatte.h"

#define EXT_MIDI_CHANNEL 7
#define MIDI_OUT_QUEUE_SIZE 1024
#define MIDI_TIME_CLOCK 0xf8
#define MIDI_START 0xfa
#define MIDI_CONTINUE 0xfb
#define MIDI_STOP 0xfc

typedef struct midi_message {
  int status;
  int d1;
  int d2;
  double extra;
} midi_message;

int midi_in_active();
int midi_out_active();

void midi_setup();
void midi_destroy();
void check_midi_messages(gambatte::GB *gb);

#endif
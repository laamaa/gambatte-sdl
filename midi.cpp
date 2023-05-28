#include "midi.h"
#include "SDL_log.h"
#include "gambatte.h"

#include "portmidi.h"
#include "pmutil.h"
#include "porttime.h"
#include <SDL.h>

static PmStream *midi_in_ext;
static PmQueue *midi_to_main;
static int _midi_in_active = 0;
static int _midi_out_active = 0;

int midi_in_active() { return _midi_in_active; }

void midi_process(PtTimestamp timestamp, void *userData) {
  if (!_midi_in_active && !_midi_out_active) {
    return;
  }

  PmError result;
  PmEvent buffer;
  midi_message msg_in;

  do {
    result = Pm_Poll(midi_in_ext);
    if (result) {
      int status, d1, d2;

      if (Pm_Read(midi_in_ext, &buffer, 1) != pmBufferOverflow) {
        status = Pm_MessageStatus(buffer.message);
        d1 = Pm_MessageData1(buffer.message);
        d2 = Pm_MessageData2(buffer.message);

        // MIDI Start message
        if (status == 0xFA) {
          msg_in.status = 0xFA;
          msg_in.d1 = 0x00;
          msg_in.d2 = 0x00;

          Pm_Enqueue(midi_to_main, &msg_in);
        }

        // MIDI Stop message
        if (status == 0xFC) {
          msg_in.status = 0xFC;
          msg_in.d1 = 0x00;
          msg_in.d2 = 0x00;

          Pm_Enqueue(midi_to_main, &msg_in);
        }

        // MIDI Clock message
        if (status == 0xF8) {
          msg_in.status = 0xF8;
          msg_in.d1 = 0x00;
          msg_in.d2 = 0x00;

          Pm_Enqueue(midi_to_main, &msg_in);
        }
      }
    }
  } while (result);
}

void midi_setup() {
  PmError pmerr = pmNoError;
  PtError pterr = ptNoError;
  int midi_input_id = 0;

  SDL_Log("Initializing MIDI");

  // Start a PortTimer timer with millisecond accuracy, calling processMidi()
  // every ms
  pterr = Pt_Start(1, &midi_process, 0);
  if (pterr != ptNoError) {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Cannot start MIDI timer. Error: %d",
                 pterr);
    return;
  }

  Pm_Initialize();

  // Create MIDI message queue to communicate with thread
  midi_to_main = Pm_QueueCreate(32, sizeof(midi_message));
  if (midi_to_main == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Cannot create MIDI msg queue");
    return;
  }

  for (int i = 0; i < Pm_CountDevices(); i++) {
    const PmDeviceInfo *dev;
    dev = Pm_GetDeviceInfo(i);
    SDL_Log("Device ID: %d, Type: %s, Name: %s", i,
            dev->input ? "Input" : "Output", dev->name);
    if (dev->input)
      midi_input_id = i;
    if (dev->input && strcmp(dev->name, "M8") == 0)
      midi_input_id = i;
  }

  pmerr = Pm_OpenInput(&midi_in_ext, midi_input_id, NULL, 0, NULL, NULL);
  if (pmerr == pmNoError) {
    _midi_in_active = 1;
    SDL_Log("Opened input ID %d, Name: %s", midi_input_id,
            Pm_GetDeviceInfo(midi_input_id)->name);
  } else {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM,
                 "Cannot open MIDI input ID %d, Name: %s. Error code: %d",
                 midi_input_id, Pm_GetDeviceInfo(midi_input_id)->name, pmerr);
    return;
  }
}

void midi_destroy() {
  PmError pmerr = pmNoError;
  PtError pterr = ptNoError;

  Pm_QueueDestroy(midi_to_main);

  pterr = Pt_Stop();
  if (pterr != ptNoError) {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM,
                 "Error stopping MIDI timer. Error: %d", pterr);
  }

  if (_midi_in_active) {
    _midi_in_active = 0;
    SDL_Log("Closing MIDI input device");
    pmerr = Pm_Close(midi_in_ext);
    if (pmerr != pmNoError) {
      SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Error closing MIDI input: %s",
                   Pm_GetErrorText(pmerr));
    }
  }
  Pm_Terminate();
}

void check_midi_messages(gambatte::GB *gb) {
  static int midi_clock_started = 0;
  int spin;
  midi_message msg;

  // Check if MIDI thread has sent us messages
  spin = Pm_Dequeue(midi_to_main, &msg);

  if (spin == 1) {
    switch (msg.status) {
    case 0xFA: // MIDI start
      SDL_Log("MIDI Clock Start");
      midi_clock_started = 1;
      gb->linkStatus(264); // enable link connection
      break;
    case 0xFC: // MIDI stop
      SDL_Log("MIDI Clock Stop");
      midi_clock_started = 0;
      gb->linkStatus(265); // disable link connection
      break;
    case 0xF8: // MIDI Clock
      if (midi_clock_started) {
        
        for (int ticks = 0; ticks < 8; ticks++) {
          {
            //SDL_Log("MIDI tick");
            gb->linkStatus(0xff);    // ShiftIn
            //gb->linkStatus(0x00);
          }
        }
      }
      break;
    }
  }
}
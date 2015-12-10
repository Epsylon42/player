#pragma once

#include "data.h"
#include "decode.h"

#include <ao/ao.h>
#include <queue>

#define COMMAND_PLAYBACK_STOP   0b00000001
#define COMMAND_PLAYBACK_RESUME 0b00000100
#define COMMAND_PLAYBACK_TOGGLE 0b00000010

struct Command;

extern std::queue<std::pair<char, Command*> > playbackControl;
extern bool playbackPause;

void play(Track* track);
void playPacket(AVPacket* packet, ao_device* device, Track* track);
void processPlaybackCommand(Track* track);

struct Command
{
   
};

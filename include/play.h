#pragma once

#include "data.h"
#include "decode.h"

#include <ao/ao.h>
#include <queue>
#include <deque>

#define COMMAND_PLAYBACK_PAUSE  0b00000001
#define COMMAND_PLAYBACK_RESUME 0b00000100
#define COMMAND_PLAYBACK_TOGGLE 0b00000010
#define COMMAND_PLAYBACK_STOP   0b00001000

#define OPTION_PLAYBACK_SHUFFLE         0b0000000000000011
#define OPTION_PLAYBACK_SHUFFLE_ALBUMS  0b0000000000000001
#define OPTION_PLAYBACK_SHUFFLE_TRACKS  0b0000000000000010

struct Command;

extern std::queue<std::pair<char, Command*> > playbackControl;
extern bool playbackPause;
extern std::thread* playback;

void play(Track* track);
void startPlayback(Artist* artist, uint_16 options = 0);
void startPlayback(Album* album, uint_16 options = 0);
void startPlayback(Track* track, uint_16 options = 0);
void playbackThread(std::deque<Track*>* tracksToPlay);
void playPacket(AVPacket* packet, ao_device* device, Track* track);
void processPlaybackCommand();

struct Command
{
   Command();
};

namespace NowPlaying
{
   extern Track* track;
   extern int frame;

   void reset();
}

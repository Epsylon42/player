#pragma once

#include "data.hpp"
#include "decode.hpp"

#include <ao/ao.h>
#include <queue>
#include <deque>
#include <memory>

using namespace std;

#define PLAYBACK_COMMAND_PAUSE  0b00000001
#define PLAYBACK_COMMAND_RESUME 0b00000100
#define PLAYBACK_COMMAND_TOGGLE 0b00000010
#define PLAYBACK_COMMAND_STOP   0b00001000
#define PLAYBACK_COMMAND_PREV   0b00010000
#define PLAYBACK_COMMAND_NEXT   0b00100000

#define PLAYBACK_OPTION_SHUFFLE         0b0000000000000011
#define PLAYBACK_OPTION_SHUFFLE_ALBUMS  0b0000000000000001
#define PLAYBACK_OPTION_SHUFFLE_TRACKS  0b0000000000000010

struct Command;

extern queue<Command*> playbackControl;
extern bool playbackPause;
extern thread* playback;

void play(shared_ptr<Track> track);
void startPlayback(shared_ptr<Artist> artist, uint_16 options = 0);
void startPlayback(shared_ptr<Album> album, uint_16 options = 0);
void startPlayback(shared_ptr<Track> track, uint_16 options = 0);
void playbackThread(deque<shared_ptr<Track> >* tracksToPlay);
void playPacket(AVPacket* packet, ao_device* device, shared_ptr<Track> track);
void processPlaybackCommand();

struct Command
{
   char commandID;
   Command();
   Command(char commandID);
};

namespace NowPlaying
{
   extern shared_ptr<Track> track;
   extern int frame;
   extern bool playing;

   void reset();
}

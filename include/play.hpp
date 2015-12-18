#pragma once

#include "data.hpp"
#include "decode.hpp"

#include <ao/ao.h>
#include <queue>
#include <deque>
#include <memory>

const uint_8 PLAYBACK_COMMAND_PAUSE  = 0b00000001;
const uint_8 PLAYBACK_COMMAND_RESUME = 0b00000100;
const uint_8 PLAYBACK_COMMAND_TOGGLE = 0b00000010;
const uint_8 PLAYBACK_COMMAND_STOP   = 0b00001000;
const uint_8 PLAYBACK_COMMAND_PREV   = 0b00010000;
const uint_8 PLAYBACK_COMMAND_NEXT   = 0b00100000;

const uint_16 PLAYBACK_OPTION_SHUFFLE         = 0b0000000000000011;
const uint_16 PLAYBACK_OPTION_SHUFFLE_ALBUMS  = 0b0000000000000001;
const uint_16 PLAYBACK_OPTION_SHUFFLE_TRACKS  = 0b0000000000000010;

struct Command;

extern std::queue<Command*> playbackControl;
extern bool playbackPause;
extern std::thread* playback;

void play(std::shared_ptr<Track> track);
void startPlayback(std::shared_ptr<Artist> artist, uint_16 options = 0);
void startPlayback(std::shared_ptr<Album> album, uint_16 options = 0);
void startPlayback(std::shared_ptr<Track> track, uint_16 options = 0);
void playbackThread(std::deque<std::shared_ptr<Track> >* tracksToPlay);
void playPacket(AVPacket* packet, ao_device* device, std::shared_ptr<Track> track);
void processPlaybackCommand();

struct Command
{
   char commandID;
   Command();
   Command(char commandID);
};

namespace NowPlaying
{
   extern std::shared_ptr<Track> track;
   extern int frame;
   extern bool playing;

   void reset();
}

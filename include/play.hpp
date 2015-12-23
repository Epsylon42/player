#pragma once

#include "data.hpp"
#include "decode.hpp"

#include <ao/ao.h>
#include <queue>
#include <deque>
#include <memory>
#include <mutex>


const uint_8 PLAYBACK_COMMAND_PAUSE  = 1;
const uint_8 PLAYBACK_COMMAND_RESUME = 2;
const uint_8 PLAYBACK_COMMAND_TOGGLE = 3;
const uint_8 PLAYBACK_COMMAND_STOP   = 4;
const uint_8 PLAYBACK_COMMAND_PREV   = 5;
const uint_8 PLAYBACK_COMMAND_NEXT   = 6;

const uint_16 PLAYBACK_OPTION_SHUFFLE = 0b0000000000000011;

struct Command;
struct ao_device_wrapper;

namespace play
{
   extern std::mutex playbackControlMutex;
   extern std::queue<std::unique_ptr<Command>> playbackControl;
   extern bool playbackPause;
   extern std::thread* playback;
   
   namespace NowPlaying
   {
      extern std::shared_ptr<Track> track;
      extern int frame;
      extern bool playing;
      
      void reset();
   }
}

void playTrack(std::shared_ptr<Track> track);
void startPlayback(std::shared_ptr<Artist> artist, uint_16 options = 0);
void startPlayback(std::shared_ptr<Album> album, uint_16 options = 0);
void startPlayback(std::shared_ptr<Track> track, uint_16 options = 0);
void playbackThread(std::deque<std::shared_ptr<Track> >* tracksToPlay, uint_16 options);
void playPacket(AVPacket* packet, ao_device* device, std::shared_ptr<Track> track);
void processPlaybackCommand();
void sendPlaybackCommand(Command* command);
bool playbackInProcess();

struct Command
{
   char commandID;
   Command();
   Command(char commandID);
};

struct ao_device_wrapper
{
   ao_device* device;

   ao_device_wrapper(ao_device* device);
   ~ao_device_wrapper();
   operator ao_device*();
};

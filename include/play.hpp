#pragma once

#include "data.hpp"
#include "decode.hpp"
#include "playlist.hpp"

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
const uint_8 PLAYBACK_COMMAND_PLAY   = 7;
const uint_8 PLAYBACK_COMMAND_EXIT   = 8;

const uint_16 PLAYBACK_OPTION_SHUFFLE = 0b0000000000000011;

struct Command;
struct ao_device_wrapper;

namespace play
{
   extern std::mutex playbackControlMutex;

   /*
     commands are processed in two functions: 
     playbackThreadWait() and processPlaybackCommand()

     playbackThreadWait() processes everything in place, when
     processPlaybackCommand() can throw commandID (mainly of commands
     that stop current playback) so that it could be processed by
     upper(?) function, which is playbackThread()
    */
   extern std::queue<std::unique_ptr<Command>> playbackControl;
   extern bool playbackPause;
   extern std::unique_ptr<std::thread> playback;
   
   namespace NowPlaying
   {
      extern std::shared_ptr<Track> track;
      extern int frame;
      extern bool playing;
      
      void reset();
   }
}

void initPlay();
void endPlay();
void playTrack(std::shared_ptr<Track> track);
void startPlayback(std::shared_ptr<Artist> artist, uint_16 options = 0);
void startPlayback(std::shared_ptr<Album> album, uint_16 options = 0);
void startPlayback(std::shared_ptr<Track> track, uint_16 options = 0);
void startPlayback(std::shared_ptr<Playlist> playlist, uint_16 options = 0);
std::unique_ptr<std::deque<std::shared_ptr<Track>>> playbackThreadWait();
void playbackThread();
void playPacket(AVPacket* packet, ao_device* device, std::shared_ptr<Track> track);
void processPlaybackCommand();
void sendPlaybackCommand(Command* command);
bool playbackInProcess();

struct Command
{
   uint_8 commandID;
   Command();
   Command(char commandID);
   virtual ~Command();
};

struct PlayCommand : Command
{
   std::unique_ptr<std::deque<std::shared_ptr<Track>>> tracks;
   uint_16 options;
   PlayCommand(std::deque<std::shared_ptr<Track>> tracks, uint_16 options);
};

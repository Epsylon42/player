#include "play.hpp"

#include <unistd.h>
#include <algorithm>
#include <memory>

using namespace std;
using namespace play;

shared_ptr<Track> play::NowPlaying::track;
int               play::NowPlaying::frame = 0;
bool              play::NowPlaying::playing = false;


mutex                      play::playbackControlMutex;
queue<unique_ptr<Command>> play::playbackControl;
bool                       play::playbackPause = false;
thread*                    play::playback = nullptr;


void playTrack(shared_ptr<Track> track)
{
   //av_dump_format(track->container, 1, track->filePath.c_str(), 0);
   track->open();
   NowPlaying::track = track;
   NowPlaying::frame = 0;
   
   ao_device_wrapper device(ao_open_live(ao_default_driver_id(), track->sampleFormat, nullptr));
   if (device.device == nullptr)
   {
      printf("Could not open device\n");
      exit(0);
   }

   bool endOfStream = false;
   while (!endOfStream)
   {
      processPlaybackCommand();

      if (playbackPause)
      {
	 usleep(1000);
	 continue;
      }
      
      AVPacket* packet = new AVPacket;
      av_init_packet(packet);
      if (av_read_frame(track->container, packet) < 0)
      {
	 delete packet;
	 endOfStream = true;
	 continue;
      }
      if (packet->stream_index != track->streamID)
      {
	 av_packet_unref(packet);
	 delete packet;
	 //printf("Packet from invalid stream\n");
	 continue;
      }

      playPacket(packet, device, track);
      
      av_packet_unref(packet);
      av_free_packet(packet);
      delete packet;
   }

   playbackPause = false;
   track->close();
   usleep(1000000);
   NowPlaying::reset();
}

void startPlayback(shared_ptr<Artist> artist, uint_16 options)
{
   deque<shared_ptr<Track>>* playbackDeque = new deque<shared_ptr<Track>>;
   for (auto album : artist->albumsDeque)
   {
      if (album != artist->albumsMap["all"])
      {
	 playbackDeque->insert(playbackDeque->end(), album->tracksDeque.begin(), album->tracksDeque.end());
      }
   }
   if (options & PLAYBACK_OPTION_SHUFFLE)
   {
      random_shuffle(playbackDeque->begin(), playbackDeque->end());
   }

   if (NowPlaying::playing == true)
   {
      playbackControl.push(make_unique<Command>(PLAYBACK_COMMAND_STOP));
      playback->join();
   }
   playback = new thread(playbackThread, playbackDeque, 0);
}

void startPlayback(shared_ptr<Album> album, uint_16 options)
{
   deque<shared_ptr<Track>>* playbackDeque = new deque<shared_ptr<Track>>;
   playbackDeque->insert(playbackDeque->end(), album->tracksDeque.begin(), album->tracksDeque.end()); 
   
   if (options & PLAYBACK_OPTION_SHUFFLE)
   {
      random_shuffle(playbackDeque->begin(), playbackDeque->end());
   }

   if (NowPlaying::playing == true)
   {
      playbackControl.push(make_unique<Command>(PLAYBACK_COMMAND_STOP));
      playback->join();
   }
   playback = new thread(playbackThread, playbackDeque, 0);
}

void startPlayback(shared_ptr<Track> track, uint_16 options)
{
   if (NowPlaying::playing == true)
   {
      playbackControl.push(make_unique<Command>(PLAYBACK_COMMAND_STOP));
      playback->join();
   }
   playback = new thread(playbackThread, new deque<shared_ptr<Track>>({track}), 0);
}

void playbackThread(deque<shared_ptr<Track>>* tracksToPlay, uint_16 options)
{
   NowPlaying::playing = true;
   for (auto track = tracksToPlay->begin(); track != tracksToPlay->end();)
   {
      try
      {
	 playTrack(*track);
      }
      catch (char e)
      {
	 switch (e)
	 {
	    case PLAYBACK_COMMAND_STOP:
	       goto end;
	       break;
	    case PLAYBACK_COMMAND_NEXT:
	       track++;
	       continue;
	       break;
	    case PLAYBACK_COMMAND_PREV:
	       if (track != tracksToPlay->begin())
	       {
		  track--;
		  continue;
	       }
	       break;
	    default:
	       break;
	 }
      }
      track++; //TODO: move this back to the cycle definition(?) if this is possible wihout even more (in/de)crements
   }
  end:;
   tracksToPlay->clear();
   delete tracksToPlay;
   playbackPause = false;
   NowPlaying::reset();
   NowPlaying::playing = false;
}

void playPacket(AVPacket* packet, ao_device* device, shared_ptr<Track> track)
{
   AVFrame* frame;
   frame = decodeFrame(track, track->sampleFormat, packet);
   if (frame == nullptr)
   {
      return;
   }
      
   int dataSize =
      av_samples_get_buffer_size(nullptr,
				 track->codecContext->channels,
				 frame->nb_samples,
				 track->codecContext->sample_fmt,
				 1);
   for (int i = 0; i < track->sampleFormat->channels; i++)
   {
      ao_play(device, reinterpret_cast<char*>(frame->extended_data[i]), frame->linesize[i]);
   }
   NowPlaying::frame++;
   
   av_frame_unref(frame);
   av_frame_free(&frame);
   delete frame;
}

void processPlaybackCommand()
{
   if (!playbackControl.empty())
   {
      playbackControlMutex.lock();
      unique_ptr<Command> command = move(playbackControl.front());
      playbackControl.pop();
      playbackControlMutex.unlock();

      switch (command->commandID)
      {
	 case PLAYBACK_COMMAND_PAUSE:
	    playbackPause = true;
	    break;
	 case PLAYBACK_COMMAND_RESUME:
	    playbackPause = false;
	    break;
	 case PLAYBACK_COMMAND_TOGGLE:
	    playbackPause = !playbackPause;
	    break;
	 default:
	    throw command->commandID;
	    break;
      }
   }
}

void sendPlaybackCommand(Command* command)
{
   playbackControlMutex.lock();
   playbackControl.push(unique_ptr<Command>(command));
   playbackControlMutex.unlock();
}

bool playbackInProcess()
{
   return NowPlaying::playing;
}


Command::Command() {}

Command::Command(char commandID) :
   commandID(commandID) {}


ao_device_wrapper::ao_device_wrapper(ao_device* device) :
   device(device) {}

ao_device_wrapper::~ao_device_wrapper()
{
   ao_close(device);
}

ao_device_wrapper::operator ao_device*()
{
   return device;
}


void NowPlaying::reset()
{
   track.reset();
   frame = 0;
}

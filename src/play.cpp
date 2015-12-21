#include "play.hpp"

#include <unistd.h>
#include <algorithm>
#include <memory>

using namespace std;

shared_ptr<Track> NowPlaying::track;
int NowPlaying::frame = 0;
bool NowPlaying::playing = false;

queue<Command*> playbackControl;
bool playbackPause = false;
thread* playback = nullptr;

void play(shared_ptr<Track> track)
{
   //av_dump_format(track->container, 1, track->filePath.c_str(), 0);
   track->open();
   NowPlaying::track = track;
   NowPlaying::frame = 0;
   NowPlaying::playing = true;
   
   ao_device* device = ao_open_live(ao_default_driver_id(), track->sampleFormat, nullptr);
   if (device == nullptr)
   {
      printf("Could not open device\n");
      exit(0);
   }

   bool endOfStream = false;
   while (!endOfStream)
   {
      processPlaybackCommand();

      if (::playbackPause)
      {
	 usleep(1000);
	 continue;
      }
      
      AVPacket* packet = new AVPacket;
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
      delete packet;
   }
   ao_close(device); //TODO: this should be deleted even if stack unfolds(?) with exception
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
      ::playbackControl.push(new Command(PLAYBACK_COMMAND_STOP));
      ::playback->join();
   }
   ::playback = new thread(playbackThread, playbackDeque, 0);
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
      ::playbackControl.push(new Command(PLAYBACK_COMMAND_STOP));
      ::playback->join();
   }
   ::playback = new thread(playbackThread, playbackDeque, 0);
}

void startPlayback(shared_ptr<Track> track, uint_16 options)
{
   if (NowPlaying::playing == true)
   {
      ::playbackControl.push(new Command(PLAYBACK_COMMAND_STOP));
      ::playback->join();
   }
   ::playback = new thread(playbackThread, new deque<shared_ptr<Track>>({track}), 0);
}

void playbackThread(deque<shared_ptr<Track>>* tracksToPlay, uint_16 options)
{
   for (auto track = tracksToPlay->begin(); track != tracksToPlay->end();)
   {
      try
      {
	 play(*track);
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
   ::playbackPause = false;
   NowPlaying::reset();
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
   delete frame;
}

void processPlaybackCommand()
{
   if (!::playbackControl.empty())
   {
      auto command = ::playbackControl.front();
      ::playbackControl.pop();

      switch (command->commandID)
      {
	 case PLAYBACK_COMMAND_PAUSE:
	    ::playbackPause = true;
	    break;
	 case PLAYBACK_COMMAND_RESUME:
	    ::playbackPause = false;
	    break;
	 case PLAYBACK_COMMAND_TOGGLE:
	    ::playbackPause = !::playbackPause;
	    break;
	 default:
	    throw command->commandID;
	    break;
      }
   }
}


Command::Command() {}

Command::Command(char commandID) :
   commandID(commandID) {}


void NowPlaying::reset()
{
   track.reset();
   playing = false;
   frame = 0;
}

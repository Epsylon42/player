#include "play.h"

#include <unistd.h>
#include <algorithm>

Track* NowPlaying::track = NULL;
int    NowPlaying::frame = 0;

std::queue<Command*> playbackControl;
bool playbackPause = false;
std::thread* playback = NULL;

void play(Track* track)
{
   //av_dump_format(track->container, 1, track->filePath.c_str(), 0);
   track->open();
   NowPlaying::track = track;
   NowPlaying::frame = 0;
   
   ao_device* device = ao_open_live(ao_default_driver_id(), track->sampleFormat, NULL);
   if (device == NULL)
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

void startPlayback(Artist* artist, uint_16 options)
{
   std::deque<Track*>* playbackDeque = new std::deque<Track*>;
   for (auto album : artist->albumsDeque)
   {
      if (album != artist->albumsMap["all"])
      {
	 playbackDeque->insert(playbackDeque->end(), album->tracksDeque.begin(), album->tracksDeque.end());
      }
   }
   if (options & OPTION_PLAYBACK_SHUFFLE_ALBUMS)
   {
      std::random_shuffle(playbackDeque->begin(), playbackDeque->end());
   }

   if (NowPlaying::track != NULL)
   {
      playbackControl.push(new Command(COMMAND_PLAYBACK_STOP));
      playback->join();
   }
   playback = new std::thread(playbackThread, playbackDeque);
}

void startPlayback(Album* album, uint_16 options)
{
   std::deque<Track*>* playbackDeque = new std::deque<Track*>;
   playbackDeque->insert(playbackDeque->end(), album->tracksDeque.begin(), album->tracksDeque.end()); 
   
   if (options & OPTION_PLAYBACK_SHUFFLE_TRACKS)
   {
      std::random_shuffle(playbackDeque->begin(), playbackDeque->end());
   }

   if (NowPlaying::track != NULL)
   {
      playbackControl.push(new Command(COMMAND_PLAYBACK_STOP));
      playback->join();
   }
   playback = new std::thread(playbackThread, playbackDeque);
}

void startPlayback(Track* track, uint_16 options)
{
   if (NowPlaying::track != NULL)
   {
      playbackControl.push(new Command(COMMAND_PLAYBACK_STOP));
      playback->join();
   }
   playback = new std::thread(playbackThread, new std::deque<Track*>({track}));
}

void playbackThread(std::deque<Track*>* tracksToPlay)
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
	    case COMMAND_PLAYBACK_STOP:
	       goto end;
	       break;
	    case COMMAND_PLAYBACK_NEXT:
	       track++;
	       continue;
	       break;
	    case COMMAND_PLAYBACK_PREV:
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
   playbackPause = false;
   NowPlaying::reset();
}

void playPacket(AVPacket* packet, ao_device* device, Track* track)
{
   AVFrame* frame;
   frame = decodeFrame(track, track->sampleFormat, packet);
   if (frame == NULL)
   {
      return;
   }
      
   int dataSize =
      av_samples_get_buffer_size(NULL,
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
   if (!playbackControl.empty())
   {
      auto command = playbackControl.front();
      playbackControl.pop();

      switch (command->commandID)
      {
	 case COMMAND_PLAYBACK_PAUSE:
	    playbackPause = true;
	    break;
	 case COMMAND_PLAYBACK_RESUME:
	    playbackPause = false;
	    break;
	 case COMMAND_PLAYBACK_TOGGLE:
	    playbackPause = !playbackPause;
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
   track = NULL;
   frame = 0;
}

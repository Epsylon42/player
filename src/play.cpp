#include "play.h"

#include <unistd.h>
#include <algorithm>

Track* NowPlaying::track = NULL;
int    NowPlaying::frame = 0;

std::queue<std::pair<char, Command*> > playbackControl;
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
      if (album->name != "all")
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
      playbackControl.push(std::make_pair(COMMAND_PLAYBACK_STOP, new Command()));
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
      playbackControl.push(std::make_pair(COMMAND_PLAYBACK_STOP, new Command()));
      playback->join();
   }
   playback = new std::thread(playbackThread, playbackDeque);
}

void startPlayback(Track* track, uint_16 options)
{
   if (NowPlaying::track != NULL)
   {
      playbackControl.push(std::make_pair(COMMAND_PLAYBACK_STOP, new Command()));
      playback->join();
   }
   playback = new std::thread(playbackThread, new std::deque<Track*>({track}));
}

void playbackThread(std::deque<Track*>* tracksToPlay)
{
   for (auto track : *tracksToPlay)
   {
      try
      {
	 play(track);
      }
      catch (int e)
      {
	 switch (e)
	 {
	    case COMMAND_PLAYBACK_STOP:
	       playbackPause = false;
	       NowPlaying::reset();
	       goto end;
	       break;
	    default:
	       break;
	 }
      }
   }
  end:;
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

      switch (command.first)
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
	 case COMMAND_PLAYBACK_STOP:
	    throw COMMAND_PLAYBACK_STOP;
	    break;
	 default:
	    break;
      }
   }
}

Command::Command() {}

void NowPlaying::reset()
{
   track = NULL;
   frame = 0;
}

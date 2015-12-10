#include "play.h"

#include <unistd.h>

std::queue<std::pair<char, Command*> > playbackControl;
bool playbackPause = false;

void play(Track* track)
{
   //av_dump_format(track->container, 1, track->filePath.c_str(), 0);
   track->open();
   
   ao_device* device = ao_open_live(ao_default_driver_id(), track->sampleFormat, NULL);
   if (device == NULL)
   {
      printf("Could not open device\n");
      exit(0);
   }

   bool endOfStream = false;
   while (!endOfStream)
   {
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

   av_frame_unref(frame);
   delete frame;
}

void processPlaybackCommand(Track* track)
{
   if (!playbackControl.empty())
   {
      auto command = playbackControl.front();
      playbackControl.pop();

      switch (command.first)
      {
	 case COMMAND_PLAYBACK_STOP:
	    playbackPause = true;
	    break;
	 case COMMAND_PLAYBACK_RESUME:
	    playbackPause = false;
	    break;
	 case COMMAND_PLAYBACK_TOGGLE:
	    playbackPause = !playbackPause;
	    break;
	 default:
	    break;
      }
   }
}

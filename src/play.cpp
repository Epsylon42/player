#include "play.h"

void play(Track* track)
{
   //av_dump_format(track->container, 1, track->filePath.c_str(), 0);
   
   //ao_sample_format* sampleFormat = getSampleFormat(track);
   ao_device* device = ao_open_live(ao_default_driver_id(), track->sampleFormat, NULL);
   if (device == NULL)
   {
      printf("Could not open device\n");
      exit(0);
   }

   bool endOfStream = false;
   while (!endOfStream)
   {
      AVPacket* packet = new AVPacket;
      if (av_read_frame(track->container, packet) < 0)
      {
	 delete packet;
	 endOfStream = true;
	 continue;
      }
      if (packet->stream_index != track->streamID)
      {
	 delete packet;
	 printf("Packet from invalid stream\n");
	 continue;
      }
      AVFrame* frame;
      frame = decodeFrame(track, track->sampleFormat, packet);
      if (frame == NULL)
      {
	 continue;
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

      av_packet_unref(packet);
      delete packet;
      av_frame_unref(frame);
      delete frame;
   }
}


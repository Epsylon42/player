#include "decode.hpp"

ao_sample_format* getSampleFormat(Track* track)
{
   ao_sample_format* sampleFormat = new ao_sample_format;
   AVPacket packet;
   av_read_frame(track->container, &packet);

   sampleFormat->bits = av_get_bits_per_sample(track->codec->id);
   if (sampleFormat->bits == 0)
   {
      printf("Unsupported format\n");
      exit(0);
   }
   sampleFormat->channels = track->codecContext->channels;
   sampleFormat->rate = track->codecContext->sample_rate;
   sampleFormat->byte_format = AO_FMT_NATIVE;
   sampleFormat->matrix = 0;
   av_seek_frame(track->container, track->streamID, 0, AVSEEK_FLAG_ANY);

   return sampleFormat;
}

ao_sample_format* getSampleFormat(std::shared_ptr<Track> track)
{
   return getSampleFormat(track.get());
}

// In case of successful decode frame must be unreferenced and deleted
// manually after use
AVFrame* decodeFrame(std::shared_ptr<Track> track, ao_sample_format* sampleFormat, AVPacket* packet)
{
   AVFrame* decodedFrame = av_frame_alloc();
   int gotFrame = 0;
   int len =
      avcodec_decode_audio4(track->codecContext,
			    decodedFrame,
			    &gotFrame,
			    packet);
   if (len < 0)
   {
      printf("Error while decoding packet\n");
      av_frame_unref(decodedFrame);
      delete decodedFrame;
      return nullptr;
   }
   if (!gotFrame)
   {
      printf("Did not get frame\n");
      av_frame_unref(decodedFrame);
      delete decodedFrame;
      return nullptr;
   }

   return decodedFrame;
}

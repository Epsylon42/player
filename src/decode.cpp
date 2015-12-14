#include "decode.hpp"

ao_sample_format* getSampleFormat(Track* track)
{
   ao_sample_format* sampleFormat = new ao_sample_format;
   AVPacket packet;
   av_read_frame(track->container, &packet);
   switch(track->codecContext->sample_fmt)
   {
      case AV_SAMPLE_FMT_U8:
      case AV_SAMPLE_FMT_U8P:
	 sampleFormat->bits = 8;
	 break;
      case AV_SAMPLE_FMT_S16:
      case AV_SAMPLE_FMT_S16P:
	 sampleFormat->bits = 16;
	 break;
      case AV_SAMPLE_FMT_S32:
      case AV_SAMPLE_FMT_S32P:
	 sampleFormat->bits = 32;
	 break;
      default:
	 printf("Unsupported format\n");
	 exit(0);
   }
   sampleFormat->channels = track->codecContext->channels;
   sampleFormat->rate = track->codecContext->sample_rate/sampleFormat->channels;
   sampleFormat->byte_format = AO_FMT_NATIVE;
   sampleFormat->matrix = 0;
   av_seek_frame(track->container, track->streamID, 0, AVSEEK_FLAG_ANY);

   return sampleFormat;
}

ao_sample_format* getSampleFormat(shared_ptr<Track> track)
{
   return getSampleFormat(track.get());
}

// In case of successful decode frame must be unreferenced and deleted
// manually after use
AVFrame* decodeFrame(shared_ptr<Track> track, ao_sample_format* sampleFormat, AVPacket* packet)
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

#pragma once

#include "data.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

#include <ao/ao.h>

ao_sample_format* getSampleFormat(Track* track);
AVFrame* decodeFrame(Track* track, ao_sample_format* sampleFormat, AVPacket* packet);

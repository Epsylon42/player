#pragma once

#include "data.hpp"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

#include <ao/ao.h>
#include <memory>

ao_sample_format* getSampleFormat(Track* track);
ao_sample_format* getSampleFormat(std::shared_ptr<Track> track);
AVFrame* decodeFrame(std::shared_ptr<Track> track, ao_sample_format* sampleFormat, AVPacket* packet);

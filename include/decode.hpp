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
#include <string>
#include <vector>

std::string getMetadata(const std::string& key, const std::vector<AVDictionary*>& dictionaries);

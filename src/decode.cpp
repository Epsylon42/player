#include "decode.hpp"

#include <iostream>

using namespace std;

ao_sample_format getSampleFormat(Track* track)
{
    ao_sample_format sampleFormat;

    switch(track->codecContext->sample_fmt)
    {
	case AV_SAMPLE_FMT_U8:
	case AV_SAMPLE_FMT_U8P:
	    sampleFormat.bits = 8;
	    break;
	case AV_SAMPLE_FMT_S16:
	case AV_SAMPLE_FMT_S16P:
	    sampleFormat.bits = 16;
	    break;
	case AV_SAMPLE_FMT_S32:
	case AV_SAMPLE_FMT_S32P:
	    sampleFormat.bits = 32;
	    break;
	case AV_SAMPLE_FMT_DBL:
	case AV_SAMPLE_FMT_FLT:
	case AV_SAMPLE_FMT_DBLP:
	case AV_SAMPLE_FMT_FLTP:
	    sampleFormat.bits = 16;
	    break;
	default:
	    cout << "Unknown number of bits per sample" << endl;
	    exit(1);
    }

    sampleFormat.channels    = track->codecContext->channels;
    sampleFormat.rate        = track->codecContext->sample_rate;
    sampleFormat.byte_format = AO_FMT_NATIVE;
    sampleFormat.matrix = nullptr;

    return sampleFormat;
}

ao_sample_format getSampleFormat(std::shared_ptr<Track> track)
{
    return getSampleFormat(track.get());
}

string getMetadata(const string& key, const vector<AVDictionary*>& dictionaries, const string& ifNotFound)
{
    for (auto &dict : dictionaries)
    {
	AVDictionaryEntry* entry = av_dict_get(dict, key.c_str(), nullptr, 0);
	if (entry != nullptr)
	{
	    return entry->value;
	}
    }
    return ifNotFound;
}

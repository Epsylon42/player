#include "play.hpp"

#include <unistd.h>

#include <iostream>
#include <algorithm>
#include <memory>
#include <iterator>

#include <limits>
#include <cstdint>


using namespace std;
using namespace play;

shared_ptr<Track> play::NowPlaying::track;
int               play::NowPlaying::frame = 0;
bool              play::NowPlaying::playing = false;


mutex                                play::playbackControlMutex;
queue<unique_ptr<Command>>           play::playbackControl;
bool                                 play::playbackPause = false;
unique_ptr<thread> play::playback(nullptr);


void initPlay()
{
    playback.reset(new thread(playbackThread));
}

void endPlay()
{
    sendPlaybackCommand(new Command(PLAYBACK_COMMAND_EXIT));
    playback->join();
    playback.reset();
}

void playTrack(shared_ptr<Track> track)
{
    track->open();
    NowPlaying::track = track;
    NowPlaying::frame = 0;

    ao_device* device = ao_open_live(ao_default_driver_id(), &track->sampleFormat, nullptr);
    if (device == nullptr)
    {
        cout << "Could not open device" << endl;
	NowPlaying::reset();
	track->close();
	return;
    }

    /*
     * Note:
     * Following code was copied from
     * 0xdeafc0de.wordpress.com/2013/12/19/ffmpeg-audio-playback-sample/
     * and modified a bit.
     * It should probably be changed (because licenses and stuff...)
     */

    AVPacket packet;
    av_init_packet(&packet);

    int bufSize = 9999; //TODO: find the right way to get buffer size
    uint8_t* buffer = new uint8_t[bufSize];
    packet.data = buffer;
    packet.size = bufSize;

    uint8_t* samples = new uint8_t[bufSize];

    AVFormatContext* container = track->container;
    AVCodecContext*  cctx      = track->codecContext;
    int streamID               = track->streamID;

    int frameFinished = 0;
    int planeSize = 0;

    AVFrame* frame = av_frame_alloc();

    while (true)
    {
	try 
	{
	    processPlaybackCommand();
	}
	catch(...)
	{
	    av_frame_free(&frame);

	    ao_close(device);
	    av_free_packet(&packet);

	    NowPlaying::reset();
	    track->close();

	    delete [] samples;
	    delete [] buffer;

	    throw;
	}

	if (playbackPause)
	{
	    usleep(1000);
	    continue;
	}

	if (av_read_frame(container, &packet) < 0)
	{
	    break;
	}

	if (packet.stream_index != streamID)
	{
	    cout << "Packet from wrong stream" << endl; //FIXME: this won't work with ncurses, will it?
	    continue;
	}

	if(packet.stream_index==streamID)
	{
	    avcodec_decode_audio4(cctx, frame, &frameFinished, &packet);
	    av_samples_get_buffer_size(&planeSize, cctx->channels,
		    frame->nb_samples,
		    cctx->sample_fmt, 1);
	    uint16_t* out = reinterpret_cast<uint16_t*>(samples);

	    if(frameFinished)
	    {
		int write_p=0;

		switch (cctx->sample_fmt)
		{
		    case AV_SAMPLE_FMT_S16P:
			for (unsigned long nb=0;nb<planeSize/sizeof(uint16_t);nb++)
			{
			    for (int ch = 0; ch < cctx->channels; ch++) 
			    {
				out[write_p] = reinterpret_cast<uint16_t*>(frame->extended_data[ch])[nb];
				write_p++;
			    }
			}
			ao_play(device, reinterpret_cast<char*>(samples), (planeSize) * cctx->channels);
			break;

		    case AV_SAMPLE_FMT_FLTP:
			for (unsigned long nb=0;nb<planeSize/sizeof(float);nb++)
			{
			    for (int ch = 0; ch < cctx->channels; ch++) 
			    {
				out[write_p] = reinterpret_cast<float*>(frame->extended_data[ch])[nb] * std::numeric_limits<short>::max() ;
				write_p++;
			    }
			}
			ao_play(device, reinterpret_cast<char*>(samples), ( planeSize/sizeof(float) )  * sizeof(uint16_t) * cctx->channels);
			break;

		    case AV_SAMPLE_FMT_S16:
			ao_play(device, (char*)frame->extended_data[0],frame->linesize[0]);
			break;
		    case AV_SAMPLE_FMT_FLT:
			for (unsigned long nb=0;nb<planeSize/sizeof(float);nb++)
			{
			    out[nb] = static_cast<short>(reinterpret_cast<float*>(frame->extended_data[0])[nb] * std::numeric_limits<short>::max());
			}
			ao_play(device, reinterpret_cast<char*>(samples), ( planeSize/sizeof(float) )  * sizeof(uint16_t));
			break;

		    case AV_SAMPLE_FMT_U8P:
			for (unsigned long nb=0;nb<planeSize/sizeof(uint8_t);nb++)
			{
			    for (int ch = 0; ch < cctx->channels; ch++) 
			    {
				out[write_p] = (reinterpret_cast<uint8_t*>(frame->extended_data[0])[nb] - 127) * std::numeric_limits<short>::max() / 127 ;
				write_p++;
			    }
			}
			ao_play(device, reinterpret_cast<char*>(samples), ( planeSize/sizeof(uint8_t) )  * sizeof(uint16_t) * cctx->channels);
			break;

		    case AV_SAMPLE_FMT_U8:
			for (unsigned long nb=0;nb<planeSize/sizeof(uint8_t);nb++)
			{
			    out[nb] = static_cast<short>((reinterpret_cast<uint8_t*>(frame->extended_data[0])[nb] - 127) * std::numeric_limits<short>::max() / 127);
			}
			ao_play(device, reinterpret_cast<char*>(samples), ( planeSize/sizeof(uint8_t) )  * sizeof(uint16_t));
			break;

		    default:
			cout << "PCM type not supported" << endl; //FIXME: also this 
			exit(1);
		}
		NowPlaying::frame++;
	    } 
	}
        av_free_packet(&packet);
    }

    av_frame_free(&frame);

    ao_close(device);
    av_packet_unref(&packet);

    NowPlaying::reset();
    track->close();

    delete [] samples;
    delete [] buffer;
}

void startPlayback(shared_ptr<Artist> artist, uint_16 options)
{
    deque<shared_ptr<Track>> playbackDeque;
    for (auto album : artist->albumsDeque)
    {
	if (album != artist->albumsMap["all"])
	{
	    playbackDeque.insert(playbackDeque.end(), album->tracksDeque.begin(), album->tracksDeque.end());
	}
    }
    if (options & PLAYBACK_OPTION_SHUFFLE)
    {
	random_shuffle(playbackDeque.begin(), playbackDeque.end());
    }

    if (playbackDeque.empty())
    {
	return;
    }

    sendPlaybackCommand(new PlayCommand(move(playbackDeque), 0));
}

void startPlayback(shared_ptr<Album> album, uint_16 options)
{
    deque<shared_ptr<Track>> playbackDeque;

    playbackDeque.insert(playbackDeque.end(), album->tracksDeque.begin(), album->tracksDeque.end());

    if (options & PLAYBACK_OPTION_SHUFFLE)
    {
	random_shuffle(playbackDeque.begin(), playbackDeque.end());
    }

    if (playbackDeque.empty())
    {
	return;
    }

    sendPlaybackCommand(new PlayCommand(move(playbackDeque), 0));
}

void startPlayback(shared_ptr<Track> track, uint_16 options)
{
    sendPlaybackCommand(new PlayCommand(deque<shared_ptr<Track>>({track}), 0));
}

void startPlayback(shared_ptr<Playlist> playlist, uint_16 options)
{
    sendPlaybackCommand(new PlayCommand(deque<shared_ptr<Track>>(playlist->getList()), 0));
}

unique_ptr<deque<shared_ptr<Track>>> playbackThreadWait()
{
    while (true)
    {
	while (!playbackControl.empty())
	{
	    playbackControlMutex.lock();
	    unique_ptr<Command> command = move(playbackControl.front());
	    playbackControl.pop();
	    playbackControlMutex.unlock();

	    switch (command->commandID)
	    {
		case PLAYBACK_COMMAND_PLAY: //TODO: provide reliable way to make sure command is actually PlayCommand in this case
		    {
			unique_ptr<PlayCommand> playCommand(dynamic_cast<PlayCommand*>(command.release()));
			return move(playCommand->tracks);
		    }
		case PLAYBACK_COMMAND_EXIT:
		    return nullptr;
		default:
		    break;
	    }
	}
	usleep(500000);
    }
}

void playbackThread()
{
    while (true)
    {
	auto tracks = playbackThreadWait();
	if (tracks == nullptr) // caught PLAYBACK_COMMAND_EXIT command
	{
	    goto end;
	}

	playbackPause = false;
	NowPlaying::playing = true;
	for (auto track = tracks->begin(); track != tracks->end() && NowPlaying::playing;)
	{
	    try
	    {
		playTrack(*track);
	    }
	    catch (uint_8& e)
	    {
		switch (e)
		{
		    case PLAYBACK_COMMAND_STOP:
			NowPlaying::playing = false;
			break;
		    case PLAYBACK_COMMAND_NEXT:
			track++;
			continue;
		    case PLAYBACK_COMMAND_PREV:
			//FIXME: this switches between two first tracks
			if (track != tracks->begin())
			{
			    track--;
			    continue;
			}
			break;
		    case PLAYBACK_COMMAND_EXIT:
			goto end;
		    default:
			exit(0); //this should not happen
		}
	    }
	    track++; //TODO: move this back to the cycle definition(?) if this is possible wihout even more (in/de)crements
	}
	NowPlaying::reset();
    }
end:
    NowPlaying::playing = false;
    NowPlaying::reset();
}

void processPlaybackCommand()
{
    if (!playbackControl.empty())
    {
	playbackControlMutex.lock();
	unique_ptr<Command> command = move(playbackControl.front());
	playbackControl.pop();
	playbackControlMutex.unlock();

	switch (command->commandID)
	{
	    case PLAYBACK_COMMAND_PAUSE:
		playbackPause = true;
		break;
	    case PLAYBACK_COMMAND_RESUME:
		playbackPause = false;
		break;
	    case PLAYBACK_COMMAND_TOGGLE:
		playbackPause = !playbackPause;
		break;
	    default:
		throw command->commandID;
		break;
	}
    }
}

void sendPlaybackCommand(Command* command)
{
    // Do not add dublicated commands
    //!!!: this might not be a very good idea
    if (playbackControl.empty() ||
	    playbackControl.front()->commandID != command->commandID)
    {
	playbackControlMutex.lock();
	switch (command->commandID) // process special cases
	{
	    case PLAYBACK_COMMAND_PLAY:
		playbackControl.push(make_unique<Command>(PLAYBACK_COMMAND_STOP));
		break;
	    default:
		break;
	}
	playbackControl.push(unique_ptr<Command>(command));
	playbackControlMutex.unlock();
    }
}

bool playbackInProcess()
{
    return NowPlaying::playing;
}


Command::Command() {}

Command::Command(char commandID) :
    commandID(commandID) {}

    Command::~Command() {}


    PlayCommand::PlayCommand(deque<shared_ptr<Track>> tracks, uint_16 options) :
	Command(PLAYBACK_COMMAND_PLAY),
	tracks(make_unique<decltype(tracks)>(tracks.begin(), tracks.end())),
	options(options) {}


void NowPlaying::reset()
{
    track.reset();
    frame = 0;
}

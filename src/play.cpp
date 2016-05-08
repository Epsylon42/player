#include "play.hpp"
#include "log.hpp"

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
size_t            play::NowPlaying::frame  = 0;
size_t            play::NowPlaying::sample = 0;
bool              play::NowPlaying::playing = false;


mutex                                play::playbackControlMutex;
queue<unique_ptr<Command>>           play::playbackControl;
deque<unique_ptr<CommandPLAY>>       play::playbackQueue;
bool                                 play::playbackPause = false;
unique_ptr<thread> play::playback(nullptr);


void initPlay()
{
    playback.reset(new thread(playbackThread));
}

void endPlay()
{
    sendPlaybackCommand(new CommandEXIT());
    playback->join();
    playback.reset();

    playbackQueue.clear();
    while (!playbackControl.empty())
    {
        playbackControl.pop();
    }
}

void playTrack(shared_ptr<Track> track)
{
    track->open();
    NowPlaying::track = track;
    NowPlaying::frame = 0;

    ao_device* device = ao_open_live(ao_default_driver_id(), &track->sampleFormat, nullptr);
    if (device == nullptr)
    {
        log("Could not open device");
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

    uint8_t* samples = new uint8_t[bufSize];

    AVFormatContext* container = track->container;
    AVCodecContext*  cctx      = track->codecContext;
    int streamID               = track->streamID;

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
            log("Packet from wrong stream");
            continue;
        }

        if(packet.stream_index==streamID)
        {
            int frameFinished = 0;
            int planeSize = 0;

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
                        log("PCM type not supported");
                        exit(1);
                }
                NowPlaying::frame++;
                NowPlaying::sample += frame->nb_samples;
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
}

void startPlayback(shared_ptr<Artist> artist, PlaybackOptions options)
{
    deque<shared_ptr<Track>> playbackDeque;
    for (auto album : artist->albumsDeque)
    {
        if (album != artist->albumsMap["all"])
        {
            playbackDeque.insert(playbackDeque.end(), album->tracksDeque.begin(), album->tracksDeque.end());
        }
    }
    if (options & PlaybackOption::shuffle)
    {
        random_shuffle(playbackDeque.begin(), playbackDeque.end());
    }

    if (playbackDeque.empty())
    {
        return;
    }

    sendPlaybackCommand(new CommandPLAY(move(playbackDeque), options));
}

void startPlayback(shared_ptr<Album> album, PlaybackOptions options)
{
    deque<shared_ptr<Track>> playbackDeque;

    playbackDeque.insert(playbackDeque.end(), album->tracksDeque.begin(), album->tracksDeque.end());

    if (options & PlaybackOption::shuffle)
    {
        random_shuffle(playbackDeque.begin(), playbackDeque.end());
    }

    if (playbackDeque.empty())
    {
        return;
    }

    sendPlaybackCommand(new CommandPLAY(move(playbackDeque), options));
}

void startPlayback(shared_ptr<Track> track, PlaybackOptions options)
{
    sendPlaybackCommand(new CommandPLAY({track}, options));
}

void startPlayback(shared_ptr<Playlist> playlist, PlaybackOptions options)
{
    sendPlaybackCommand(new CommandPLAY(playlist->getList(), options));
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

            switch (command->type())
            {
                case CommandType::play: //TODO: provide reliable way to make sure command is actually PlayCommand in this case
                    {
                        unique_ptr<CommandPLAY> playCommand(dynamic_cast<CommandPLAY*>(command.release()));
                        return move(playCommand->tracks);
                    }
                case CommandType::exit:
                    return nullptr;
                default:
                    break;
            }
        }

        if (!playbackQueue.empty())
        {
            unique_ptr<CommandPLAY> playCommand(playbackQueue.front().release());
            playbackQueue.pop_front();

            return move(playCommand->tracks);
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
            catch (CommandType& e)
            {
                switch (e)
                {
                    case CommandType::stop:
                        NowPlaying::playing = false;
                        break;
                    case CommandType::next:
                        track++;
                        continue;
                    case CommandType::previous:
                        //FIXME: this switches between two first tracks
                        if (track != tracks->begin())
                        {
                            track--;
                            continue;
                        }
                        break;
                    case CommandType::exit:
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

        switch (command->type())
        {
            case CommandType::pause:
                playbackPause = true;
                break;
            case CommandType::resume:
                playbackPause = false;
                break;
            case CommandType::toggle:
                playbackPause = !playbackPause;
                break;
            default:
                throw command->type();
                break;
        }
    }
}

void sendPlaybackCommand(Command* command)
{
    lock_guard<mutex> lock(playbackControlMutex);

    switch (command->type()) // process special cases
    {
        case CommandType::play:
            {
                CommandPLAY* playCommand = dynamic_cast<CommandPLAY*>(command);

                if (playCommand->options & PlaybackOption::queue)
                {
                    playbackQueue.emplace_back(playCommand);
                    return;
                }
                else
                {
                    playbackQueue.clear();
                    //TODO: add option not to clear this queue

                    playbackControl.emplace(new CommandSTOP());
                    //NOTE: what is done here might be not obvious
                    //don't know if there is something that should be done here
                }
            }
        case CommandType::stop:
            playbackQueue.clear();
            break;
        default:
            break;
    }

    playbackControl.emplace(command);
}

bool playbackInProcess()
{
    return NowPlaying::playing;
}


Command::Command(CommandType commandType) :
    commandType(commandType) {};

Command::~Command() {};

CommandType Command::type() const
{
    return commandType;
}


CommandPLAY::CommandPLAY(const deque<shared_ptr<Track>>& tracks, PlaybackOptions options) :
    Command(CommandType::play),
    tracks(new deque<shared_ptr<Track>>(tracks.begin(), tracks.end())),
    options(options) {};


void NowPlaying::reset()
{
    track.reset();
    frame = 0;
    sample = 0;
}

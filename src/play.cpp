#include "play.hpp"
#include "log.hpp"

#include <iostream>
#include <algorithm>
#include <memory>
#include <iterator>
#include <thread>
#include <chrono>
#include <tuple>

#include <stack>
#include <deque>
#include <vector>

#include <limits>
#include <cstdint>


using namespace std;

using data::Track;
using data::Album;
using data::Artist;

namespace playback
{
    shared_ptr<Track> NowPlaying::track;
    size_t            NowPlaying::frame  = 0;
    size_t            NowPlaying::sample = 0;
    bool              NowPlaying::playing = false;


    queue<unique_ptr<Command>>           playbackControl;
    mutex                                playbackControlMutex;
    bool                                 playbackPause = false;
    thread playbackThread;

    void init()
    {
        playbackThread = thread(playbackThreadFunc);
    }

    void end()
    {
        sendPlaybackCommand(new CommandEXIT());
        playbackThread.join();

        while (!playbackControl.empty())
        {
            playbackControl.pop();
        }
    }

    unique_ptr<Command> playTrack(data::OpenedTrack& track)
    {
        NowPlaying::frame = 0; //TODO: add frame counter to OpenedTrack

        ao_sample_format sampleFormat;
        sampleFormat.bits = 16;
        sampleFormat.byte_format = AO_FMT_NATIVE;
        sampleFormat.channels = track.codecContext->channels;
        sampleFormat.rate = track.codecContext->sample_rate;
        sampleFormat.matrix = nullptr;

        ao_device* device = ao_open_live(ao_default_driver_id(), &sampleFormat, nullptr);
        if (device == nullptr)
        {
            log("Could not open device");
            NowPlaying::reset();
            return {};
        }

        /*
         * NOTE:
         * Following code was copied from
         * 0xdeafc0de.wordpress.com/2013/12/19/ffmpeg-audio-playback-sample/
         * and modified a bit.
         * It should probably be changed (because licenses and stuff...)
         */

        AVPacket packet;
        av_init_packet(&packet);

        int bufSize = 9999; //TODO: find the right way to get buffer size

        uint8_t* samples = new uint8_t[bufSize];

        AVFormatContext* container = track.formatContext;
        AVCodecContext*  cctx      = track.codecContext;
        int streamID               = track.audioStreamID;

        AVFrame* frame = av_frame_alloc();

        while (true)
        {
            {
                unique_ptr<Command> command = getPlaybackCommand();
                if (command)
                {
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
                            av_frame_free(&frame);

                            ao_close(device);
                            av_free_packet(&packet);

                            delete [] samples;

                            return command;
                    }
                }
            }

            if (playbackPause)
            {
                this_thread::sleep_for(chrono::milliseconds(10));
                continue;
            }

            if (av_read_frame(container, &packet) < 0)
            {
                track.markAsInvalid();
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

        delete [] samples;

        return {};
    }

    void startPlayback(shared_ptr<Artist> artist, PlaybackOptions options)
    {
        list<shared_ptr<Track>> playbackList = artist->getTracks();

        if (options & PlaybackOption::shuffle)
        {
            vector<shared_ptr<Track>> temp(make_move_iterator(playbackList.begin()), make_move_iterator(playbackList.end()));
            playbackList.clear();

            random_shuffle(temp.begin(), temp.end());

            playbackList = { make_move_iterator(temp.begin()), make_move_iterator(temp.end()) };
        }

        if (playbackList.empty())
        {
            return;
        }

        sendPlaybackCommand(new CommandPLAY(move(playbackList), options));
    }

    void startPlayback(shared_ptr<Album> album, PlaybackOptions options)
    {
        list<shared_ptr<Track>> playbackList = album->getTracks();

        if (options & PlaybackOption::shuffle)
        {
            vector<shared_ptr<Track>> temp(make_move_iterator(playbackList.begin()), make_move_iterator(playbackList.end()));
            playbackList.clear();

            random_shuffle(temp.begin(), temp.end());

            playbackList = { make_move_iterator(temp.begin()), make_move_iterator(temp.end()) };
        }


        if (playbackList.empty())
        {
            return;
        }

        sendPlaybackCommand(new CommandPLAY(move(playbackList), options));
    }

    void startPlayback(shared_ptr<Track> track, PlaybackOptions options)
    {
        sendPlaybackCommand(new CommandPLAY({track}, options));
    }

    void startPlayback(shared_ptr<Playlist> playlist, PlaybackOptions options)
    {
        sendPlaybackCommand(new CommandPLAY(playlist->getTracks(), options));
    }

    unique_ptr<CommandPLAY> playbackThreadWait()
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
                            return unique_ptr<CommandPLAY>(dynamic_cast<CommandPLAY*>(command.release()));
                    case CommandType::exit:
                        return {};
                    default:
                        break;
                }
            }

            this_thread::sleep_for(chrono::milliseconds(500));
        }
    }

    void playbackThreadFunc()
    {
        deque<list<shared_ptr<Track>>> queued;
        stack<shared_ptr<Track>>       done;

        stack< tuple<
            deque<list<shared_ptr<Track>>>, // queued + currentList 0
            stack<shared_ptr<Track>>        // done                 1 
                >> suspended;

        while (true)
        {
            list<shared_ptr<Track>> currentList;
            if (!queued.empty())
            {
                currentList = move(queued.front());
                queued.pop_front();
            }
            else if (!suspended.empty())
            {
                queued = move(get<0>(suspended.top()));
                done.swap(get<1>(suspended.top()));
                suspended.pop();

                currentList = move(queued.front());
                queued.pop_front();
            }
            else
            {
                unique_ptr<CommandPLAY> commandPlay = playbackThreadWait();
                if (!commandPlay)
                {
                    break;
                }
                else
                {
                    currentList = move(commandPlay->tracks);
                }
            }

            shared_ptr<Track> currentTrack;
            data::OpenedTrack opened;
            while (!currentList.empty())
            {
                if (!opened.isValid())
                {
                    currentTrack = currentList.front();
                    currentList.pop_front();

                    opened = currentTrack->open();

                    if (!opened.isValid())
                    {
                        continue;
                    }
                }

                NowPlaying::track = currentTrack;
                unique_ptr<Command> command = playTrack(opened);
                if (!command)
                {
                    done.push(currentTrack);
                }
                else
                {
                    switch (command->type())
                    {
                        case CommandType::stopAll:
                            opened.markAsInvalid();
                            currentList.clear();
                            queued.clear();
                            while (!suspended.empty())
                            {
                                suspended.pop();
                            }
                            while (!done.empty())
                            {
                                done.pop();
                            }
                            NowPlaying::reset();
                            break;

                        case CommandType::stop:
                            opened.markAsInvalid();
                            currentList.clear();
                            queued.clear();
                            while (!done.empty())
                            {
                                done.pop();
                            }
                            NowPlaying::reset();

                            if (!suspended.empty())
                            {
                                queued = move(get<0>(suspended.top()));
                                done   = move(get<1>(suspended.top()));
                                suspended.pop();
                            }
                            break;

                        case CommandType::exit:
                            goto end;

                        case CommandType::next:
                            opened.markAsInvalid();
                            done.push(currentTrack);
                            if (currentList.empty())
                            {
                                if (!queued.empty())
                                {
                                    currentList = move(queued.front());
                                    queued.pop_front();
                                }
                                if (!suspended.empty())
                                {
                                    queued = move(get<0>(suspended.top()));
                                    done   = move(get<1>(suspended.top()));
                                    suspended.pop();
                                }
                            }
                            break;

                        case CommandType::previous:
                            opened.markAsInvalid();
                            currentList.push_front(currentTrack);
                            if (!done.empty())
                            {
                                currentList.push_front(done.top());
                                done.pop();
                            }
                            break;

                        case CommandType::play:
                            {
                                unique_ptr<CommandPLAY> commandPlay = unique_ptr<CommandPLAY>(dynamic_cast<CommandPLAY*>(command.release()));
                                if (commandPlay->options & PlaybackOption::stopCurrentPlayback)
                                {
                                    opened.markAsInvalid();
                                    currentList.clear();
                                    queued.clear();
                                    while (!done.empty())
                                    {
                                        done.pop();
                                    }
                                    while (!suspended.empty())
                                    {
                                        suspended.pop();
                                    }

                                    currentList = move(commandPlay->tracks);
                                }
                                else if (commandPlay->options & PlaybackOption::suspendCurrentPlayback)
                                {
                                    opened.markAsInvalid();
                                    currentList.push_front(currentTrack);
                                    queued.push_front(move(currentList));

                                    suspended.emplace(move(queued), move(done));

                                    decltype(done) temp{};
                                    done.swap(temp); // make it valid after move

                                    currentList = move(commandPlay->tracks);
                                }
                                else if (commandPlay->options & PlaybackOption::playAfterCurrentList)
                                {
                                    queued.push_front(move(commandPlay->tracks));
                                }
                                else if (commandPlay->options & PlaybackOption::playAfterCurrentTrack)
                                {
                                    currentList.splice(currentList.begin(), commandPlay->tracks, commandPlay->tracks.begin(), commandPlay->tracks.end());
                                }
                                else
                                {
                                    queued.push_back(move(commandPlay->tracks));
                                }
                                break;
                            }

                        default:
                            log("Playback thread received invalid command");
                            break;
                    }
                }
            }
        }
end:    NowPlaying::reset();
    }

    unique_ptr<Command> getPlaybackCommand()
    {
        if (!playbackControl.empty())
        {
            playbackControlMutex.lock();

            auto ret = move(playbackControl.front());
            playbackControl.pop();

            playbackControlMutex.unlock();
            return ret;
        }
        else
        {
            return {};
        }
    }

    void sendPlaybackCommand(Command* command)
    {
        playbackControlMutex.lock();
        playbackControl.emplace(command);
        playbackControlMutex.unlock();
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


    CommandPLAY::CommandPLAY(const list<shared_ptr<Track>>& tracks, PlaybackOptions options) :
        Command(CommandType::play),
        tracks(tracks.begin(), tracks.end()),
        options(options) {};


    void NowPlaying::reset()
    {
        track.reset();
        frame = 0;
        sample = 0;
    }
}

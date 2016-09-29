#include "play.hpp"
#include "log.hpp"

#include <iostream>
#include <algorithm>
#include <memory>
#include <iterator>
#include <thread>
#include <chrono>
#include <random>
#include <tuple>

#include <stack>
#include <deque>
#include <vector>

#include <limits>
#include <cstdint>

#include <fstream>

using namespace std;
using namespace chrono;

using data::Track;
using data::Album;
using data::Artist;

ofstream qd("queue");
                                

namespace playback
{
    shared_ptr<Track> NowPlaying::track;
    bool              NowPlaying::playing = false;
    gint64              NowPlaying::duration = 0;
    gint64              NowPlaying::current = 0;


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
        cout << "\033]0;" << track.parent->name << "\007\n";

        track.pipeline->set_state(Gst::STATE_PLAYING);

        while (true)
        {
            unique_ptr<Command> command = getPlaybackCommand();
            if (command)
            {
                switch (command->type())
                {
                    case CommandType::pause:
                        playbackPause = true;
                        track.pipeline->set_state(Gst::STATE_PAUSED);
                        break;
                    case CommandType::resume:
                        track.pipeline->set_state(Gst::STATE_PLAYING);
                        playbackPause = false;
                        break;
                    case CommandType::toggle:
                        playbackPause = !playbackPause;
                        if (playbackPause)
                        {
                            track.pipeline->set_state(Gst::STATE_PAUSED);
                        }
                        else
                        {
                            track.pipeline->set_state(Gst::STATE_PLAYING);
                        }
                        break;

                    default:
                        track.pipeline->set_state(Gst::STATE_PAUSED);
                        cout << "\033]0;" << "player" << "\007\n";
                        return command;
                }
            }

            if (track.pipeline->get_bus()->poll(Gst::MESSAGE_EOS, 0))
            {
                track.markAsInvalid();
                cout << "\033]0;" << "player" << "\007\n";
                return {};
            }

            track.pipeline->query_position(Gst::FORMAT_TIME, NowPlaying::current);
            track.pipeline->query_duration(Gst::FORMAT_TIME, NowPlaying::duration);

            this_thread::sleep_for(50ms);
        }
    }

    void startPlayback(shared_ptr<Artist> artist, PlaybackOptions options)
    {
        list<shared_ptr<Track>> playbackList = artist->getTracks();

        if (options & PlaybackOption::shuffle)
        {
            vector<shared_ptr<Track>> temp(make_move_iterator(playbackList.begin()), make_move_iterator(playbackList.end()));
            playbackList.clear();

            shuffle(temp.begin(), temp.end(), default_random_engine(system_clock::now().time_since_epoch().count()));

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

            shuffle(temp.begin(), temp.end(), default_random_engine(system_clock::now().time_since_epoch().count()));

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
            while (!currentList.empty() || opened.isValid())
            {
                if (!opened.isValid())
                {
                    currentTrack = currentList.front();
                    currentList.pop_front();

                    opened = currentTrack->open();

                    if (!opened.isValid())
                    {
                        log(LT::error, "Could not open track %s") % currentTrack->name;
                        continue;
                    }
                }

                NowPlaying::track = currentTrack;
                NowPlaying::playing = true;
                playbackPause = false;
                unique_ptr<Command> command = playTrack(opened);
                NowPlaying::reset();
                if (!command)
                {
                    done.push(currentTrack);
                }
                else
                {
                    switch (command->type())
                    {
                        case CommandType::dumpQueue:
                            {
                                qd << "currentTrack: " << currentTrack->name << endl;

                                qd << "currentList:" << endl;
                                for (auto& e : currentList)
                                {
                                    qd << '\t' << e->name << endl;
                                }
                                    
                                qd << "queued:" << endl;
                                for (int i = 0; i < queued.size(); i++)
                                {
                                    qd << '\t' << i << ':' << endl;
                                    for (auto& e : queued[i])
                                    {
                                        qd << "\t\t" << e->name << endl;
                                    }
                                }
                                qd << "---------------------" << endl;
                            }
                            break;
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
                            break;

                        case CommandType::stop:
                            opened.markAsInvalid();
                            currentList.clear();
                            queued.clear();
                            while (!done.empty())
                            {
                                done.pop();
                            }

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
                                else if (commandPlay->options & PlaybackOption::playAfterEverything)
                                {
                                    queued.push_back(move(commandPlay->tracks));
                                }
                                else
                                {
                                    log(LT::error, "No playback option");
                                }
                            }
                            break;

                        default:
                            log(LT::error, "Playback thread received invalid command");
                            break;
                    }
                }
            }
        }
end:;
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
        duration = 0;
        current = 0;
    }
}

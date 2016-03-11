#pragma once

#include "data.hpp"
#include "decode.hpp"
#include "playlist.hpp"
#include "options.hpp"

#include <ao/ao.h>
#include <queue>
#include <deque>
#include <memory>
#include <mutex>
#include <initializer_list>


class Command;
class CommandPAUSE;
class CommandRESUME;
class CommandTOGGLE;
class CommandSTOP;
class CommandPREVIOUS;
class CommandNEXT;
class CommandEXIT;
class CommandPLAY;

enum class PlaybackOption
{
    shuffle,
    queue
};

using PlaybackOptions = Options<PlaybackOption>;

namespace play
{
    extern std::mutex playbackControlMutex;

    /*
       commands are processed in two functions: 
       playbackThreadWait() and processPlaybackCommand()

       playbackThreadWait() processes everything in place, when
       processPlaybackCommand() can throw commandID (mainly of commands
       that stop current playback) so that it could be processed by
       upper(?) function, which is playbackThread()
       */
    extern std::queue<std::unique_ptr<Command>>     playbackControl;
    extern std::deque<std::unique_ptr<CommandPLAY>> playbackQueue;
    extern bool playbackPause;
    extern std::unique_ptr<std::thread> playback;

    namespace NowPlaying
    {
	extern std::shared_ptr<Track> track;
	extern size_t frame;
	extern size_t sample;
	extern bool playing;

	void reset();
    }
}

void initPlay();
void endPlay();
void playTrack(std::shared_ptr<Track> track);
void startPlayback(std::shared_ptr<Artist> artist, PlaybackOptions options);
void startPlayback(std::shared_ptr<Album> album, PlaybackOptions options);
void startPlayback(std::shared_ptr<Track> track, PlaybackOptions options );
void startPlayback(std::shared_ptr<Playlist> playlist, PlaybackOptions options);
std::unique_ptr<std::deque<std::shared_ptr<Track>>> playbackThreadWait();
void playbackThread();
void playPacket(AVPacket* packet, ao_device* device, std::shared_ptr<Track> track);
void processPlaybackCommand();
void sendPlaybackCommand(Command* command);
bool playbackInProcess();


enum class CommandType
{
    pause,
    resume,
    toggle,
    stop,
    previous,
    next,
    exit,
    play
};

// SIMPLE COMMANDS
class Command
{
    const CommandType commandType;

    protected:
    Command(CommandType commandType);

    public:

    CommandType type() const;
    virtual ~Command();
};

class CommandPAUSE : public Command
{
    public:
	CommandPAUSE() : Command(CommandType::pause) {};
};

class CommandRESUME : public Command
{
    public:
	CommandRESUME() : Command(CommandType::resume) {};
};

class CommandTOGGLE : public Command
{
    public:
	CommandTOGGLE() : Command(CommandType::toggle) {};
};

class CommandSTOP : public Command
{
    public:
	CommandSTOP() : Command(CommandType::stop) {};
};

class CommandPREVIOUS : public Command
{
    public:
	CommandPREVIOUS() : Command(CommandType::previous) {};
};

class CommandNEXT : public Command
{
    public:
	CommandNEXT() : Command(CommandType::next) {};
};

class CommandEXIT : public Command
{
    public:
	CommandEXIT() : Command(CommandType::exit) {};
};

// PLAYBACK COMMAND
class CommandPLAY : public Command
{
    public:
        CommandPLAY(const std::deque<std::shared_ptr<Track>>& tracks, PlaybackOptions options);

	std::unique_ptr<std::deque<std::shared_ptr<Track>>> tracks;
	PlaybackOptions options;
};

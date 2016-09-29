#pragma once

#include "data.hpp"
#include "playlist.hpp"
#include "options.hpp"

#include <queue>
#include <list>
#include <memory>
#include <mutex>
#include <initializer_list>

namespace playback
{
    class Command;
    class CommandPAUSE;
    class CommandRESUME;
    class CommandTOGGLE;
    class CommandSTOP;
    class CommandSTOPALL;
    class CommandPREVIOUS;
    class CommandNEXT;
    class CommandEXIT;
    class CommandPLAY;

    enum class PlaybackOption
    {
        shuffle,
        stopCurrentPlayback,
        suspendCurrentPlayback,
        playAfterCurrentList,
        playAfterCurrentTrack,
        playAfterEverything
    };

    using PlaybackOptions = Options<PlaybackOption>;

    /*
       commands are processed in two functions: 
       playbackThreadWait() and processPlaybackCommand()

       playbackThreadWait() processes everything in place, when
       processPlaybackCommand() can throw commandID (mainly of commands
       that stop current playback) so that it could be processed by
       upper(?) function, which is playbackThread()
       */
    extern std::queue<std::unique_ptr<Command>>     playbackControl;
    extern std::mutex playbackControlMutex;
    extern bool playbackPause;
    extern std::thread playbackThread;

    namespace NowPlaying
    {
        extern std::shared_ptr<data::Track> track;
        extern bool playing;
        extern gint64 duration;
        extern gint64 current;

        void reset();
    }


    void init();
    void end();
    std::unique_ptr<Command> playTrack(data::OpenedTrack& track);
    void startPlayback(std::shared_ptr<data::Artist> artist, PlaybackOptions options);
    void startPlayback(std::shared_ptr<data::Album> album, PlaybackOptions options);
    void startPlayback(std::shared_ptr<data::Track> track, PlaybackOptions options );
    void startPlayback(std::shared_ptr<Playlist> playlist, PlaybackOptions options);
    std::unique_ptr<CommandPLAY> playbackThreadWait();
    void playbackThreadFunc();
    std::unique_ptr<Command> getPlaybackCommand();
    void sendPlaybackCommand(Command* command);
    bool playbackInProcess();


    enum class CommandType
    {
        pause,
        resume,
        toggle,
        stop,
        stopAll,
        previous,
        next,
        exit,
        play,
        dumpQueue
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

    class CommandSTOPALL : public Command
    {
        public:
            CommandSTOPALL() : Command(CommandType::stopAll) {};
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

    class CommandDQ : public Command
    {
        public:
            CommandDQ() : Command(CommandType::dumpQueue) {};
    };

    // PLAYBACK COMMAND
    class CommandPLAY : public Command
    {
        public:
            CommandPLAY(const std::list<std::shared_ptr<data::Track>>& tracks, PlaybackOptions options); //TODO: remove const reference so that it is possible to use move

            std::list<std::shared_ptr<data::Track>> tracks;
            PlaybackOptions options;
    };
}

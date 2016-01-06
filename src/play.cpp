#include "play.hpp"

#include <unistd.h>
#include <algorithm>
#include <memory>

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
   //av_dump_format(track->container, 1, track->filePath.c_str(), 0);
   track->open();
   NowPlaying::track = track;
   NowPlaying::frame = 0;
   
   unique_ptr<ao_device, decltype(&ao_close)> device(ao_open_live(ao_default_driver_id(), track->sampleFormat, nullptr), ao_close);
   if (device.get() == nullptr)
   {
      printf("Could not open device\n");
      exit(0);
   }

   bool endOfStream = false;
   while (!endOfStream)
   {
      processPlaybackCommand();
      
      if (playbackPause)
      {
	 usleep(1000);
	 continue;
      }
      
      unique_ptr<AVPacket, void(*)(AVPacket*)> packet(new AVPacket,
						      [](AVPacket* packet)
						      {
							 av_packet_unref(packet);
							 av_free_packet(packet);
							 delete packet;
						      });
      av_init_packet(packet.get());
      if (av_read_frame(track->container, packet.get()) < 0)
      {
	 endOfStream = true;
	 continue;
      }
      if (packet->stream_index != track->streamID)
      {
	 //printf("Packet from invalid stream\n");
	 continue;
      }

      playPacket(packet.get(), device.get(), track);
   }

   playbackPause = false;
   track->close();
   NowPlaying::reset();
}

void startPlayback(shared_ptr<Artist> artist, uint_16 options)
{
   unique_ptr<deque<shared_ptr<Track>>> playbackDeque(new deque<shared_ptr<Track>>);
   for (auto album : artist->albumsDeque)
   {
      if (album != artist->albumsMap["all"])
      {
	 playbackDeque->insert(playbackDeque->end(), album->tracksDeque.begin(), album->tracksDeque.end());
      }
   }
   if (options & PLAYBACK_OPTION_SHUFFLE)
   {
      random_shuffle(playbackDeque->begin(), playbackDeque->end());
   }

   if (playbackDeque->empty())
   {
      return;
   }
   sendPlaybackCommand(new Command(PLAYBACK_COMMAND_STOP));
   sendPlaybackCommand(new PlayCommand(move(playbackDeque), 0));
}

void startPlayback(shared_ptr<Album> album, uint_16 options)
{
   unique_ptr<deque<shared_ptr<Track>>> playbackDeque(new deque<shared_ptr<Track>>);
   playbackDeque->insert(playbackDeque->end(), album->tracksDeque.begin(), album->tracksDeque.end());
   
   if (options & PLAYBACK_OPTION_SHUFFLE)
   {
      random_shuffle(playbackDeque->begin(), playbackDeque->end());
   }

   if (playbackDeque->empty())
   {
      return;
   }
   sendPlaybackCommand(new Command(PLAYBACK_COMMAND_STOP));
   sendPlaybackCommand(new PlayCommand(move(playbackDeque), 0));
}

void startPlayback(shared_ptr<Track> track, uint_16 options)
{
   auto initList = {track};
   sendPlaybackCommand(new Command(PLAYBACK_COMMAND_STOP));
   sendPlaybackCommand(new PlayCommand(make_unique<deque<shared_ptr<Track>>>(initList), 0));
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
	 
	 unique_ptr<PlayCommand> playCommand;
	 switch (command->commandID)
	 {
	    case PLAYBACK_COMMAND_PLAY: //TODO: provide reliable way to make sure command is actually PlayCommand in this case
	       playCommand.reset(dynamic_cast<PlayCommand*>(command.release()));
	       return move(playCommand->tracks);
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
      for (auto track = tracks->begin(); track != tracks->end();)
      {
	 try
	 {
	    playTrack(*track);
	 }
	 catch (uint_8 e)
	 {
	    switch (e)
	    {
	       case PLAYBACK_COMMAND_STOP:
		  goto iterationEnd;
	       case PLAYBACK_COMMAND_NEXT:
		  track++;
		  continue;
	       case PLAYBACK_COMMAND_PREV:
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
     iterationEnd:;
      NowPlaying::playing = false;
      NowPlaying::reset();
   }
  end:;
   NowPlaying::playing = false;
   NowPlaying::reset();
}

void playPacket(AVPacket* packet, ao_device* device, shared_ptr<Track> track)
{
   AVFrame* frame;
   frame = decodeFrame(track, track->sampleFormat, packet);
   if (frame == nullptr)
   {
      return;
   }
      
   int dataSize =
      av_samples_get_buffer_size(nullptr,
				 track->codecContext->channels,
				 frame->nb_samples,
				 track->codecContext->sample_fmt,
				 1);
   for (int i = 0; i < track->sampleFormat->channels; i++)
   {
      ao_play(device, reinterpret_cast<char*>(frame->extended_data[i]), frame->linesize[i]);
   }
   NowPlaying::frame++;
   
   av_frame_unref(frame);
   av_frame_free(&frame);
   delete frame;
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


PlayCommand::PlayCommand(unique_ptr<deque<shared_ptr<Track>>>&& tracks, uint_16 options) :
   Command(PLAYBACK_COMMAND_PLAY), tracks(move(tracks)), options(options) {}


void NowPlaying::reset()
{
   track.reset();
   frame = 0;
}

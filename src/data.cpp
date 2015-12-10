#include "data.h"
#include "decode.h"

#include <algorithm>
#include <exception>
#include <string>
#include <stdio.h>

std::map<std::string, Artist*> artistsMap;
std::deque<Artist*> artistsDeque;
std::deque<Track*> testTracks;

void initData()
{
   Artist* temp;
   temp = new Artist("all");
   artistsDeque.push_back(temp);
   artistsMap["all"] = temp;

   temp = new Artist("unknown");
   artistsDeque.push_back(temp);
   artistsMap["unknown"] = temp;
}

void addTrack(Track* track)
{
   if (artistsMap.find(track->artistName) == artistsMap.end())
   {
      Artist* temp = new Artist(track->artistName);
      artistsDeque.push_back(temp);
      artistsMap[track->artistName] = temp;
   }

   artistsMap["all"]->addTrack(track);
   artistsMap[track->artistName]->addTrack(track);   
}

std::deque<Track*>* getTracks()
{
   std::deque<Track*>* tracks = new std::deque<Track*>;
   for (auto artist : artistsDeque)
   {
      for (auto album : artist->albumsDeque)
      {
	 tracks->insert(tracks->end(), album->tracksDeque.begin(), album->tracksDeque.end());
      }
   }
   return tracks;
}

std::deque<Album*>* getAlbums(bool includeUnknown)
{
   std::deque<Album*>* albums = new std::deque<Album*>;
   for (auto artist : artistsDeque)
   {
      albums->insert(albums->end(), artist->albumsDeque.begin(), artist->albumsDeque.end());
   }

   return albums;
}

Track::Track(const std::string& file)
{
   filePath = file;
   container = avformat_alloc_context();
   codecContext = NULL;
   codec = NULL;

   open();
   decodeMetadata();
   sampleFormat = getSampleFormat(this);
   close();
}

Track::~Track()
{
   delete sampleFormat;
   delete codecContext;
   delete container;
}

void Track::open()
{
   container = avformat_alloc_context();

   if (avformat_open_input(&container, filePath.c_str(), NULL, NULL) < 0)
   {
      throw std::invalid_argument(
	 std::string("Could not open file: ") + filePath.c_str()
	 );
   }
   if (avformat_find_stream_info(container, NULL) < 0)
   {
      throw std::logic_error(
	 std::string("Could not find stream data: ") + filePath.c_str()
	 );
   }
   
   streamID = -1;
   for (int i = 0; i < container->nb_streams; i++)
   {
      if (container->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
      {
	 streamID = i;
	 break;
      }
   }
   if (streamID == -1)
   {
      throw std::logic_error(
	 std::string("No audio stream: ") + filePath.c_str()
	 );
   }

   codecContext = container->streams[streamID]->codec;
   codec = avcodec_find_decoder(codecContext->codec_id);
   if (codec == NULL)
   {
      throw std::logic_error(
	 std::string("Could not find codec: ") + filePath.c_str()
	 );
   }
   if (avcodec_open2(codecContext, codec, NULL) < 0)
   {
      throw std::logic_error(
	 std::string("Could not open codec: ") + filePath.c_str()
	 );
   }
}

void Track::close()
{
   //WARNING: this might cause memory leak
   
   // if (codec != NULL)
   // {
   //    delete codec;
   //    codec = NULL;
   // }
   // if (codecContext != NULL)
   // {
   //    delete codecContext;
   //    codecContext = NULL;
   // }
   
   avformat_close_input(&container);
}

void Track::decodeMetadata()
{
   AVDictionaryEntry* title = av_dict_get(container->metadata, "title", NULL, 0);
   if (title == NULL)
   {
      name = filePath;
   }
   else
   {
      name = title->value;
   }
   AVDictionaryEntry* artist = av_dict_get(container->metadata, "artist", NULL, 0);
   if (artist == NULL)
   {
      artistName = "unknown";
   }
   else
   {
      artistName = artist->value;
   }
   AVDictionaryEntry* album = av_dict_get(container->metadata, "album", NULL, 0);
   if (album == NULL)
   {
      albumName = "unknown";
   }
   else
   {
      albumName = album->value;
   }
}

void Track::testPrint()
{
   printf("\t\t%s: %s (%s)\n", artistName.c_str(), name.c_str(), albumName.c_str());
}

Album::Album(const std::string& name) :
   name(name) {};

Album::~Album()
{
   for (auto track : tracksDeque)
   {
      delete track;
   }
   tracksDeque.clear();
   tracksMap.clear();
}

std::deque<Track*>* Album::getTracks()
{
   std::deque<Track*>* ret = new std::deque<Track*>;
   *ret = tracksDeque;
   return ret;
}

void Album::addTrack(Track* track)
{
   if (tracksMap.find(track->name) != tracksMap.end())
      // if (std::find(tracksMap.begin(), tracksMap.end(), track->name) != tracksMap.end())
   {
      track->name = track->name + "_";
      addTrack(track);
   }
   else
   {
      tracksMap[track->name] = track;
      tracksDeque.push_back(track);
   }
}

void Album::testPrint()
{
   printf("\tStarting to print album %s\n", name.c_str());
   for (auto track : tracksDeque)
   {
      track->testPrint();
   }
   printf("\tDone printing album %s\n", name.c_str());
}

Artist::Artist(const std::string& name) :
   name(name)
{
   Album* temp;
   temp = new Album(this->name + ": all");
   albumsDeque.push_back(temp);
   albumsMap["all"] = temp;

   temp = new Album(this->name + ": unknown");
   albumsDeque.push_back(temp);
   albumsMap["unknown"] = temp;
}

Artist::~Artist()
{
   for (auto album : albumsDeque)
   {
      delete album;
   }
   albumsDeque.clear();
   albumsMap.clear();
   
}

std::deque<Album*>* Artist::getAlbums()
{
   std::deque<Album*>* ret = new std::deque<Album*>;
   *ret = albumsDeque;
   return ret;
}

void Artist::addAlbum(Album* album)
{
   albumsDeque.push_back(album); //TODO: this needs to check if such album already exists
   albumsMap[album->name] = album;
}

void Artist::addTrack(Track* track)
{
   if (albumsMap.find(track->albumName) == albumsMap.end())
   {
      Album* temp = new Album(track->albumName);
      albumsDeque.push_back(temp);
      albumsMap[track->albumName] = temp;
   }
   albumsMap["all"]->addTrack(track);
   albumsMap[track->albumName]->addTrack(track);
}

void Artist::testPrint()
{
   printf("Starting to print artist %s\n", name.c_str());
   for (auto album : albumsDeque)
   {
      album->testPrint();
   }
   printf("Done printing artist %s\n\n", name.c_str());
}

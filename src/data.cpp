#include "data.hpp"
#include "decode.hpp"

#include <algorithm>
#include <exception>
#include <memory>
#include <string>
#include <stdio.h>

using namespace std;

map<string, shared_ptr<Artist> > artistsMap;
deque<shared_ptr<Artist> > artistsDeque;
deque<shared_ptr<Track> > testTracks;

void initData()
{
   shared_ptr<Artist> temp(new Artist("all"));
   ::artistsDeque.push_back(temp);
   ::artistsMap["all"] = temp;

   temp.reset(new Artist("unknown"));
   ::artistsDeque.push_back(temp);
   ::artistsMap["unknown"] = temp;
}

void addTrack(shared_ptr<Track> track)
{
   if (::artistsMap.find(track->artistName) == ::artistsMap.end())
   {
      shared_ptr<Artist> temp(new Artist(track->artistName));
      ::artistsDeque.push_back(temp);
      ::artistsMap[track->artistName] = temp;
   }

   ::artistsMap["all"]->addTrack(track);
   ::artistsMap[track->artistName]->addTrack(track);
}

deque<shared_ptr<Track> >* getTracks()
{
   deque<shared_ptr<Track> >* tracks = new deque<shared_ptr<Track> >;
   for (auto artist : ::artistsDeque)
   {
      for (auto album : artist->albumsDeque)
      {
	 tracks->insert(tracks->end(), album->tracksDeque.begin(), album->tracksDeque.end());
      }
   }
   
   return tracks;
}

deque<shared_ptr<Album> >* getAlbums(bool includeUnknown)
{
   deque<shared_ptr<Album> >* albums = new deque<shared_ptr<Album> >;
   for (auto artist : ::artistsDeque)
   {
      albums->insert(albums->end(), artist->albumsDeque.begin(), artist->albumsDeque.end());
   }
   
   return albums;
}

Track::Track(const string& file)
{
   filePath = file;
   container = avformat_alloc_context();
   codecContext = nullptr;
   codec = nullptr;

   open();
   decodeMetadata();
   sampleFormat = getSampleFormat(this);
   close();
}

Track::~Track()
{
   close();
   delete sampleFormat;
   delete container;
}

void Track::open()
{
   container = avformat_alloc_context();

   if (avformat_open_input(&container, filePath.c_str(), nullptr, nullptr) < 0)
   {
      throw invalid_argument(
	 string("Could not open file: ") + filePath.c_str()
	 );
   }
   if (avformat_find_stream_info(container, nullptr) < 0)
   {
      throw logic_error(
	 string("Could not find stream data: ") + filePath.c_str()
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
      throw logic_error(
	 string("No audio stream: ") + filePath.c_str()
	 );
   }

   codecContext = container->streams[streamID]->codec;
   codec = avcodec_find_decoder(codecContext->codec_id);
   if (codec == nullptr)
   {
      throw logic_error(
	 string("Could not find codec: ") + filePath.c_str()
	 );
   }
   if (avcodec_open2(codecContext, codec, nullptr) < 0)
   {
      throw logic_error(
	 string("Could not open codec: ") + filePath.c_str()
	 );
   }
}

void Track::close()
{
   //WARNING: this might cause memory leak
   
   // if (codec != nullptr)
   // {
   //    delete codec;
   //    codec = nullptr;
   // }
   // if (codecContext != nullptr)
   // {
   //    delete codecContext;
   //    codecContext = nullptr;
   // }
   if (container != NULL)
   {
      avformat_close_input(&container);
      container = NULL;
   }
}

void Track::decodeMetadata()
{
   AVDictionaryEntry* title = av_dict_get(container->metadata, "title", nullptr, 0);
   if (title == nullptr)
   {
      name = filePath;
   }
   else
   {
      name = title->value;
   }
   AVDictionaryEntry* artist = av_dict_get(container->metadata, "artist", nullptr, 0);
   if (artist == nullptr)
   {
      artistName = "unknown";
   }
   else
   {
      artistName = artist->value;
   }
   AVDictionaryEntry* album = av_dict_get(container->metadata, "album", nullptr, 0);
   if (album == nullptr)
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

Album::Album(const string& name) :
   name(name) {};

Album::~Album()
{
   tracksDeque.clear();
   tracksMap.clear();
}

deque<shared_ptr<Track> >* Album::getTracks()
{
   deque<shared_ptr<Track> >* ret = new deque<shared_ptr<Track> >;
   *ret = tracksDeque;
   return ret;
}

void Album::addTrack(shared_ptr<Track> track)
{
   if (tracksMap.find(track->name) != tracksMap.end())
      // if (find(tracksMap.begin(), tracksMap.end(), track->name) != tracksMap.end())
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

Artist::Artist(const string& name) :
   name(name)
{
   shared_ptr<Album> temp(new Album(this->name + ": all"));
   albumsDeque.push_back(temp);
   albumsMap["all"] = temp;

   temp.reset(new Album(this->name + ": unknown"));
   albumsDeque.push_back(temp);
   albumsMap["unknown"] = temp;
}

Artist::~Artist()
{
   albumsDeque.clear();
   albumsMap.clear();  
}

deque<shared_ptr<Album> >* Artist::getAlbums()
{
   deque<shared_ptr<Album> >* ret = new deque<shared_ptr<Album> >;
   *ret = albumsDeque;
   return ret;
}

void Artist::addAlbum(shared_ptr<Album> album)
{
   albumsDeque.push_back(album); //TODO: this needs to check if such album already exists
   albumsMap[album->name] = album;
}

void Artist::addTrack(shared_ptr<Track> track)
{
   if (albumsMap.find(track->albumName) == albumsMap.end())
   {
      shared_ptr<Album> temp(new Album(track->albumName));
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

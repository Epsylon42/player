#include "data.h"
#include "decode.h"

#include <exception>
#include <string>
#include <stdio.h>

std::map<std::string, Artist*> artists;

void initData()
{

}

void addTrack(Track* track)
{
   if (artists[track->artistName] == NULL)
   {
      artists[track->artistName] = new Artist(track->artistName);
   }
   artists[track->artistName]->addTrack(track);
}

std::list<Album*>* getUnknownAlbums()
{
   std::list<Album*>* ret = new std::list<Album*>();

   for (auto artistPair : artists)
   {
      Album* album = artistPair.second->albums["unknown"];
      if (album != NULL)
      {
	 ret->push_back(album);
      }
   }
   
   return ret;
}

Track::Track(const std::string& file)
{
   filePath = file;
   container = avformat_alloc_context();
   codecContext = NULL;
   codec = NULL;
   // filePath = NULL;
   // name = NULL;
   // artistName = NULL;
   // albumName = NULL;
   if (avformat_open_input(&container, file.c_str(), NULL, NULL) < 0)
   {
      printf("Could not open file: %s\n", file.c_str());
      exit(0);
   }
   
   if (avformat_find_stream_info(container, NULL) < 0)
   {
      printf("Could not find stream data: %s\n", file.c_str());
      exit(0);
   }
   
   AVDictionaryEntry* title = av_dict_get(container->metadata, "title", NULL, 0);
   if (title == NULL)
   {
      name = "untitled";
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
      printf("No audio stream: %s\n", file.c_str());
      exit(0);
   }

   codecContext = container->streams[streamID]->codec;
   codec = avcodec_find_decoder(codecContext->codec_id);
   if (codec == NULL)
   {
      printf("Could not find decoder: %s\n", file.c_str());
      exit(0);
   }
   if (avcodec_open2(codecContext, codec, NULL) < 0)
   {
      printf("Could not find codec\n");
      exit(0);
   }

   sampleFormat = getSampleFormat(this);
}

Track::~Track()
{
   delete sampleFormat;
   delete codecContext;
   delete container;
}

void Track::testPrint()
{
   printf("\t\t%s: %s (%s)\n", artistName.c_str(), name.c_str(), albumName.c_str());
}


Album::Album(const std::string& _name, const std::string& _artistName)
{
   name = _name;
   artistName = _artistName;
}

Album::Album(const std::string& _name, Artist* artist)
{
   name = _name;
   artistName = artist->name;
}

Album::~Album()
{
   for (auto trackPair : tracks)
   {
      delete trackPair.second;
   }
   tracks.clear();
}

int Album::addTrack(Track* track)
{
   if (track->artistName != artistName)
   {
      printf("Artist name of track and album do not match: '%s' and '%s'\n",
	     track->artistName.c_str(),
	     artistName.c_str());
      return -1;
   }
   
   tracks[track->name] = track;

   return 0;
}

void Album::testPrint()
{
   printf("\tStarting to print album %s\n", name.c_str());
   for (auto trackPair : tracks)
   {
      trackPair.second->testPrint();
   }
   printf("\tDone printing album %s\n", name.c_str());
}


Artist::Artist(const std::string& _name)
{
   name = _name;
}

Artist::~Artist()
{
   for (auto albumPair : albums)
   {
      delete albumPair.second;
   }
   albums.clear();
}

int Artist::addAlbum(Album* album)
{
   if (album->artistName != name)
   {
      printf("Artist name do not match with album: '%s' and '%s'\n",
	     album->artistName.c_str(),
	     name.c_str());
      return -1;
   }

   albums[album->name] = album;
}

int Artist::addTrack(Track* track)
{
   if (track->artistName != name)
   {
      printf("Artist name do not match with track: '%s' and '%s'\n",
	     track->artistName.c_str(),
	     name.c_str());
      return -1;
   }
   
   if (albums[track->albumName] == NULL)
   {
      albums[track->albumName] = new Album(track->albumName, this);
   }
   return albums[track->albumName]->addTrack(track);
}

void Artist::testPrint()
{
   printf("Starting to print artist %s\n", name.c_str());
   for (auto albumPair : albums)
   {
      albumPair.second->testPrint();
   }
   printf("Done printing artist %s\n\n", name.c_str());
}

std::list<Album*>* Artist::getAlbums(bool includeUnknown)
{
   std::list<Album*>* ret = new std::list<Album*>();

   for (auto albumPair : albums)
   {
      Album* album = albumPair.second;
      if (album->name != "unknown" || includeUnknown)
      {
	 ret->push_back(album);
      }
   }
   
   return ret;
}

#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

#include <ao/ao.h>
#include <string>
#include <map>
#include <list>

struct Track;
struct Album;
struct Artist;

extern std::map<std::string, Artist*> artists;

void initData();

void addTrack(Track* track);

std::list<Album*>* getUnknownAlbums();

struct Track
{
   std::string filePath;
   AVFormatContext* container;
   AVCodecContext* codecContext;
   AVCodec* codec;
   ao_sample_format* sampleFormat;
   int streamID;

   std::string name;
   std::string artistName;
   std::string albumName;

   Track(const std::string& file);
   ~Track();
   void testPrint();
};

struct Album
{
   std::string name;
   std::string artistName;
   
   std::map<std::string, Track*> tracks;

   Album(const std::string& _name, const std::string& _artistName);
   Album(const std::string& _name, Artist* artist);
   ~Album();
   int addTrack(Track* track);
   void testPrint();
};

struct Artist
{
   std::string name;

   std::map<std::string, Album*> albums;

   Artist(const std::string& _name);
   ~Artist();
   int addAlbum(Album* album);
   int addTrack(Track* track);
   void testPrint();
   std::list<Album*>* getAlbums(bool includeUnknown = true);
};

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
#include <deque>

struct Track;
struct Album;
struct Artist;

extern std::map<std::string, Artist*> artistsMap;
extern std::deque<Artist*> artistsDeque;

void initData();

void addTrack(Track* track);
std::deque<Track*>* getTracks();
std::deque<Album*>* getAlbums(bool includeUnknown = true);
Album* globalAlbum();

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
   void open();
   void close();
   void decodeMetadata();
   void testPrint();
};

struct Album
{
   std::string name;

   std::map<std::string, Track*> tracksMap;
   std::deque<Track*> tracksDeque;

   Album(const std::string& name);
   ~Album();
   std::deque<Track*>* getTracks();
   void addTrack(Track* track);
   void testPrint();
};

struct Artist
{
   std::string name;

   std::map<std::string, Album*> albumsMap;
   std::deque<Album*> albumsDeque;

   Artist(const std::string& name);
   ~Artist();
   std::deque<Album*>* getAlbums();
   void addAlbum(Album* album);
   void addTrack(Track* track);
   void testPrint();
};

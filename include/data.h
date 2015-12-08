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

extern std::map<std::string, int> artistIndexes;
extern std::deque<Artist*> artists;
extern std::deque<Track*> testTracks;

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
   void testPrint();
};

struct Album
{
   std::string name;
   int artistIndex;

   std::map<std::string, int> trackIndexes;
   std::deque<Track*> tracks;

   Album(const std::string& name, int artistIndex);
   Album(const std::string& name, const std::string& artistName);
   Album(const std::string& name, Artist* artist);
   ~Album();
   std::deque<Track*>* getTracks();
   void addTrack(Track* track);
   void testPrint();
};

struct Artist
{
   std::string name;

   std::map<std::string, int> albumIndexes;
   std::deque<Album*> albums;

   Artist(const std::string& name);
   ~Artist();
   std::deque<Album*>* getAlbums();
   void addAlbum(Album* album);
   void addTrack(Track* track);
   void testPrint();
};

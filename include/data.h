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
#include <vector>

struct Track;
struct Album;
struct Artist;

extern std::map<std::string, int> artistIndexes;
extern std::vector<Artist*> artists;
extern std::vector<Track*> testTracks;

void initData();

void addTrack(Track* track);
std::vector<Track*>* getTracks();
std::vector<Album*>* getAlbums(bool includeUnknown = true);
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
   std::vector<Track*> tracks;

   Album(const std::string& name, int artistIndex);
   Album(const std::string& name, const std::string& artistName);
   Album(const std::string& name, Artist* artist);
   ~Album();
   std::vector<Track*>* getTracks();
   void addTrack(Track* track);
   void testPrint();
};

struct Artist
{
   std::string name;

   std::map<std::string, int> albumIndexes;
   std::vector<Album*> albums;

   Artist(const std::string& name);
   ~Artist();
   std::vector<Album*>* getAlbums();
   void addAlbum(Album* album);
   void addTrack(Track* track);
   void testPrint();
};

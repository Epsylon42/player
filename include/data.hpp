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
#include <thread>
#include <memory>

struct Track;
struct Album;
struct Artist;

namespace data
{
   extern std::map<std::string, std::shared_ptr<Artist>> artistsMap;
   extern std::deque<std::shared_ptr<Artist>>            artistsDeque;
}

void initData();

void addTrack(std::shared_ptr<Track> track);
std::deque<std::shared_ptr<Track>>* getTracks();
std::deque<std::shared_ptr<Album>>* getAlbums(bool includeUnknown = true);

struct Track
{
   std::string       filePath;
   AVFormatContext*  container;
   AVCodecContext*   codecContext;
   AVCodec* codec;
   ao_sample_format* sampleFormat;
   int               streamID;

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

   std::map<std::string, std::shared_ptr<Track>> tracksMap;
   std::deque<std::shared_ptr<Track>>            tracksDeque;

   Album(const std::string& name);
   ~Album();
   std::deque<std::shared_ptr<Track>>* getTracks();
   void addTrack(std::shared_ptr<Track> track);
   void testPrint();
};

struct Artist
{
   std::string name;

   std::map<std::string, std::shared_ptr<Album>> albumsMap;
   std::deque<std::shared_ptr<Album>>            albumsDeque;

   Artist(const std::string& name);
   ~Artist();
   std::deque<std::shared_ptr<Album>>* getAlbums();
   void addAlbum(std::shared_ptr<Album> album);
   void addTrack(std::shared_ptr<Track> track);
   void testPrint();
};

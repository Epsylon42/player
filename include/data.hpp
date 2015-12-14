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

using namespace std;

struct Track;
struct Album;
struct Artist;

extern map<string, shared_ptr<Artist> > artistsMap;
extern deque<shared_ptr<Artist> > artistsDeque;

void initData();

void addTrack(shared_ptr<Track> track);
deque<shared_ptr<Track> >* getTracks();
deque<shared_ptr<Album> >* getAlbums(bool includeUnknown = true);

struct Track
{
   string filePath;
   AVFormatContext* container;
   AVCodecContext* codecContext;
   AVCodec* codec;
   ao_sample_format* sampleFormat;
   int streamID;

   string name;
   string artistName;
   string albumName;

   Track(const string& file);
   ~Track();
   void open();
   void close();
   void decodeMetadata();
   void testPrint();
};

struct Album
{
   string name;

   map<string, shared_ptr<Track> > tracksMap;
   deque<shared_ptr<Track> > tracksDeque;

   Album(const string& name);
   ~Album();
   deque<shared_ptr<Track> >* getTracks();
   void addTrack(shared_ptr<Track> track);
   void testPrint();
};

struct Artist
{
   string name;

   map<string, shared_ptr<Album> > albumsMap;
   deque<shared_ptr<Album> > albumsDeque;

   Artist(const string& name);
   ~Artist();
   deque<shared_ptr<Album> >* getAlbums();
   void addAlbum(shared_ptr<Album> album);
   void addTrack(shared_ptr<Track> track);
   void testPrint();
};

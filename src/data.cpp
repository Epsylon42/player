#include "data.h"
#include "decode.h"

#include <string>
#include <stdio.h>

std::map<std::string, int> artistIndexes;
std::vector<Artist*> artists;
std::vector<Track*> testTracks;

void initData()
{
   artists.push_back(new Artist("unknown"));
   artistIndexes["unknown"] = 0;
}

void addTrack(Track* track)
{
   if (artistIndexes.find(track->artistName) == artistIndexes.end())
   {
      artists.push_back(new Artist(track->artistName));
      artistIndexes[track->artistName] = artists.size()-1;
   }
   
   artists[artistIndexes[track->artistName]]->addTrack(track);
}

std::vector<Track*>* getTracks()
{
   std::vector<Track*>* tracks = new std::vector<Track*>;
   for (auto artist : artists)
   {
      for (auto album : artist->albums)
      {
	 tracks->insert(tracks->end(), album->tracks.begin(), album->tracks.end());
      }
   }
   return tracks;
}

std::vector<Album*>* getAlbums(bool includeUnknown)
{
   std::vector<Album*>* albums = new std::vector<Album*>;
   for (auto artist : artists)
   {
      albums->insert(albums->end(), artist->albums.begin(), artist->albums.end());
   }
   return albums;
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

Album::Album(const std::string& name, int artistIndex) :
   name(name), artistIndex(artistIndex) {};

Album::Album(const std::string& name, const std::string& artistName) :
   Album(name, artistIndexes[artistName]) {};

Album::Album(const std::string& name, Artist* artist) :
   Album(name, artist->name) {};

Album::~Album()
{
   for (auto track : tracks)
   {
      delete track;
   }
   tracks.clear();
}

void Album::addTrack(Track* track)
{
   tracks.push_back(track);
   trackIndexes[track->name] = tracks.size()-1;
}

void Album::testPrint()
{
   printf("\tStarting to print album %s\n", name.c_str());
   for (auto track : tracks)
   {
	 track->testPrint();
   }
   printf("\tDone printing album %s\n", name.c_str());
}

Artist::Artist(const std::string& name) :
   name(name)
{
   albums.push_back(new Album(this->name + ": unknown", this));
   albumIndexes["unknown"] = 0;
}

Artist::~Artist()
{
   for (auto album : albums)
   {
      delete album;
   }
   albums.clear();
}

void Artist::addAlbum(Album* album)
{
   albums.push_back(album);
   albumIndexes[album->name] = albums.size()-1;
}

void Artist::addTrack(Track* track)
{
   if (albumIndexes.find(track->albumName) == albumIndexes.end())
   {
      albums.push_back(new Album(track->albumName, this));
      albumIndexes[track->albumName] = albums.size()-1;
   }
   albums[albumIndexes[track->albumName]]->addTrack(track);
}

void Artist::testPrint()
{
   printf("Starting to print artist %s\n", name.c_str());
   for (auto album : albums)
   {
	 album->testPrint();
   }
   printf("Done printing artist %s\n\n", name.c_str());
}

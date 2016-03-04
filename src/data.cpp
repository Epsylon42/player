#include "data.hpp"
#include "decode.hpp"

#include <algorithm>
#include <exception>
#include <memory>
#include <string>
#include <stdio.h>

using namespace std;
using namespace data;

map<string, shared_ptr<Artist>> data::artistsMap;
deque<shared_ptr<Artist>>       data::artistsDeque;

void initData()
{
    shared_ptr<Artist> temp(new Artist("all"));
    artistsDeque.push_back(temp);
    artistsMap["all"] = temp;

    temp.reset(new Artist("unknown"));
    artistsDeque.push_back(temp);
    artistsMap["unknown"] = temp;
}

void addTrack(shared_ptr<Track> track)
{
    if (artistsMap.find(track->artistName) == artistsMap.end())
    {
	shared_ptr<Artist> temp(new Artist(track->artistName));
	artistsDeque.push_back(temp);
	artistsMap[track->artistName] = temp;
    }

    artistsMap["all"]->addTrack(track);
    artistsMap[track->artistName]->addTrack(track);
}

deque<shared_ptr<Track>> getTracks()
{
    deque<shared_ptr<Track>> tracks;
    for (auto artist = artistsDeque.begin()+1; artist != artistsDeque.end(); artist++)
    {
	for (auto album = (*artist)->albumsDeque.begin()+1; album != (*artist)->albumsDeque.end(); album++)
	{
	    tracks.insert(tracks.end(), (*album)->tracksDeque.begin(), (*album)->tracksDeque.end());
	}
    }

    return tracks;
}

deque<shared_ptr<Album>> getAlbums(bool includeUnknown)
{
    deque<shared_ptr<Album>> albums;
    for (auto artist : artistsDeque)
    {
	albums.insert(albums.end(), artist->albumsDeque.begin(), artist->albumsDeque.end());
    }

    return albums;
}

Track::Track(const string& file)
{
    filePath = file;
    container = nullptr;
    codecContext = nullptr;
    codec = nullptr;

    open();
    decodeMetadata();
    sampleFormat = getSampleFormat(this);
    close();
}

Track::~Track()
{
    if (opened)
    {
	close();
    }
}

void Track::open()
{
    if (opened)
    {
        return;
    }

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
    for (unsigned int i = 0; i < container->nb_streams; i++)
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

    opened = true;
}

void Track::close()
{
    avcodec_close(codecContext);
    avformat_close_input(&container);
    avformat_free_context(container);
    opened = false;
}

void Track::decodeMetadata()
{
	name = getMetadata("title", {container->metadata, container->streams[streamID]->metadata}, filePath);
	albumName = getMetadata("album", {container->metadata, container->streams[streamID]->metadata});
	artistName = getMetadata("artist", {container->metadata, container->streams[streamID]->metadata});
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

deque<shared_ptr<Track>> Album::getTracks()
{
    return tracksDeque;
}

void Album::addTrack(shared_ptr<Track> track)
{
    if (tracksMap.find(track->name) != tracksMap.end())
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

deque<shared_ptr<Album>> Artist::getAlbums()
{
    return albumsDeque;
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

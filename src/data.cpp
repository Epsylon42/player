#include "data.hpp"
#include "decode.hpp"
#include "log.hpp"

#include <algorithm>
#include <exception>
#include <memory>
#include <string>
#include <iostream>

#include <boost/format.hpp>

using namespace std;
using namespace data;

using boost::format;

namespace data
{
    map<string, shared_ptr<Artist>> artistsMap;
    list<shared_ptr<Artist>>        artists;

    shared_ptr<Artist> allArtists;
    shared_ptr<Artist> unknownArtist;

    void init()
    {
        allArtists    = make_shared<Artist>("all");
        unknownArtist = make_shared<Artist>("unknown");
    }

    void end()
    {
        artistsMap.clear();
        artists.clear();

        allArtists.reset();
        unknownArtist.reset();
    }

    void addTrack(shared_ptr<Track> track)
    {
        allArtists->addTrack(track);

        if (track->artistName.empty())
        {
            unknownArtist->addTrack(track);
            return;
        }

        if (artistsMap.find(track->artistName) == artistsMap.end())
        {
            shared_ptr<Artist> temp = make_shared<Artist>(track->artistName);
            artists.push_back(temp);
            artistsMap[track->artistName] = temp;
        }

        artistsMap[track->artistName]->addTrack(track);
    }

    list<shared_ptr<Artist>> getArtists()
    {
        auto ret = artists;
        ret.push_front(unknownArtist);
        ret.push_front(allArtists);

        return ret;
    }



    OpenedTrack::OpenedTrack() {}

    OpenedTrack::OpenedTrack(const Track* parent) :
        parent(parent),
        filePath(parent->filePath)
    {
        assert(parent != nullptr); //FIXME: also assert the thing above

        formatContext = avformat_alloc_context();

        if (avformat_open_input(&formatContext, filePath.c_str(), nullptr, nullptr) < 0)
        {
            log("Cannot open %s") % filePath;
            return;
        }

        if (avformat_find_stream_info(formatContext, nullptr) < 0)
        {
            log("Cannot find stream info in %s") % filePath;
            return;
        }

        audioStreamID = -1;
        for (unsigned int i = 0; i < formatContext->nb_streams; i++)
        {
            if (formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                audioStreamID = i;
            }
        }
        if (audioStreamID == -1)
        {
            log("No audio streams in %s") % filePath;
            return;
        }

        codecContext = formatContext->streams[audioStreamID]->codec;
        codec = avcodec_find_decoder(codecContext->codec_id);
        if (codec == nullptr)
        {
            log("Cannot find codec for %s") % filePath;
            codecContext = nullptr;
            return;
        }

        if (avcodec_open2(codecContext, codec, nullptr) < 0)
        {
            codec = nullptr;
            codecContext = nullptr;
            log("Cannot open codec for %s") % filePath;
            return;
        }

        duration = chrono::seconds((formatContext->streams[audioStreamID]->duration * formatContext->streams[audioStreamID]->time_base.num) / formatContext->streams[audioStreamID]->time_base.den);

        valid = true;
    }

    OpenedTrack::OpenedTrack(OpenedTrack&& other) :
        parent(other.parent),
        filePath(other.filePath)
    {
        formatContext = other.formatContext;
        codecContext  = other.codecContext;
        codec         = other.codec;
        audioStreamID = other.audioStreamID;
        duration      = other.duration;
        valid         = other.valid;
    }

    void OpenedTrack::operator= (OpenedTrack&& other)
    {
        finalize();

        parent        = other.parent;
        filePath      = other.filePath;
        formatContext = other.formatContext;
        codecContext  = other.codecContext;
        codec         = other.codec;
        audioStreamID = other.audioStreamID;
        duration      = other.duration;
        valid         = other.valid;

        other.valid = false;
        other.codec = nullptr;
        other.codecContext = nullptr;
        other.formatContext = nullptr;
    }

    void OpenedTrack::markAsInvalid()
    {
        valid = false;
    }

    bool OpenedTrack::isValid() const
    {
        return valid;
    }

    OpenedTrack::~OpenedTrack()
    {
        finalize();
    }

    void OpenedTrack::finalize()
    {
        if (codec != nullptr)
        {
            avcodec_close(codecContext);
            codec = nullptr;
            codecContext = nullptr;
        }

        if (formatContext != nullptr)
        {
            avformat_close_input(&formatContext);
            formatContext = nullptr;
        }
    }



    Track::Track(const string& file)
    {
        filePath = file;

        auto opened = open();
        if (!opened.isValid())
        {
            log("Cannot read track data from %s") % filePath;
            exit(1); //TODO: proper error handling
        }

        numSamples = (opened.duration * opened.codecContext->sample_rate).count();
        duration = opened.duration;

        name = getMetadata("title", {opened.formatContext->metadata, opened.formatContext->streams[opened.audioStreamID]->metadata});
        if (name.empty())
        {
            name = filePath;
        }

        albumName = getMetadata("album", {opened.formatContext->metadata, opened.formatContext->streams[opened.audioStreamID]->metadata});
        artistName = getMetadata("artist", {opened.formatContext->metadata, opened.formatContext->streams[opened.audioStreamID]->metadata});
    }

    OpenedTrack Track::open() const
    {
        return OpenedTrack(this);
    }

    void Track::testPrint() const
    {
        cout << format("\t\t%s: %s (%s)") % artistName.c_str() % name.c_str() % albumName.c_str() << endl;
    }



    Album::Album(const string& name) :
        name(name) {};

    void Album::addTrack(shared_ptr<Track> track)
    {
        while (tracksMap.find(track->name) != tracksMap.end())
        {
            track->name = track->name + "_";
            //NOTE: change real track name might be a bad idea
            //      maybe only change its key in the map
        }
        tracksMap[track->name] = track;
        tracks.push_back(track);
    }

    list<shared_ptr<Track>> Album::getTracks() const
    {
        return tracks;
    }

    void Album::testPrint() const
    {
        cout << format("\tStarting to print album %s\n") % name.c_str() << endl;
        for (auto track : tracks)
        {
            track->testPrint();
        }
        cout << format("\tDone printing album %s\n") % name.c_str() << endl;
    }

    Album::~Album()
    {
        tracks.clear();
        tracksMap.clear();
    }



    Artist::Artist(const string& name) :
        name(name)
    {
        allAlbums    = make_shared<Album>(name + ": all");
        unknownAlbum = make_shared<Album>(name + ": unknown");
    }

    void Artist::addAlbum(shared_ptr<Album> album)
    {
        // An album with the same name should not exist
        // If it exists, something has gone really wrong
        albums.push_back(album);
        albumsMap[album->name] = album;
    }

    void Artist::addTrack(shared_ptr<Track> track)
    {
        allAlbums->addTrack(track);

        if (track->albumName.empty())
        {
            unknownAlbum->addTrack(track);
            return;
        }

        if (albumsMap.find(track->albumName) == albumsMap.end())
        {
            shared_ptr<Album> temp = make_shared<Album>(track->albumName);
            albums.push_back(temp);
            albumsMap[track->albumName] = temp;
        }

        albumsMap[track->albumName]->addTrack(track);
    }

    list<shared_ptr<Track>> Artist::getTracks() const
    {
        return allAlbums->getTracks();
    }

    list<shared_ptr<Album>> Artist::getAlbums() const
    {
        return albums;
    }

    void Artist::testPrint() const
    {
        cout << format("Starting to print artist %s\n") % name.c_str() << endl;
        unknownAlbum->testPrint();
        for (auto album : albums)
        {
            album->testPrint();
        }
        cout << format("Done printing artist %s\n\n") % name.c_str() << endl;
    }

    Artist::~Artist()
    {
        albums.clear();
        albumsMap.clear();  
    }
}

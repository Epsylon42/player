#include "data.hpp"
#include "log.hpp"

#include <algorithm>
#include <exception>
#include <memory>
#include <string>
#include <iostream>
#include <regex>

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
        filepath(parent->filepath)
    {
        assert(parent != nullptr);

        pipeline = Gst::Pipeline::create();

        src = Gst::FileSrc::create();
        if (!src)
        {
            log("Error creating source: %s") % filepath;
            return;
        }
        src->property_location() = filepath;

        decode = Gst::ElementFactory::create_element("decodebin");
        if (!decode)
        {
            log("Error creating decodebin: %s") % filepath;
            return;
        }

        conv = Gst::ElementFactory::create_element("audioconvert");
        if (!conv)
        {
            log("Error creating audioconvert: %s") % filepath;
            return;
        }

        resample = Gst::ElementFactory::create_element("audioresample");
        if (!resample)
        {
            log("Error creating audioresample: %s") % filepath;
            return;
        }

        sink = Gst::ElementFactory::create_element("autoaudiosink");
        if (!sink)
        {
            log("Error creating autoaudiosink: %s") % filepath;
            return;
        }

        pipeline->add(src)->add(decode)->add(conv)->add(resample)->add(sink);
        src->link(decode);
        conv->link(resample);
        resample->link(sink);

        decode->signal_pad_added().connect([this](const Glib::RefPtr<Gst::Pad>& pad)
        {
            decode->link(conv);
        });

        pipeline->set_state(Gst::STATE_PAUSED);

        valid = true;
    }

    OpenedTrack::OpenedTrack(OpenedTrack&& other) :
        parent(other.parent),
        filepath(other.filepath)
    {
        pipeline = move(other.pipeline);
        valid = other.valid;
        other.valid = false;
    }

    void OpenedTrack::operator= (OpenedTrack&& other)
    {
        parent = other.parent;
        filepath = other.filepath;
        pipeline = move(other.pipeline);
        valid = other.valid;
        other.valid = false;
    }

    void OpenedTrack::markAsInvalid()
    {
        if (valid)
        {
            pipeline->set_state(Gst::STATE_NULL);
        }
        valid = false;
    }

    bool OpenedTrack::isValid() const
    {
        return valid;
    }

    OpenedTrack::~OpenedTrack()
    {
        if (valid)
        {
            pipeline->set_state(Gst::STATE_NULL);
        }
    }



    Track::Track(const string& file)
    {
        filepath = file;

        auto opened = open();
        if (!opened.isValid())
        {
            log("Cannot read track data from %s") % filepath;
            exit(1); //TODO: proper error handling
        }


        Gst::TagList list;
        Glib::RefPtr<Gst::MessageTag> message = Glib::RefPtr<Gst::MessageTag>::cast_static(opened.pipeline->get_bus()->poll(Gst::MESSAGE_TAG, Gst::CLOCK_TIME_NONE));
        message->parse(list);

        Glib::ustring str;
        bool readSuccess;
        readSuccess = list.get(Gst::TAG_TITLE, str);
        if (readSuccess)
        {
            name = str;
        }
        else
        {
            name = regex_replace(filepath, regex(".*/(.*)"), "$1");
        }

        readSuccess = list.get(Gst::TAG_ALBUM, str);
        albumName = (readSuccess ? str : "unknown");

        readSuccess = list.get(Gst::TAG_ARTIST, str);
        artistName = (readSuccess ? str : "unknown");
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

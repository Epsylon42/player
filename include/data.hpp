#pragma once

#include <gstreamermm.h>

#include <ao/ao.h>
#include <string>
#include <map>
#include <list>
#include <thread>
#include <memory>
#include <chrono>


namespace data
{
    struct OpenedTrack;
    struct Track;
    struct Album;
    struct Artist;

    extern std::map<std::string, std::shared_ptr<Artist>> artistsMap;
    extern std::list<std::shared_ptr<Artist>>             artists;

    extern std::shared_ptr<Artist> allArtists;
    extern std::shared_ptr<Artist> unknownArtist;

    void init();
    void end();

    void addTrack(std::shared_ptr<Track> track);
    std::list<std::shared_ptr<Artist>> getArtists();

    struct OpenedTrack
    {
        const Track* parent;
        std::string filepath;

        Glib::RefPtr<Gst::Pipeline> pipeline;

        Glib::RefPtr<Gst::FileSrc> src;

        Glib::RefPtr<Gst::Element> decode;
        Glib::RefPtr<Gst::Element> conv;
        Glib::RefPtr<Gst::Element> resample;
        Glib::RefPtr<Gst::Element> sink;

        OpenedTrack();
        OpenedTrack(const Track* parent);
        OpenedTrack(OpenedTrack&& other);

        OpenedTrack& operator= (const OpenedTrack&) = delete;
        void operator= (OpenedTrack&& other);

        void markAsInvalid();

        bool isValid() const;

        ~OpenedTrack();

        private:
        bool valid = false;
    };

    struct Track
    {
        std::string filepath;

        std::string name;
        std::string artistName;
        std::string albumName;

        Track(const std::string& file);

        OpenedTrack open() const;
        void testPrint() const;
    };



    struct Album
    {
        std::string name;

        std::map<std::string, std::shared_ptr<Track>> tracksMap;
        std::list<std::shared_ptr<Track>>             tracks;

        Album(const std::string& name);

        void addTrack(std::shared_ptr<Track> track);

        std::list<std::shared_ptr<Track>> getTracks() const;
        void testPrint() const;

        ~Album();
    };



    struct Artist
    {
        std::string name;

        std::map<std::string, std::shared_ptr<Album>> albumsMap;
        std::list<std::shared_ptr<Album>>             albums;

        std::shared_ptr<Album> allAlbums;
        std::shared_ptr<Album> unknownAlbum;

        Artist(const std::string& name);

        void addAlbum(std::shared_ptr<Album> album);
        void addTrack(std::shared_ptr<Track> track);

        std::list<std::shared_ptr<Track>> getTracks() const;
        std::list<std::shared_ptr<Album>> getAlbums() const;
        void testPrint() const;

        ~Artist();
    };
}

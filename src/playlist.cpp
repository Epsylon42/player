#include "playlist.hpp"

#include <iostream>
#include <utility>
#include <algorithm>

using namespace std;

using data::Track;
using data::Album;
using data::Artist;

// ABSTRACT PLAYLIST
Playlist::Playlist(const string& name) : name(name) {}



// SIMPLE PLAYLIST
SimplePlaylist::SimplePlaylist(const string& name) : Playlist(name) {}

SimplePlaylist::SimplePlaylist(const string& name, const list<shared_ptr<Track>>& tracks) :
    Playlist(name), tracks(tracks) 
{}

void SimplePlaylist::addTrack(shared_ptr<Track> track)
{
    tracks.push_back(track);
}

void SimplePlaylist::removeTrack(shared_ptr<Track> track)
{
    auto trackIter = find(tracks.begin(), tracks.end(), track);
    if (trackIter != tracks.end())
    {
        tracks.erase(trackIter);
    }
}

list<shared_ptr<Track>> SimplePlaylist::getTracks() const
{
    return tracks;
}

void SimplePlaylist::testPrint() const
{
    cout << "starting to print playlist " << name << endl;
    for (auto &track : tracks)
    {
        track->testPrint();
    }
    cout << "done printing playlist " << name << endl;
}



// SMART PLAYLIST
SmartPlaylist::SmartPlaylist(const string& name, std::unique_ptr<Condition> condition) : 
    Playlist(name),
    condition(move(condition))
{}

list<shared_ptr<Track>> SmartPlaylist::getTracks() const
{
    list<shared_ptr<Track>> ret;

    for (auto &track : data::allArtists->getTracks())
    {
        if(condition->check(track))
        {
            ret.push_back(track);
        }
    }

    return ret;
}

void SmartPlaylist::testPrint() const
{
    cout << "starting to print playlist " << name << endl;
    for (auto &track : data::allArtists->getTracks())
    {
        track->testPrint();
    }
    cout << "done printing playlist " << name << endl;   
}



// CONDITIONS

// NAME
NameCondition::NameCondition(const string& name) : name(name) {}

bool NameCondition::check(const shared_ptr<Track>& track) const
{
    return track->name == name;
}


// ALBUM
AlbumNameCondition::AlbumNameCondition(const string& albumName) : albumName(albumName) {}

bool AlbumNameCondition::check(const shared_ptr<Track>& track) const
{
    return track->albumName == albumName;
}


// ARTIST
ArtistNameCondition::ArtistNameCondition(const string& artistName) : artistName(artistName) {}

bool ArtistNameCondition::check(const shared_ptr<Track>& track) const
{
    return track->artistName == artistName;
}


// LOGICAL
LogicalCondition::LogicalCondition(vector<unique_ptr<Condition>> conditions) :
    conditions(move(conditions))
{}


// AND
bool AND_Condition::check(const shared_ptr<Track>& track) const
{
    if (conditions.empty())
    {
        return true;
    }

    for (auto &cond : conditions)
    {
        if (!cond->check(track))
        {
            return false;
        }
    }

    return true;
}


// OR
bool OR_Condition::check(const shared_ptr<Track>& track) const
{
    if (conditions.empty())
    {
        return true;
    }

    for (auto &cond : conditions)
    {
        if (cond->check(track))
        {
            return true;
        }
    }

    return false;
}

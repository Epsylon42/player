#include "playlist.hpp"

#include <iostream>

using namespace std;

Playlist::Playlist(const string& name) : name(name) {}


SimplePlaylist::SimplePlaylist(const string& name) : Playlist(name) {}

SimplePlaylist::SimplePlaylist(const string& name, const deque<shared_ptr<Track>>& tracks) :
   Playlist(name), tracks(tracks) {}

deque<shared_ptr<Track>> SimplePlaylist::getList() const
{
   return tracks;
}

void SimplePlaylist::addTrack(std::shared_ptr<Track> track)
{
   tracks.push_back(track);
}

void SimplePlaylist::testPrint() const
{
   cout << "starting to print playlist " << name << endl;
   for (auto track : tracks)
   {
      track->testPrint();
   }
   cout << "done printing playlist " << name << endl;
}

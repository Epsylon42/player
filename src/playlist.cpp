#include "playlist.hpp"

#include <iostream>
#include <utility>

using namespace std;

// ABSTRACT PLAYLIST
Playlist::Playlist(const string& name) : name(name) {}

// SIMPLE PLAYLIST
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
   for (auto &track : tracks)
   {
      track->testPrint();
   }
   cout << "done printing playlist " << name << endl;
}

// SMART PLAYLIST
SmartPlaylist::SmartPlaylist(const string& name) : Playlist(name) {}

void SmartPlaylist::addAND_Condition(unique_ptr<Condition> cond)
{
   AND_condition.addCondition(move(cond));
}

void SmartPlaylist::addOR_Condition(unique_ptr<Condition> cond)
{
   OR_condition.addCondition(move(cond));
}

deque<shared_ptr<Track>> SmartPlaylist::getList() const
{
   deque<shared_ptr<Track>> ret;

   auto tracks = getTracks();
   for (auto &track : tracks)
   {
      if(AND_condition.check(track) && OR_condition.check(track))
      {
	 ret.push_back(track);
      }
   }

   return ret;
}

void SmartPlaylist::testPrint() const
{
   cout << "starting to print playlist " << name << endl;
   for (auto &track : getList())
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


// AND
void AND_Condition::addCondition(unique_ptr<Condition> cond)
{
   conditions.push_back(move(cond));
}

bool AND_Condition::check(const shared_ptr<Track>& track) const
{
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
void OR_Condition::addCondition(unique_ptr<Condition> cond)
{
   conditions.push_back(move(cond));
}

bool OR_Condition::check(const shared_ptr<Track>& track) const
{
   if (conditions.size() == 0)
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

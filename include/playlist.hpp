#pragma once

#include "data.hpp"

#include <deque>
#include <memory>
#include <string>

class Playlist
{
public:
   std::string name;

   Playlist(const std::string& name);
   
   virtual std::deque<std::shared_ptr<Track>> getList() const = 0;
   virtual void testPrint() const = 0;
};

class SimplePlaylist : public Playlist
{
   std::deque<std::shared_ptr<Track>> tracks;
   
public:
   SimplePlaylist(const std::string& name);
   SimplePlaylist(const std::string& name, const std::deque<std::shared_ptr<Track>>& tracks);

   void addTrack(std::shared_ptr<Track> track);
   
   virtual std::deque<std::shared_ptr<Track>> getList() const override;
   virtual void testPrint() const override;
};

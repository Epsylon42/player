#pragma once

#include "data.hpp"

#include <deque>
#include <memory>
#include <string>

class Playlist;
class SimplePlaylist;
class SmartPlaylist;

class Condition;
class NameCondition; 
class AlbumNameCondition;
class ArtistNameCondition;
class AND_Condition;
class OR_Condition;

// PLAYLISTS
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


// CONDITIONS
class Condition
{
    public:
	virtual bool check(const std::shared_ptr<Track>& tracks) const = 0;
};


class NameCondition : public Condition
{
    std::string name;

    public:
    NameCondition(const std::string& name);

    virtual bool check(const std::shared_ptr<Track>& tracks) const override;
};


class AlbumNameCondition : public Condition
{
    std::string albumName;

    public:
    AlbumNameCondition(const std::string& albumName);

    virtual bool check(const std::shared_ptr<Track>& tracks) const override;
};


class ArtistNameCondition : public Condition
{
    std::string artistName;

    public:
    ArtistNameCondition(const std::string& artistName);

    virtual bool check(const std::shared_ptr<Track>& tracks) const override;
};


class AND_Condition : public Condition
{
    std::deque<std::unique_ptr<Condition>> conditions;

    public:
    void addCondition(std::unique_ptr<Condition> cond);

    virtual bool check(const std::shared_ptr<Track>& tracks) const override;
};


class OR_Condition : public Condition
{
    std::deque<std::unique_ptr<Condition>> conditions;

    public:
    void addCondition(std::unique_ptr<Condition> cond);

    virtual bool check(const std::shared_ptr<Track>& tracks) const override;
};


// SMART PLAYLIST
class SmartPlaylist : public Playlist
{
    AND_Condition AND_condition;
    OR_Condition  OR_condition;

    public:
    SmartPlaylist(const std::string& name);

    void addAND_Condition(std::unique_ptr<Condition> cond);
    void addOR_Condition(std::unique_ptr<Condition> cond);

    virtual std::deque<std::shared_ptr<Track>> getList() const override;
    virtual void testPrint() const override;
};

#pragma once

#include "data.hpp"

#include <list>
#include <vector>
#include <initializer_list>
#include <memory>
#include <string>

class Playlist;
class SimplePlaylist;
class SmartPlaylist;

class Condition;
class NameCondition; 
class AlbumNameCondition;
class ArtistNameCondition;
class LogicalCondition;
class AND_Condition;
class OR_Condition;

// PLAYLISTS
class Playlist
{
    public:
	std::string name;

	Playlist(const std::string& name);

	virtual std::list<std::shared_ptr<data::Track>> getTracks() const = 0;
	virtual void testPrint() const = 0;
};


class SimplePlaylist : public Playlist
{
    std::list<std::shared_ptr<data::Track>> tracks;

    public:
    SimplePlaylist(const std::string& name);
    SimplePlaylist(const std::string& name, const std::list<std::shared_ptr<data::Track>>& tracks);

    void addTrack(std::shared_ptr<data::Track> track);
    void removeTrack(std::shared_ptr<data::Track> track);

    virtual std::list<std::shared_ptr<data::Track>> getTracks() const override;
    virtual void testPrint() const override;
};


// CONDITIONS
class Condition
{
    public:
	virtual bool check(const std::shared_ptr<data::Track>& tracks) const = 0;
};


class NameCondition : public Condition
{
    std::string name;

    public:
    NameCondition(const std::string& name);

    virtual bool check(const std::shared_ptr<data::Track>& tracks) const override;
};


class AlbumNameCondition : public Condition
{
    std::string albumName;

    public:
    AlbumNameCondition(const std::string& albumName);

    virtual bool check(const std::shared_ptr<data::Track>& tracks) const override;
};


class ArtistNameCondition : public Condition
{
    std::string artistName;

    public:
    ArtistNameCondition(const std::string& artistName);

    virtual bool check(const std::shared_ptr<data::Track>& tracks) const override;
};


class LogicalCondition : public Condition
{
    protected:
    std::vector<std::unique_ptr<Condition>> conditions;

    public:
    LogicalCondition(std::vector<std::unique_ptr<Condition>> conditions);
};


class AND_Condition : public LogicalCondition
{
    public:
    virtual bool check(const std::shared_ptr<data::Track>& tracks) const override;
};


class OR_Condition : public LogicalCondition
{

    public:
    virtual bool check(const std::shared_ptr<data::Track>& tracks) const override;
};


// SMART PLAYLIST
class SmartPlaylist : public Playlist
{
    std::unique_ptr<Condition> condition;

    public:
    SmartPlaylist(const std::string& name, std::unique_ptr<Condition> condition);

    virtual std::list<std::shared_ptr<data::Track>> getTracks() const override;
    virtual void testPrint() const override;
};

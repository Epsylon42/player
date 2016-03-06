#pragma once

#include <ncurses.h>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

#include "data.hpp"

constexpr char BORDERS_ALL        = 0b00001111;
constexpr char BORDER_TOP         = 0b00001000;
constexpr char BORDER_LEFT        = 0b00000100;
constexpr char BORDER_RIGHT       = 0b00000010;
constexpr char BORDER_BOTTOM      = 0b00000001;
constexpr char BORDERS_NONE       = 0b00000000;
constexpr char BORDERS_HORIZONTAL = 0b00001001;
constexpr char BORDERS_VERTICAL   = 0b00000110;

class Window;
template< typename DequeType > class DequeListingWindow;
class TracksListingWindow;
class ArtistsListingWindow;
class AlbumsListingWindow;
class PlaybackControlWindow;

namespace interface
{
    extern std::recursive_mutex interfaceMutex;
    extern int sizeX;
    extern int sizeY;
    extern std::deque<std::shared_ptr<Window>> windows;
    extern std::shared_ptr<ArtistsListingWindow>  artistsWindow;
    extern std::shared_ptr<AlbumsListingWindow>   albumsWindow;
    extern std::shared_ptr<TracksListingWindow>   tracksWindow;
    extern std::shared_ptr<PlaybackControlWindow> playbackWindow;
    extern std::shared_ptr<Window> selectedWindow;
}

void initInterface();
void interfaceLoop();
void endInterface();
void updateWindows();
void fullRefresh();
bool readKey();

class Window
{
    public:
	int startX;
	int startY;
	int nlines;
	int ncols;
	std::unique_ptr<WINDOW> window;
	std::deque<std::shared_ptr<Window>> subWindows;
	std::shared_ptr<Window> selectedSubWindow;

	Window(int startY, int startX, int nlines, int ncols, char borders);

	void reshapeWindow(int newY, int newX, int newLines, int newColumns);

	virtual void update();
	virtual void processKey(int ch) = 0;

	virtual ~Window();

    protected:
	bool borderTop;
	bool borderLeft;
	bool borderRight;
	bool borderBottom;

	void displayBorders();

	virtual void afterReshape() = 0;
};



// DequeType !MUST! be of std::deque<something> type
template< typename DequeType > 
class DequeListingWindow : public Window
{
    public:
	typename std::deque<DequeType>::iterator cursorPos;
	typename std::deque<DequeType>::iterator screenStart;

	DequeListingWindow(int startY, int startX, int nlines, int ncols, char borders, std::deque<DequeType> data);

	void assignNewDeque(std::deque<DequeType> newDeque);

	virtual void update() override;
	virtual void processKey(int ch)      override;

	virtual ~DequeListingWindow()        override;

    protected:
	std::deque<DequeType> data;

	virtual void select() = 0;
	virtual void press(int key)  = 0;

	virtual std::string getName(typename std::deque<DequeType>::iterator iter) const = 0;

	virtual void afterReshape()          override;
};

template< typename DequeType >
class MediaListingWindow : public DequeListingWindow<DequeType>
{
    public:
	template< typename ... Args >
	    MediaListingWindow(Args... args) :
		DequeListingWindow<DequeType>::DequeListingWindow(std::forward<Args>(args)...) {}

	virtual std::string getName(typename std::deque<DequeType>::iterator iter) const override;
};



class TracksListingWindow : public MediaListingWindow<std::shared_ptr<Track>>
{
    public:
	template< typename ... Args >
	    TracksListingWindow(Args... args) :
		MediaListingWindow(std::forward<Args>(args)...) {select();}

    protected:
	virtual void select() override;
	virtual void press(int key)  override;
};

class AlbumsListingWindow : public MediaListingWindow<std::shared_ptr<Album>>
{
    public:
	template< typename ... Args >
	    AlbumsListingWindow(Args... args) :
		MediaListingWindow(std::forward<Args>(args)...) {select();}

    protected:
	virtual void select() override;
	virtual void press(int key)  override;
};

class ArtistsListingWindow : public MediaListingWindow<std::shared_ptr<Artist>>
{
    public:
	template< typename ... Args >
	    ArtistsListingWindow(Args... args) :
		MediaListingWindow(std::forward<Args>(args)...) {select();}

    protected:
	virtual void select() override;
	virtual void press(int key) override;
};



class PlaybackControlWindow : public Window
{
    friend void playbackWindowThread(PlaybackControlWindow* win);
    public:
    PlaybackControlWindow(int startY, int startX, int nlines, int ncols, char borders);

    virtual void update() override;
    virtual void processKey(int ch)      override;

    virtual ~PlaybackControlWindow()     override;

    protected:
    bool stopThread;
    std::thread winThread;

    void rewindForward();
    void rewindBackward();

    void playbackWindowThread();

    virtual void afterReshape()          override;
};

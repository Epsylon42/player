#pragma once

#include <ncurses.h>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <functional>

#include "data.hpp"

class Window;
class EmptyWindow;
class MetaWindow;
class ColumnWindow;
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
    extern std::shared_ptr<ColumnWindow> mainWindow;
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

class Window : public std::enable_shared_from_this<Window>
{
    friend class MetaWindow;

    public:
	int startX;
	int startY;
	int nlines;
	int ncols;
	std::unique_ptr<WINDOW> window;

	Window(int startY, int startX, int nlines, int ncols);

	void reshapeWindow(int newY, int newX, int newLines, int newColumns);

    std::weak_ptr<Window> getParent() const;

    // It would be better for everyone if this function is called on hjkl press in all its overrides
    // except for meta windows and such
    // it passes window switch keys to parent window
	virtual void processKey(int ch);

    // This REALLY must be called from all overrides
	virtual void update();

    virtual std::weak_ptr<Window> getSelected();

	virtual ~Window();

    protected:
    std::weak_ptr<Window> parent;

	virtual void afterReshape() = 0;
};


class EmptyWindow : public Window
{
    public:
        EmptyWindow(int startY, int startX, int nlines, int ncols);

	virtual void update()           override;
	virtual void processKey(int ch) override;

    protected:
	virtual void afterReshape()     override;
};


class MetaWindow : public Window
{
    public:
        MetaWindow(int startY, int startX, int nlines, int ncols);

        virtual void addWindow(std::shared_ptr<Window> win);
        virtual std::weak_ptr<Window> getSelected() override;

    protected:
        std::deque<float> quotients;
        std::deque<std::shared_ptr<Window>> windows;
        decltype(windows)::iterator selectedWindow;

        virtual void recalculateSizes() = 0;
};

class ColumnWindow : public MetaWindow
{
    public:
        ColumnWindow(int startY, int startX, int nlines, int ncols);

	virtual void update()                               override;
	virtual void processKey(int ch)                     override;

    protected:
    virtual void afterReshape()                         override;
    virtual void recalculateSizes()                     override;
};

class LineWindow : public MetaWindow
{
    public:
        LineWindow(int startY, int startX, int nlines, int ncols);

	virtual void update()                               override;
	virtual void processKey(int ch)                     override;

    protected:
	virtual void afterReshape()     override;
    virtual void recalculateSizes() override;
};


template< typename DequeType > 
class DequeListingWindow : public Window
{
    public:
	typename std::deque<DequeType>::iterator cursorPos;
	typename std::deque<DequeType>::iterator screenStart;

	DequeListingWindow(int startY, int startX, int nlines, int ncols, std::deque<DequeType> data);

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
	    MediaListingWindow(Args&&... args) :
		DequeListingWindow<DequeType>::DequeListingWindow(std::forward<Args>(args)...) {}

	virtual std::string getName(typename std::deque<DequeType>::iterator iter) const override;
};



class TracksListingWindow : public MediaListingWindow<std::shared_ptr<Track>>
{
    public:
	template< typename ... Args >
	    TracksListingWindow(Args&&... args) :
		MediaListingWindow(std::forward<Args>(args)...) {select();}

    protected:
	virtual void select() override;
	virtual void press(int key)  override;
};

class AlbumsListingWindow : public MediaListingWindow<std::shared_ptr<Album>>
{
    public:
	template< typename ... Args >
	    AlbumsListingWindow(Args&&... args) :
		MediaListingWindow(std::forward<Args>(args)...) {select();}

    protected:
	virtual void select() override;
	virtual void press(int key)  override;
};

class ArtistsListingWindow : public MediaListingWindow<std::shared_ptr<Artist>>
{
    public:
	template< typename ... Args >
	    ArtistsListingWindow(Args&&... args) :
		MediaListingWindow(std::forward<Args>(args)...) {select();}

    protected:
	virtual void select() override;
	virtual void press(int key) override;
};



class PlaybackControlWindow : public Window
{
    friend void playbackWindowThread(PlaybackControlWindow* win);
    public:
    PlaybackControlWindow(int startY, int startX, int nlines, int ncols);

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

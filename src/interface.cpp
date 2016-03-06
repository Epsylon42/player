#include "interface.hpp"
#include "play.hpp"

#include "ncurses_wrapper.hpp"

#include <unistd.h>
#include <iostream>
#include <functional>

using namespace std;
using namespace interface;

recursive_mutex interface::interfaceMutex;
int interface::sizeX;
int interface::sizeY;
deque<shared_ptr<Window>> interface::windows;
shared_ptr<ArtistsListingWindow>  interface::artistsWindow;
shared_ptr<AlbumsListingWindow>   interface::albumsWindow;
shared_ptr<TracksListingWindow>   interface::tracksWindow;
shared_ptr<PlaybackControlWindow> interface::playbackWindow;
shared_ptr<Window> interface::selectedWindow;


void initInterface()
{
    initscr();
    halfdelay(10);
    curs_set(FALSE);
    noecho();

    sizeX = getmaxx(stdscr);
    sizeY = getmaxy(stdscr);

    tracksWindow = make_shared<TracksListingWindow>(sizeY/2+1, 0, sizeY/2-1+(sizeY%2), sizeX, BORDERS_ALL, getTracks());
    albumsWindow = make_shared<AlbumsListingWindow>(0, sizeX/2, sizeY/2+1, sizeX/2+(sizeX%2), BORDERS_ALL, getAlbums());
    artistsWindow = make_shared<ArtistsListingWindow>(5, 0, sizeY/2+1-5, sizeX/2, BORDERS_ALL, data::artistsDeque);
    playbackWindow = make_shared<PlaybackControlWindow>(0, 0, 5, sizeX/2, BORDERS_ALL);

    selectedWindow =  tracksWindow;
    windows.push_back(artistsWindow);
    windows.push_back(albumsWindow);
    windows.push_back(tracksWindow);
    windows.push_back(playbackWindow);
}

void interfaceLoop()
{
    initInterface();
    do
    {
	updateWindows();
    } while (readKey());
    endInterface();
}

void endInterface()
{
    selectedWindow.reset();
    tracksWindow.reset();
    albumsWindow.reset();
    artistsWindow.reset();
    playbackWindow.reset();
    windows.clear();

    endwin();
}

void updateWindows()
{
    //usleep(10000); 
    int newSizeX = getmaxx(stdscr);
    int newSizeY = getmaxy(stdscr);
    if (newSizeX != sizeX || newSizeY != sizeY)
    {
	sizeX = newSizeX;
	sizeY = newSizeY;
	tracksWindow->reshapeWindow(sizeY/2+1, 0, sizeY/2-1+(sizeY%2), sizeX);
	albumsWindow->reshapeWindow(0, sizeX/2, sizeY/2+1, sizeX/2+(sizeX%2));
	artistsWindow->reshapeWindow(5, 0, sizeY/2+1-5, sizeX/2);
	playbackWindow->reshapeWindow(0, 0, 5, sizeX/2);
	clear();
	refresh();
    }

    for (auto window : windows)
    {
	window->update();
    }
}

void fullRefresh()
{
    clear();
    for (auto window : windows)
    {
	wclear(window->window);
    }
    refresh();
}

//returns false if pressed exit key
bool readKey()
{ 
    int ch = wgetch(selectedWindow->window.get());
    switch (ch)
    {
	case ERR:
	    break;
	case 'Q':
	    return false;
	    break;
	case 'R':
	    fullRefresh();
	    break;
	case 'a':
	    selectedWindow = artistsWindow;
	    break;
	case 'l': // a(L)bums
	    selectedWindow = albumsWindow;
	    break;
	case 't':
	    selectedWindow = tracksWindow;
	    break;
	case 'P':
	    selectedWindow = playbackWindow;
	    break;
	case ' ':
	    sendPlaybackCommand(new CommandTOGGLE());
	    break;
	case 'E': // (E)nd
	    sendPlaybackCommand(new CommandSTOP());
	    break;
	case 'p':
	    sendPlaybackCommand(new CommandPREVIOUS());
	    break;
	case 'n':
	    sendPlaybackCommand(new CommandNEXT());
	    break;
	default:
	    selectedWindow->processKey(ch);
    }
    return true;
}

Window::Window(int startY, int startX, int nlines, int ncols, char borders) :
    startX(startX), startY(startY), nlines(nlines), ncols(ncols)
{
    window.reset(newwin(nlines, ncols, startY, startX));
    // nodelay(window, true);
    keypad(window.get(), true);

    borderTop    = borders & BORDER_TOP;
    borderLeft   = borders & BORDER_LEFT;
    borderRight  = borders & BORDER_RIGHT;
    borderBottom = borders & BORDER_BOTTOM;
}

Window::~Window()
{
    delwin(window.release());
}

void Window::displayBorders()
{
    char frame;
    if (this == selectedWindow.get())
    {
	frame = '#';
    }
    else
    {
	frame = '+';
    }

    for (int i = 0; i < ncols; i++)
    {
	if (borderTop)
	{
	    mvwprintw(window, 0, i, "%c", frame);
	}
	if (borderBottom)
	{
	    mvwprintw(window, nlines-1, i, "%c", frame);
	}
    }
    for (int i = 0; i < nlines; i++)
    {
	if (borderLeft)
	{
	    mvwprintw(window, i, 0, "%c", frame);
	}
	if (borderRight)
	{
	    mvwprintw(window, i, ncols-1, "%c", frame);
	}
    }
}

void Window::reshapeWindow(int newY, int newX, int newLines, int newColumns)
{
    startX = newX;
    startY = newY;
    nlines = newLines;
    ncols = newColumns;

    mvwin(window, startY, startX);
    wresize(window, nlines, ncols);

    afterReshape();
    wclear(window);
    update();
}

void Window::update()
{
    displayBorders();

    for (auto win : subWindows)
    {
	win->update();
    }

    interfaceMutex.lock();
    wrefresh(window);
    interfaceMutex.unlock();
}


    template< typename DequeType >
void DequeListingWindow<DequeType>::afterReshape()
{
    screenStart = cursorPos;
}

template< typename DequeType >
DequeListingWindow<DequeType>::DequeListingWindow(int startY, int startX, int nlines, int ncols, char borders, deque<DequeType> data) :
    Window(startY, startX, nlines, ncols, borders), data(data)
{
    cursorPos   = this->data.begin();
    screenStart = this->data.begin();
}

    template< typename DequeType >
DequeListingWindow<DequeType>::~DequeListingWindow()
{
    data.clear();
    // delete data;
}

    template< typename DequeType >
void DequeListingWindow<DequeType>::update()
{
    auto p = screenStart;
    for (int i = 0; i < nlines-2; i++)
    {
	if (p == data.end())
	{
	    break;
	}
	wmove(window, i+1, 1);
	wclrtoeol(window);
	if (p == cursorPos)
	{
	    wattron(window, A_REVERSE);
	    if (this == selectedWindow.get())
	    {
		wattron(window, A_BOLD);
	    }
	}
	wprintw(window, "%s", getName(p).c_str());
	wattroff(window, A_REVERSE | A_BOLD);
	p++;
    }

    Window::update();
}

    template< typename DequeType >
void DequeListingWindow<DequeType>::processKey(int ch)
{
    switch (ch)
    {
	case KEY_UP:
	    if (cursorPos != data.begin())
	    {
		if (cursorPos == screenStart)
		{
		    screenStart--;
		}
		cursorPos--;
		select();
	    }
	    break;
	case KEY_DOWN:
	    if (cursorPos != data.end()-1)
	    {
		if (cursorPos == screenStart+nlines-3)
		{
		    screenStart++;
		}
		cursorPos++;
		select();
	    }
	    break;
	case 'S':
	    press();
	    break;
	default:
	    break;
    }
}

    template< typename DequeType >
void DequeListingWindow<DequeType>::assignNewDeque(deque<DequeType> newDeque)
{
    data.clear();

    data = newDeque;
    cursorPos   = data.begin();
    screenStart = data.begin();

    select();

    wclear(window);
    update();
}

template< typename DequeType >
string MediaListingWindow<DequeType>::getName(typename deque<DequeType>::iterator iter) const
{
    return (*iter)->name;
}

void TracksListingWindow::select()
{

}

void TracksListingWindow::press()
{
    startPlayback(*cursorPos, {});
}

void AlbumsListingWindow::select()
{
    tracksWindow->assignNewDeque((*cursorPos)->getTracks());
}

void AlbumsListingWindow::press()
{
    startPlayback(*cursorPos, PlaybackOption::shuffle);
}

void ArtistsListingWindow::select()
{
    albumsWindow->assignNewDeque((*cursorPos)->getAlbums());
}

void ArtistsListingWindow::press()
{
    startPlayback(*cursorPos, PlaybackOption::shuffle);
}

PlaybackControlWindow::PlaybackControlWindow(int startY, int startX, int nlines, int ncols, char borders) :
    Window(startY, startX, nlines, ncols, borders),
    stopThread(false),
    winThread(mem_fn(&PlaybackControlWindow::playbackWindowThread), this) {}

PlaybackControlWindow::~PlaybackControlWindow()
{
    stopThread = true;
    winThread.join();
}

void PlaybackControlWindow::update()
{
    Window::update();
}

void PlaybackControlWindow::processKey(int ch)
{
    switch (ch)
    {
	case KEY_RIGHT:
	    rewindForward();
	    break;
	case KEY_LEFT:
	    rewindBackward();
	    break;
	default:
	    break;
    }
}

void PlaybackControlWindow::afterReshape()
{

}

void PlaybackControlWindow::rewindForward()
{

}

void PlaybackControlWindow::rewindBackward()
{

}

void PlaybackControlWindow::playbackWindowThread()
{
    while(!stopThread)
    {
	for (int i = 1; i < nlines-1; i++)
	{
	    wmove(window, i, 1);
	    wclrtoeol(window);
	}

	if (playbackInProcess() && play::NowPlaying::track)
	{
	    wmove(window, 1, 1);
	    wattron(window, A_BOLD);
	    wprintw(window, "Track: ");

	    wattroff(window, A_BOLD);
	    wprintw(window, "%s", play::NowPlaying::track->name.c_str());
	    wattron(window, A_BOLD);

	    wmove(window, 2, 1);
	    if (play::playbackPause)
	    {
		wprintw(window, "paused:  ");
	    }
	    else
	    {
		wprintw(window, "playing: ");
	    }
	    wattroff(window, A_BOLD);
	    int time = play::NowPlaying::frame;
	    wprintw(window, "%d", time);

	    Window::update();
	    usleep(10000);
	}
	else
	{
	    usleep(500000);
	}
    }
}

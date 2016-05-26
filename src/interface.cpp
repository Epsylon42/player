#include "interface.hpp"
#include "play.hpp"
#include "log.hpp"

#include "ncurses_wrapper.hpp"

#include <vector>
#include <set>
#include <iostream>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <chrono>
#include <algorithm>

using namespace std;
using namespace interface;

recursive_mutex interface::interfaceMutex;
int interface::sizeX;
int interface::sizeY;
shared_ptr<ColumnWindow> interface::mainWindow;
shared_ptr<ArtistsListingWindow>  interface::artistsWindow;
shared_ptr<AlbumsListingWindow>   interface::albumsWindow;
shared_ptr<TracksListingWindow>   interface::tracksWindow;
shared_ptr<PlaybackControlWindow> interface::playbackWindow;
/* shared_ptr<Window> interface::selectedWindow; */


void initInterface()
{
    initscr();
    halfdelay(10);
    curs_set(FALSE);
    noecho();

    sizeX = getmaxx(stdscr);
    sizeY = getmaxy(stdscr);

    mainWindow = make_shared<ColumnWindow>(0, 0, sizeY, sizeX);

    tracksWindow = make_shared<TracksListingWindow>(0, 0, 0, 0, getTracks());
    albumsWindow = make_shared<AlbumsListingWindow>(0, 0, 0, 0, getAlbums());
    artistsWindow = make_shared<ArtistsListingWindow>(0, 0, 0, 0, data::artistsDeque);
    playbackWindow = make_shared<PlaybackControlWindow>(0, 0, 0, 0);

    auto line1 = make_shared<LineWindow>(0, 0, 0, 0);
    {
        auto line1Col1 = make_shared<ColumnWindow>(0, 0, 0, 0);
        line1Col1->addWindow(playbackWindow);
        line1Col1->addWindow(artistsWindow);
        line1->addWindow(line1Col1);
    }
    line1->addWindow(albumsWindow);

    mainWindow->addWindow(line1);
    mainWindow->addWindow(tracksWindow);
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
    tracksWindow.reset();
    albumsWindow.reset();
    artistsWindow.reset();
    playbackWindow.reset();
    mainWindow.reset();

    endwin();
}

void updateWindows()
{
    int newSizeX = getmaxx(stdscr);
    int newSizeY = getmaxy(stdscr);
    if (newSizeX != sizeX || newSizeY != sizeY)
    {
        sizeX = newSizeX;
        sizeY = newSizeY;
        mainWindow->reshapeWindow(0, 0, sizeY, sizeX);
        clear();
        refresh();
    }

    mainWindow->update();
}

void fullRefresh()
{
    clear();
    wclear(mainWindow->window);
    refresh();

    mainWindow->update();
}

//returns false if pressed exit key
bool readKey()
{ 
    int ch = wgetch(mainWindow->window.get());
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
        case ' ':
            // TODO: override operator new to return static value.
            sendPlaybackCommand(new CommandTOGGLE());
            break;
        case 'E': // (E)nd
            sendPlaybackCommand(new CommandSTOP());
            break;
        case '<':
            sendPlaybackCommand(new CommandPREVIOUS());
            break;
        case '>':
            sendPlaybackCommand(new CommandNEXT());
            break;
        default:
            {
                if (auto locked = mainWindow->getSelected().lock())
                {
                    locked->processKey(ch);
                }
            }
    }
    return true;
}

Window::Window(int startY, int startX, int nlines, int ncols) :
    startX(startX), startY(startY), nlines(nlines), ncols(ncols),
    window(newwin(nlines, ncols, startY, startX)), 
    parent()
{
    // nodelay(window, true);
    keypad(window.get(), true);
}

weak_ptr<Window> Window::getSelected()
{
    return shared_from_this();
}

Window::~Window()
{
    delwin(window.release());
}

// TODO: add option without update, clear, etc.
void Window::reshapeWindow(int newY, int newX, int newLines, int newColumns)
{
    startX = newX;
    startY = newY;
    nlines = newLines;
    ncols = newColumns;

    wresize(window, nlines, ncols);
    mvwin(window, startY, startX);

    afterReshape();
    wclear(window);
    update();
}

void Window::update()
{
    interfaceMutex.lock();
    wrefresh(window);
    interfaceMutex.unlock();
}

void Window::processKey(int ch)
{
    switch(ch)
    {
        case 'h':
        case 'j':
        case 'k':
        case 'l':
            {
                if (auto locked = parent.lock())
                {
                    locked->processKey(ch);
                }
            }
            break;

        default:
            break;
    }
}

weak_ptr<Window> Window::getParent() const
{
    return parent;
}

EmptyWindow::EmptyWindow(int startY, int startX, int nlines, int ncols) :
    Window(startY, startX, nlines, ncols) {}

void EmptyWindow::update() 
{
    /* wmove(window, 0, 0); */
    /* wclrtoeol(window); */
    /* wprintw(window, "st: %d", nlines); */
    mvwprintw(window, 0, 0, "st: %d", nlines);
    for (int i = 1; i < nlines; i++)
    {
        /* wmove(window, i, 0); */
        /* wclrtoeol(window); */
        /* wprintw(window, "%d", i); */
        mvwprintw(window, i, 0, "%d", i);
    }
    /* wmove(window, nlines-1, 0); */
    /* wclrtoeol(window); */
    /* wprintw(window, "end"); */
    /* mvwprintw(window, nlines-1, 0, "end"); */
    Window::update();
}
void EmptyWindow::processKey(int ch) 
{
    Window::processKey(ch);
}

void EmptyWindow::afterReshape() {}


MetaWindow::MetaWindow(int startY, int startX, int nlines, int ncols) :
    Window(startY, startX, nlines, ncols)
{
    selectedWindow = windows.begin();
}

weak_ptr<Window> MetaWindow::getSelected() 
{
    if(selectedWindow != windows.end())
    {
        return (*selectedWindow)->getSelected();
    }
    else
    {
        return {};
    }
}

void MetaWindow::addWindow(shared_ptr<Window> win)
{
    float sum = 0;
    for(auto &e : quotients)
    {
        e *= static_cast<float>(quotients.size())/(quotients.size()+1);
        sum += e;
    }

    win->parent = shared_from_this();
    windows.push_back(win);
    quotients.push_back(1-sum);
    recalculateSizes();

    update();
}

ColumnWindow::ColumnWindow(int startY, int startX, int nlines, int ncols) :
    MetaWindow(startY, startX, nlines, ncols) {}

void ColumnWindow::update()
{
    int border = 0;
    for (auto &win : windows)
    {
        win->update();
        border += win->nlines;
        if (border < nlines)
        {
            for (int i = 0; i < ncols; i++)
            {
                mvwprintw(window, border, i, "-");
            }
        }
        border++;
    }

    Window::update();
}

void ColumnWindow::processKey(int ch)
{
    switch (ch)
    {
        case 'j':
            ++selectedWindow;
            if(selectedWindow == windows.end())
            {
               --selectedWindow;
               if (auto locked = parent.lock())
               {
                    locked->processKey(ch);
               }
            }
            break;

        case 'k':
            if(selectedWindow == windows.begin())
            {
                if (auto locked = parent.lock())
                {
                    locked->processKey(ch);
                }
            }
            else
            {
                --selectedWindow;
            }
            break;

        case 'h':
        case 'l':
            if (auto locked = parent.lock())
            {
                locked->processKey(ch);
            }
            break;

        default:
            (*selectedWindow)->processKey(ch);
            break;
    }
}

void ColumnWindow::afterReshape()
{
    recalculateSizes();
    update();
}

void ColumnWindow::recalculateSizes() 
{
    int y = 0;
    auto w = windows.begin();
    for(auto e = quotients.begin(); e != quotients.end() && w != windows.end(); ++e, ++w)
    {
        int winLines = (*e) * nlines;
        if(y + winLines > nlines)
        {
            winLines = nlines - y;
        }
        (*w)->reshapeWindow(y, startX, winLines, ncols);
        y += (*w)->nlines + 1;
    }
}


LineWindow::LineWindow(int startY, int startX, int nlines, int ncols) :
    MetaWindow(startY, startX, nlines, ncols) {}

void LineWindow::update()
{
    int border = 0;
    for (auto &win : windows)
    {
        win->update();
        border += win->ncols;
        if (border < ncols)
        {
            for (int i = 0; i < nlines; i++)
            {
                mvwprintw(window, i, border, "|");
            }
        }
        border++;
    }

    Window::update();
}

void LineWindow::processKey(int ch)
{
    switch (ch)
    {
        case 'l':
            ++selectedWindow;
            if(selectedWindow == windows.end())
            {
                --selectedWindow;
                if (auto locked = parent.lock())
                {
                    locked->processKey(ch);
                }
            }
            break;

        case 'h':
            if(selectedWindow == windows.begin())
            {
                if (auto locked = parent.lock())
                {
                    locked->processKey(ch);
                }
            }
            else
            {
                --selectedWindow;
            }
            break;

        case 'j':
        case 'k':
            if (auto locked = parent.lock())
            {
                locked->processKey(ch);
            }
            break;

        default:
            (*selectedWindow)->processKey(ch);
            break;
    }
}

void LineWindow::afterReshape()
{
    recalculateSizes();
    update();
}

void LineWindow::recalculateSizes() 
{
    int x = 0;
    auto w = windows.begin();
    for(auto e = quotients.begin(); e != quotients.end() && w != windows.end(); ++e, ++w)
    {
        int winCols = (*e) * ncols;
        if(x + winCols > ncols)
        {
            winCols = ncols - x;
        }
        (*w)->reshapeWindow(startY, x, nlines, winCols);
        x += (*w)->ncols + 1;
    }
}


    template< typename DequeType >
void DequeListingWindow<DequeType>::afterReshape()
{
    screenStart = cursorPos;
}

template< typename DequeType >
DequeListingWindow<DequeType>::DequeListingWindow(int startY, int startX, int nlines, int ncols, deque<DequeType> data) :
    Window(startY, startX, nlines, ncols), data(data)
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
    for (int i = 0; i < nlines; i++)
    {
        if (p == data.end())
        {
            break;
        }
        wmove(window, i, 1);
        wclrtoeol(window);
        if (p == cursorPos)
        {
            wattron(window, A_REVERSE);

            if (this == mainWindow->getSelected().lock().get())
            {
                wattron(window, A_BOLD);
            }
        }
        wprintw(window, "%s", getName(p).c_str());
        wattroff(window, A_REVERSE | A_BOLD);
        ++p;
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
                    --screenStart;
                }
                --cursorPos;
                select();
            }
            break;

        case KEY_DOWN:
            if (cursorPos != data.end()-1)
            {
                if (cursorPos == screenStart+nlines-1)
                {
                    ++screenStart;
                }
                ++cursorPos;
                select();
            }
            break;

        case 'h':
        case 'j':
        case 'k':
        case 'l':
            Window::processKey(ch);
            break;
            
        default:
            press(ch);
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

void TracksListingWindow::press(int key)
{
    set<PlaybackOption> options;
    switch (key)
    {
        case 's':
            options.insert(PlaybackOption::queue);
        case 'S':
            startPlayback(*cursorPos, options);
            break;

        default:
            break;
    }
}

void AlbumsListingWindow::select()
{
    tracksWindow->assignNewDeque((*cursorPos)->getTracks());
}

void AlbumsListingWindow::press(int key)
{
    set<PlaybackOption> options = {PlaybackOption::shuffle};
    switch (key)
    {
        case 's':
            options.insert(PlaybackOption::queue);
        case 'S':
            startPlayback(*cursorPos, options);
            break;

        default:
            break;
    }
}

void ArtistsListingWindow::select()
{
    albumsWindow->assignNewDeque((*cursorPos)->getAlbums());
}

void ArtistsListingWindow::press(int key)
{
    set<PlaybackOption> options = {PlaybackOption::shuffle};
    switch (key)
    {
        case 's':
            options.insert(PlaybackOption::queue);
        case 'S':
            startPlayback(*cursorPos, options);
            break;

        default:
            break;
    }
}

PlaybackControlWindow::PlaybackControlWindow(int startY, int startX, int nlines, int ncols) :
    Window(startY, startX, nlines, ncols),
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
            Window::processKey(ch);
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
        for (int i = 0; i < nlines; i++)
        {
            wmove(window, i, 1);
            wclrtoeol(window);
        }

        if (playbackInProcess() && play::NowPlaying::track)
        {
            wmove(window, 0, 1);
            wclrtoeol(window);

            wattron(window, A_BOLD);
            wprintw(window, "Track: ");
            wattroff(window, A_BOLD);

            wprintw(window, "%s", play::NowPlaying::track->name.c_str());


            wmove(window, 1, 1);
            wclrtoeol(window);

            wattron(window, A_BOLD);
            if (play::playbackPause)
            {
                wprintw(window, "Paused:  ");
            }
            else
            {
                wprintw(window, "Playing: ");
            }
            wattroff(window, A_BOLD);

            {
                //FIXME: stops showing correct time after two minutes

                using namespace chrono;

                shared_ptr<Track>& track = play::NowPlaying::track;

                seconds sec(play::NowPlaying::sample / track->codecContext->sample_rate);
                minutes min(duration_cast<minutes>(sec));

                wprintw(window, "%d:%d / %d:%d", min, sec, duration_cast<minutes>(track->duration), duration_cast<seconds>(track->duration).count()%60);
            }


            wmove(window, 2, 1);
            wclrtoeol(window);

            wattron(window, A_BOLD);
            wprintw(window, "Queued: ");
            wattroff(window, A_BOLD);
            wprintw(window, "%d", play::playbackQueue.size());


            Window::update();
            this_thread::sleep_for(chrono::milliseconds(100));
        }
        else
        {
            this_thread::sleep_for(chrono::milliseconds(500));
        }
    }
}

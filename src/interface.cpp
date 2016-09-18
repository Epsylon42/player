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

#include <stdio.h>

using namespace std;
using namespace interface;
using namespace data;

namespace play = playback;
using namespace playback;

recursive_mutex interface::interfaceMutex;
int interface::sizeX;
int interface::sizeY;
shared_ptr<ColumnWindow> interface::mainWindow;

list<shared_ptr<Artist>> interface::DataLists::artistsList;
list<shared_ptr<Album>>  interface::DataLists::albumsList;
list<shared_ptr<Track>>  interface::DataLists::tracksList;
bool interface::DataLists::artistsUpdated= true;
bool interface::DataLists::albumsUpdated = true;
bool interface::DataLists::tracksUpdated = true;

bool doShuffle = false;

void initInterface()
{
    initscr();
    halfdelay(10);
    curs_set(FALSE);
    noecho();
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);

    sizeX = getmaxx(stdscr);
    sizeY = getmaxy(stdscr);

    mainWindow = make_shared<ColumnWindow>(0, 0, sizeY, sizeX);

    DataLists::artistsList = data::getArtists();

    if (!data::allArtists->allAlbums->tracks.empty())
    {
        DataLists::albumsList = data::allArtists->getAlbums();
    }
    else
    {
        DataLists::albumsList = {};
    }

    if (!DataLists::tracksList.empty())
    {
        DataLists::tracksList = DataLists::albumsList.front()->getTracks();
    }
    else
    {
        DataLists::tracksList = {};
    }

    auto tracksWindow = make_shared<TracksListingWindow>(0, 0, 0, 0, DataLists::tracksList, DataLists::tracksUpdated);
    auto albumsWindow = make_shared<AlbumsListingWindow>(0, 0, 0, 0, DataLists::albumsList, DataLists::albumsUpdated);
    static bool artistUpd = true; //TODO: replace this with actual update flag
    auto artistsWindow = make_shared<ArtistsListingWindow>(0, 0, 0, 0, DataLists::artistsList, DataLists::artistsUpdated);
    auto playbackWindow = make_shared<PlaybackControlWindow>(0, 0, 0, 0);

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
    mainWindow.reset();
    DataLists::albumsList.clear();
    DataLists::tracksList.clear();

    endwin();
}

void updateWindows()
{
    //log("updwindows");
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

    if (auto selectedWindow = mainWindow->getSelected().lock())
    {
        auto& nwindow = selectedWindow->nwindow;
        int x = nwindow->_begx-1;
        int y = nwindow->_begy-1;
        int mx = nwindow->_maxx+nwindow->_begx+1;
        int my = nwindow->_maxy+nwindow->_begy+1;

        unique_ptr<WINDOW> stdscr(::stdscr);

        if (has_colors())
        {
            wattron(stdscr, COLOR_PAIR(1));
        }
        else
        {
            wattron(stdscr, A_REVERSE);
        }

        for (int i = 1; i <= getmaxx(nwindow.get()); i++)
        {
            if (y >= 0)
            {
                wmove(stdscr, y, x+i);
                wprintw(stdscr, "-");
            }
            if (my < getmaxy(stdscr.get()))
            {
                wmove(stdscr, my, x+i);
                wprintw(stdscr, "-");
            }
        }

        for (int i = 1; i <= getmaxy(nwindow.get()); i++)
        {
            if (x >= 0)
            {
                wmove(stdscr, y+i, x);
                wprintw(stdscr, "|");
            }
            if (mx < getmaxx(stdscr.get()))
            {
                wmove(stdscr, y+i, mx);
                wprintw(stdscr, "|");
            }
        }

        if (x >= 0 && y >= 0)
        {
            wmove(stdscr, y, x);
            wprintw(stdscr, "+");
        }

        if (mx < getmaxx(stdscr.get()) && y >= 0)
        {
            wmove(stdscr, y, mx);
            wprintw(stdscr, "+");
        }

        if (x >= 0 && my < getmaxy(stdscr.get()))
        {
            wmove(stdscr, my, x);
            wprintw(stdscr, "+");
        }

        if(mx < getmaxx(stdscr.get()) && my < getmaxy(stdscr.get()))
        {
            wmove(stdscr, my, mx);
            wprintw(stdscr, "+");
        }

        if (has_colors())
        {
            wattroff(stdscr, COLOR_PAIR(1));
        }
        else
        {
            wattroff(stdscr, A_REVERSE);
        }

        interfaceMutex.lock();
        wrefresh(stdscr);
        interfaceMutex.unlock();

        stdscr.release();

    }
}

void fullRefresh()
{
    //log("fullrefresh");
    clear();
    wclear(mainWindow->nwindow);
    refresh();

    mainWindow->update();
}

//returns false if pressed exit key
bool readKey()
{ 
    //log("readkey");
    int ch = wgetch(mainWindow->nwindow.get());
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
        case 'e': // (E)nd
            sendPlaybackCommand(new CommandSTOP());
            break;
        case 'E':
            sendPlaybackCommand(new CommandSTOPALL());
            break;
        case '<':
            sendPlaybackCommand(new CommandPREVIOUS());
            break;
        case '>':
            sendPlaybackCommand(new CommandNEXT());
            break;
        case 't': //(t)oggle shuffle
            doShuffle = !doShuffle;
            break;
        default:
            {
                if (auto locked = mainWindow->getSelected().lock())
                {
                    //log("processkey_call");
                    locked->processKey(ch);
                }
                else
                {
                    //log("lock failed");
                }
            }
    }
    return true;
}

Window::Window(int startY, int startX, int nlines, int ncols) :
    startX(startX), startY(startY), nlines(nlines), ncols(ncols),
    nwindow(newwin(nlines, ncols, startY, startX)),
    parent()
{
    // nodelay(window, true);
    keypad(nwindow.get(), true);
}

weak_ptr<Window> Window::getSelected()
{
    //log("getselected_this");
    return shared_from_this();
}

Window::~Window()
{
    delwin(nwindow.release());
}

// TODO: add option without update, clear, etc.
void Window::reshapeWindow(int newY, int newX, int newLines, int newColumns)
{
    startX = newX;
    startY = newY;
    nlines = newLines;
    ncols = newColumns;

    wresize(nwindow, nlines, ncols);
    mvwin(nwindow, startY, startX);

    afterReshape();
    wclear(nwindow);
    update();
}

void Window::update()
{
    interfaceMutex.lock();
    wrefresh(nwindow);
    interfaceMutex.unlock();
}

void Window::processKey(int ch)
{
    //log("processkey");
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
    mvwprintw(nwindow, 0, 0, "st: %d", nlines);
    for (int i = 1; i < nlines; i++)
    {
        /* wmove(window, i, 0); */
        /* wclrtoeol(window); */
        /* wprintw(window, "%d", i); */
        mvwprintw(nwindow, i, 0, "%d", i);
    }
    /* wmove(window, nlines-1, 0); */
    /* wclrtoeol(window); */
    /* wprintw(window, "end"); */
    /* mvwprintw(window, nlines-1, 0, "end"); */
    Window::update();
}
void EmptyWindow::processKey(int ch) 
{
    //log("processkey_empty");
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
    //log("getselected_meta");
    if(selectedWindow != windows.end())
    {
        //log("getselected_meta success");
        return (*selectedWindow)->getSelected();
    }
    else
    {
        //log("return {}");
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

    if (selectedWindow == windows.end())
    {
        selectedWindow = windows.begin();
    }

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
                mvwprintw(nwindow, border, i, "-");
            }
        }
        border++;
    }

    Window::update();
}

void ColumnWindow::processKey(int ch)
{
    //log("processkey_column");
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
                mvwprintw(nwindow, i, border, "|");
            }
        }
        border++;
    }

    Window::update();
}

void LineWindow::processKey(int ch)
{
    //log("processkey_line");
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


    template< typename ListType >
void ListListingWindow<ListType>::afterReshape()
{
    updateScreenIters();
}

    template< typename ListType >
void ListListingWindow<ListType>::updateScreenIters()
{
    screenStart = cursorPos;
    screenEnd = screenStart;
    for (int i = 0; i < nlines; i++)
    {
        if (screenEnd == data.end())
        {
            break;
        }
        ++screenEnd;
    }
}

template< typename ListType >
ListListingWindow<ListType>::ListListingWindow(int startY, int startX, int nlines, int ncols, list<ListType>& data, bool& dataUpdated) :
    Window(startY, startX, nlines, ncols), data(data), dataUpdated(dataUpdated)
{
    cursorPos   = this->data.begin();
    screenStart = this->data.begin();
}

    template< typename ListType >
ListListingWindow<ListType>::~ListListingWindow()
{

}

    template< typename ListType >
void ListListingWindow<ListType>::update()
{
    if (data.empty())
    {
        return;
    }

    if (dataUpdated)
    {
        cursorPos = data.begin();
        updateScreenIters();
        dataUpdated = false;
    }

    for (int i = 0; i < nlines; i++)
    {
        wmove(nwindow, i, 1);
        wclrtoeol(nwindow);
    }

    int line = 0;
    for (auto iter = screenStart; iter != screenEnd && iter != data.end(); ++iter)
    {
        wmove(nwindow, line++, 1);
        wclrtoeol(nwindow);
        if (iter == cursorPos)
        {
            wattron(nwindow, A_REVERSE);

            if (this == mainWindow->getSelected().lock().get())
            {
                wattron(nwindow, A_BOLD);
            }
        }
        wprintw(nwindow, "%s", getName(iter).c_str());
        wattroff(nwindow, A_REVERSE | A_BOLD);
    }

    Window::update();
}

    template< typename ListType >
void ListListingWindow<ListType>::processKey(int ch)
{
    //log("processkey");
    switch (ch)
    {
        case KEY_UP:
            if (cursorPos != data.begin())
            {
                if (cursorPos == screenStart)
                {
                    --screenStart;
                    --screenEnd;
                }
                --cursorPos;
                select();
            }
            break;

        case KEY_DOWN:
            if (cursorPos != prev(data.end()))
            {
                if (cursorPos == prev(screenEnd))
                {
                    ++screenStart;
                    ++screenEnd;
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

template< typename ListType >
string MediaListingWindow<ListType>::getName(typename list<ListType>::iterator iter) const
{
    return (*iter)->name;
}

void TracksListingWindow::select()
{

}

void TracksListingWindow::press(int key)
{
    if (data.empty())
    {
        return;
    }

    set<PlaybackOption> options;
    bool play = true;
    switch (key)
    {
        case 's':
            options.insert(PlaybackOption::stopCurrentPlayback);
            break;
        case 'S':
            options.insert(PlaybackOption::playAfterEverything);
            break;
        case 'd':
            options.insert(PlaybackOption::playAfterCurrentTrack);
            break;
        case 'D':
            options.insert(PlaybackOption::playAfterCurrentList);
            break;
        case 'f':
            options.insert(PlaybackOption::suspendCurrentPlayback);
            break;

        default:
            play = false;
            break;
    }
    if (play)
    {
        startPlayback(*cursorPos, options);
    }
}

void AlbumsListingWindow::select()
{
    if (data.empty())
    {
        return;
    }

    DataLists::tracksList = (*cursorPos)->getTracks();
    DataLists::tracksUpdated = true;
}

void AlbumsListingWindow::press(int key)
{
    if (data.empty())
    {
        return;
    }

    set<PlaybackOption> options;
    if (doShuffle)
    {
        options.insert(PlaybackOption::shuffle);
    }

    bool play = true;
    switch (key)
    {
        case 's':
            options.insert(PlaybackOption::stopCurrentPlayback);
            break;
        case 'S':
            options.insert(PlaybackOption::playAfterEverything);
            break;
        case 'd':
            options.insert(PlaybackOption::playAfterCurrentTrack);
            break;
        case 'D':
            options.insert(PlaybackOption::playAfterCurrentList);
            break;
        case 'f':
            options.insert(PlaybackOption::suspendCurrentPlayback);
            break;

        default:
            play = false;
            break;
    }
    if (play)
    {
        startPlayback(*cursorPos, options);
    }
}

void ArtistsListingWindow::select()
{
    if (data.empty())
    {
        return;
    }

    DataLists::albumsList = (*cursorPos)->getAlbums();
    DataLists::tracksList = DataLists::albumsList.front()->getTracks();
    DataLists::albumsUpdated = true;
    DataLists::tracksUpdated = true;
}

void ArtistsListingWindow::press(int key)
{
    if (data.empty())
    {
        return;
    }

    set<PlaybackOption> options;
    if (doShuffle)
    {
        options.insert(PlaybackOption::shuffle);
    }

    bool play = true;
    switch (key)
    {
        case 's':
            options.insert(PlaybackOption::stopCurrentPlayback);
            break;
        case 'S':
            options.insert(PlaybackOption::playAfterEverything);
            break;
        case 'd':
            options.insert(PlaybackOption::playAfterCurrentTrack);
            break;
        case 'D':
            options.insert(PlaybackOption::playAfterCurrentList);
            break;
        case 'f':
            options.insert(PlaybackOption::suspendCurrentPlayback);
            break;

        default:
            play = false;
            break;
    }
    if (play)
    {
        startPlayback(*cursorPos, options);
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
    //log("processkey_playback_c");
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
            wmove(nwindow, i, 1);
            wclrtoeol(nwindow);
        }

        if (playbackInProcess() && play::NowPlaying::track)
        {
            if (playbackInProcess())
            {
                wmove(nwindow, 0, 1);
                wattron(nwindow, A_BOLD);
                wprintw(nwindow, "Track: ");
                wattroff(nwindow, A_BOLD);

                wprintw(nwindow, "%s", play::NowPlaying::track->name.c_str());

                wmove(nwindow, 1, 1);
                wclrtoeol(nwindow);

                wattron(nwindow, A_BOLD);
                if (play::playbackPause)
                {
                    wprintw(nwindow, "Paused:  ");
                }
                else
                {
                    wprintw(nwindow, "Playing: ");
                }
                wattroff(nwindow, A_BOLD);
            }

            {
                char* duration = new char[10];
                char* current = new char[10];

                snprintf(duration, 10, "%" GST_TIME_FORMAT, GST_TIME_ARGS(play::NowPlaying::duration));
                snprintf(current, 10, "%" GST_TIME_FORMAT, GST_TIME_ARGS(play::NowPlaying::current));
                replace(duration, duration+10, '.', '\0');
                replace(current, current+10, '.', '\0');
                wprintw(nwindow, "%s / %s", current, duration);

                delete [] duration;
                delete [] current;
            }

        }

        wmove(nwindow, nlines-1, ncols-1);
        if (doShuffle)
        {
            wattron(nwindow, A_REVERSE);
            wprintw(nwindow, "S");
            wattroff(nwindow, A_REVERSE);
        }
        else
        {
            wprintw(nwindow, "S");
        }

        Window::update();

        this_thread::sleep_for(100ms);
    }
}

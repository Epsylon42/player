#include "interface.hpp"
#include "play.hpp"

#include <unistd.h>
#include <memory>

using namespace std;

int sizeX;
int sizeY;
deque<shared_ptr<Window> > windows;
shared_ptr<DequeListingWindow<shared_ptr<Artist>>> artistsWindow;
shared_ptr<DequeListingWindow<shared_ptr<Album>>>  albumsWindow;
shared_ptr<TracksListingWindow>                      tracksWindow;
shared_ptr<PlaybackControlWindow>                    playbackWindow;
shared_ptr<Window> selectedWindow;


void initInterface()
{
   initscr();
   curs_set(FALSE);
   noecho();

   ::sizeX = getmaxx(stdscr);
   ::sizeY = getmaxy(stdscr);

   
   ::tracksWindow = make_shared<TracksListingWindow>(::sizeY/2+1, 0, ::sizeY/2-1+(::sizeY%2), ::sizeX, BORDERS_ALL, getTracks());
   ::albumsWindow = make_shared<DequeListingWindow<shared_ptr<Album>>>(0, ::sizeX/2, ::sizeY/2+1, ::sizeX/2+(::sizeX%2), BORDERS_ALL, getAlbums(),
						 [](shared_ptr<Album> album)
						 {
						    startPlayback(album, PLAYBACK_OPTION_SHUFFLE);
						 },
						 [](shared_ptr<Album> album)
						 {
						    ::tracksWindow->assignNewDeque(album->getTracks());
						 });
   ::artistsWindow = make_shared<DequeListingWindow<shared_ptr<Artist>>>(5, 0, ::sizeY/2+1-5, ::sizeX/2, BORDERS_ALL, &::artistsDeque,
						   [](shared_ptr<Artist> artist)
						   {
						      startPlayback(artist, PLAYBACK_OPTION_SHUFFLE);
						   },
						   [](shared_ptr<Artist> artist)
						   {
						      ::albumsWindow->assignNewDeque(artist->getAlbums());
						   });
   ::playbackWindow = make_shared<PlaybackControlWindow>(0, 0, 5, ::sizeX/2, BORDERS_ALL);
   
   ::selectedWindow =  ::tracksWindow;
   ::windows.push_back(::artistsWindow);
   ::windows.push_back(::albumsWindow);
   ::windows.push_back(::tracksWindow);
   ::windows.push_back(::playbackWindow);
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
   ::selectedWindow.reset();
   ::tracksWindow.reset();
   ::albumsWindow.reset();
   ::artistsWindow.reset();
   ::playbackWindow.reset();
   ::windows.clear();

   endwin();
}

void updateWindows()
{
   usleep(10000); 
   int newSizeX = getmaxx(stdscr);
   int newSizeY = getmaxy(stdscr);
   if (newSizeX != ::sizeX || newSizeY != ::sizeY)
   {
      ::sizeX = newSizeX;
      ::sizeY = newSizeY;
      ::tracksWindow->reshapeWindow(::sizeY/2+1, 0, ::sizeY/2-1+(::sizeY%2), ::sizeX);
      ::albumsWindow->reshapeWindow(0, ::sizeX/2, ::sizeY/2+1, ::sizeX/2+(::sizeX%2));
      ::artistsWindow->reshapeWindow(5, 0, ::sizeY/2+1-5, ::sizeX/2);
      ::playbackWindow->reshapeWindow(0, 0, 5, ::sizeX/2);
      clear();
      refresh();
   }
   
   for (auto window : ::windows)
   {
      window->update(true);
   }
}

void fullRefresh()
{
   clear();
   for (auto window : ::windows)
   {
      wclear(window->window);
   }
   refresh();
}

//returns false if pressed exit key
bool readKey()
{
   int ch = wgetch(::selectedWindow->window);
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
	 ::selectedWindow = ::artistsWindow;
	 break;
      case 'l': // a(L)bums
	 ::selectedWindow = ::albumsWindow;
	 break;
      case 't':
	 ::selectedWindow = ::tracksWindow;
	 break;
      case 'P':
	 ::selectedWindow = ::playbackWindow;
	 break;
      case ' ':
	 ::playbackControl.push(new Command(PLAYBACK_COMMAND_TOGGLE));
	 break;
      case 'E': // (E)nd
	 ::playbackControl.push(new Command(PLAYBACK_COMMAND_STOP));
	 break;
      case 'p':
	 ::playbackControl.push(new Command(PLAYBACK_COMMAND_PREV));
	 break;
      case 'n':
	 ::playbackControl.push(new Command(PLAYBACK_COMMAND_NEXT));
	 break;
      default:
	 ::selectedWindow->processKey(ch);
   }
   return true;
}

Window::Window(int startY, int startX, int nlines, int ncols, char borders) :
   startY(startY), startX(startX), nlines(nlines), ncols(ncols)
{
   window = newwin(nlines, ncols, startY, startX);
   keypad(window, true);
   nodelay(window, true); //TODO: remove this line and make interface self-update whithout key pressing

   borderTop    = borders & BORDER_TOP;
   borderLeft   = borders & BORDER_LEFT;
   borderRight  = borders & BORDER_RIGHT;
   borderBottom = borders & BORDER_BOTTOM;
}

Window::~Window()
{
   delwin(window);
}

void Window::displayBorders()
{
   char frame;
   if (this == ::selectedWindow.get())
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

   wclear(window);
   wrefresh(window);
   afterReshape();
}

void Window::update(bool isSelected)
{
   displayBorders();
   
   for (auto win : subWindows)
   {
      win->update(win == selectedSubWindow);
   }

   wrefresh(window);
}


template< typename DequeType >
void DequeListingWindow<DequeType>::afterReshape()
{
   screenStart = cursorPos;
}

template< typename DequeType >
DequeListingWindow<DequeType>::DequeListingWindow(int startY, int startX, int nlines, int ncols, char borders,
						  deque<DequeType>* data,
						  void (*select)(DequeType),
						  void (*allocate)(DequeType)) :
   Window(startY, startX, nlines, ncols, borders), data(data), select(select), allocate(allocate)
{
   cursorPos   = data->begin();
   screenStart = data->begin();
   if (allocate != nullptr)
   {
      (allocate)(*cursorPos);
   }
}

template< typename DequeType >
DequeListingWindow<DequeType>::~DequeListingWindow()
{
   data->clear();
}

template< typename DequeType >
void DequeListingWindow<DequeType>::update(bool isSelected)
{
   auto p = screenStart;
   for (int i = 0; i < nlines-2; i++)
   {
      if (p == data->end())
      {
	 break;
      }
      wmove(window, i+1, 1);
      wclrtoeol(window);
      if (p == cursorPos)
      {
	 wattron(window, A_REVERSE);
	 if (this == ::selectedWindow.get())
	 {
	    wattron(window, A_BOLD);
	 }
      }
      wprintw(window, "%s", (*p)->name.c_str());
      wattroff(window, A_REVERSE | A_BOLD);
      p++;
   }

   Window::update(isSelected);
}

template< typename DequeType >
void DequeListingWindow<DequeType>::processKey(int ch)
{
   switch (ch)
   {
      case KEY_UP:
	 if (cursorPos != data->begin())
	 {
	    if (cursorPos == screenStart)
	    {
	       screenStart--;
	    }
	    cursorPos--;
	    if (allocate != nullptr)
	    {
	       (allocate)(*cursorPos);
	    }
	 }
	 break;
      case KEY_DOWN:
	 if (cursorPos != data->end()-1)
	 {
	    if (cursorPos == screenStart+nlines-3)
	    {
	       screenStart++;
	    }
	    cursorPos++;
	    if (allocate != nullptr)
	    {
	       (allocate)(*cursorPos);
	    }
	 }
	 break;
      case 'S':
	 if (select != nullptr)
	 {
	    (select)(*cursorPos);
	 }
      default:
	 break;
   }
}

template< typename DequeType >
void DequeListingWindow<DequeType>::assignNewDeque(deque<DequeType>* newDeque)
{
   data->clear();
   delete data;
   
   data = newDeque;
   cursorPos   = data->begin();
   screenStart = data->begin();
   if (allocate != nullptr)
   {
      (allocate)(*cursorPos);
   }
   
   wclear(window);
   update(false);
}


TracksListingWindow::TracksListingWindow(int startY, int startX, int nlines, int ncols, char borders, deque<shared_ptr<Track> >* data) :
   DequeListingWindow<shared_ptr<Track>>(startY, startX, nlines, ncols, borders, data, [](shared_ptr<Track> track)
			      {
				 startPlayback(track);
			      }, nullptr) {}


PlaybackControlWindow::PlaybackControlWindow(int startY, int startX, int nlines, int ncols, char borders) :
   Window(startY, startX, nlines, ncols, borders) {}

void PlaybackControlWindow::update(bool isSelected)
{
   for (int i = 1; i < nlines-1; i++)
   {
      wmove(window, i, 1);
      wclrtoeol(window);
   }
   if (NowPlaying::playing == true)
   {
      wmove(window, 1, 1);
      wattron(window, A_BOLD);
      wprintw(window, "Track: ");
      
      wattroff(window, A_BOLD);
      wprintw(window, "%s", NowPlaying::track->name.c_str());
      wattron(window, A_BOLD);
      
      wmove(window, 2, 1);
      if (::playbackPause)
      {
	 wprintw(window, "paused:  ");
      }
      else
      {
	 wprintw(window, "playing: ");
      }
      wattroff(window, A_BOLD);
      int time = NowPlaying::frame;
      wprintw(window, "%d", time);
   }
   
   Window::update(isSelected);
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

#include "interface.h"
#include "play.h"

#include <unistd.h>

int sizeX;
int sizeY;
std::deque<Window*> windows;
DequeListingWindow<Artist*>* artistsWindow;
DequeListingWindow<Album*>* albumsWindow;
DequeListingWindow<Track*>* tracksWindow;
Window* selectedWindow;


void initInterface()
{
   initscr();
   curs_set(FALSE);
   noecho();

   sizeX = getmaxx(stdscr);
   sizeY = getmaxy(stdscr);

   tracksWindow = new DequeListingWindow<Track*>(sizeY/2+1, 0, sizeY/2-1+(sizeY%2), sizeX, BORDERS_ALL, getTracks(), [](Track* track)
						  {
						     play(track);
						  });
   albumsWindow = new DequeListingWindow<Album*>(0, sizeX/2, sizeY/2+1, sizeX/2+(sizeX%2), BORDERS_ALL, getAlbums(), NULL, [](Album* album)
						 {
						    tracksWindow->assignNewDeque(album->getTracks());
						 });
   artistsWindow = new DequeListingWindow<Artist*>(0, 0, sizeY/2+1, sizeX/2, BORDERS_ALL, &artistsDeque, NULL, [](Artist* artist)
						   {
						      albumsWindow->assignNewDeque(artist->getAlbums());
						   });
   
   selectedWindow = tracksWindow;
   windows.push_back(artistsWindow);
   windows.push_back(albumsWindow);
   windows.push_back(tracksWindow);
}

void updateWindows()
{
   usleep(10000);
   
   int newSizeX = getmaxx(stdscr);
   int newSizeY = getmaxy(stdscr);
   if (newSizeX != sizeX || newSizeY != sizeY)
   {
      sizeX = newSizeX;
      sizeY = newSizeY;
      artistsWindow->reshapeWindow(0, 0, sizeY/2+1, sizeX/2);
      albumsWindow->reshapeWindow(0, sizeX/2, sizeY/2+1, sizeX/2+(sizeX%2));
      tracksWindow->reshapeWindow(sizeY/2+1, 0, sizeY/2-1+(sizeY%2), sizeX);
      clear();
      refresh();
   }
   
   for (auto window : windows)
   {
      window->update(true);
   }
   readKey();
}

void readKey()
{
   int ch = wgetch(selectedWindow->window);
   switch (ch)
   {
      case ERR:
	 break;
      case 'A':
	 selectedWindow = artistsWindow;
	 break;
      case 'L': // 'L' stands for aLbums
	 selectedWindow = albumsWindow;
	 break;
      case 'T':
	 selectedWindow = tracksWindow;
	 break;
      default:
	 selectedWindow->processKey(ch);
   }
}

Window::Window(int startY, int startX, int nlines, int ncols, char borders) :
   startY(startY), startX(startX), nlines(nlines), ncols(ncols)
{
   window = newwin(nlines, ncols, startY, startX);
   keypad(window, true);
   nodelay(window, true);

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
   if (this == selectedWindow)
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
						  std::deque<DequeType>* deque,
						  void (*select)(DequeType),
						  void (*allocate)(DequeType)) :
   Window(startY, startX, nlines, ncols, borders), deque(deque), select(select), allocate(allocate)
{
   cursorPos = deque->begin();
   screenStart = deque->begin();
   if (allocate != NULL)
   {
      (allocate)(*cursorPos);
   }
}

template< typename DequeType >
void DequeListingWindow<DequeType>::update(bool isSelected)
{
   auto p = screenStart;
   for (int i = 0; i < nlines-2; i++)
   {
      if (p == deque->end())
      {
	 break;
      }
      wmove(window, i+1, 1);
      wclrtoeol(window);
      if (p == cursorPos)
      {
	 wattron(window, A_REVERSE);
	 if (this == selectedWindow)
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
	 if (cursorPos != deque->begin())
	 {
	    if (cursorPos == screenStart)
	    {
	       screenStart--;
	    }
	    cursorPos--;
	    if (allocate != NULL)
	    {
	       (allocate)(*cursorPos);
	    }
	 }
	 break;
      case KEY_DOWN:
	 if (cursorPos != deque->end()-1)
	 {
	    if (cursorPos == screenStart+nlines-3)
	    {
	       screenStart++;
	    }
	    cursorPos++;
	    if (allocate != NULL)
	    {
	       (allocate)(*cursorPos);
	    }
	 }
	 break;
      case 'S':
	 if (select != NULL)
	 {
	    (select)(*cursorPos);
	 }
      default:
	 break;
   }
}

template< typename DequeType >
void DequeListingWindow<DequeType>::assignNewDeque(std::deque<DequeType>* newDeque)
{
   deque->clear();
   delete deque;
   
   deque = newDeque;
   cursorPos = deque->begin();
   screenStart = deque->begin();
   if (allocate != NULL)
   {
      (allocate)(*cursorPos);
   }
   
   wclear(window);
   update(false);
}

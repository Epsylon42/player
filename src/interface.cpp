#include "interface.h"
#include "play.h"

#include <unistd.h>

int sizeX;
int sizeY;
std::vector<Window*> windows;
VectorListingWindow<Artist*>* artistsWindow;
VectorListingWindow<Album*>* albumsWindow;
VectorListingWindow<Track*>* tracksWindow;
Window* selectedWindow;


void initInterface()
{
   initscr();
   curs_set(FALSE);
   noecho();

   sizeX = getmaxx(stdscr);
   sizeY = getmaxy(stdscr);

   tracksWindow = new VectorListingWindow<Track*>(sizeY/2+1, 0, sizeY/2-1+(sizeY%2), sizeX, BORDERS_ALL, getTracks(), [](Track* track)
						  {
						     play(track);
						  });
   albumsWindow = new VectorListingWindow<Album*>(0, sizeX/2, sizeY/2+1, sizeX/2+(sizeX%2), BORDERS_ALL, getAlbums(), [](Album* album)
						  {
						     tracksWindow->assignNewVector(album->getTracks());
						  });
   artistsWindow = new VectorListingWindow<Artist*>(0, 0, sizeY/2+1, sizeX/2, BORDERS_ALL, &artists, [](Artist* artist)
						    {
						       albumsWindow->assignNewVector(artist->getAlbums());
						       albumsWindow->processKey('S'); //TODO: replace this with something more... good
						    });

   selectedWindow = tracksWindow;
   windows.push_back(artistsWindow);
   windows.push_back(albumsWindow);
   windows.push_back(tracksWindow);
   artistsWindow->processKey('S'); //TODO: replace also this
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

template< typename VectorType >
void VectorListingWindow<VectorType>::afterReshape()
{
   screenStart = cursorPos;
}

template< typename VectorType >
VectorListingWindow<VectorType>::VectorListingWindow(int startY, int startX, int nlines, int ncols, char borders, std::vector<VectorType>* vector, void (*select)(VectorType)) :
   Window(startY, startX, nlines, ncols, borders), vector(vector), select(select)
{
   cursorPos = vector->begin();
   screenStart = vector->begin();
}

template< typename VectorType >
void VectorListingWindow<VectorType>::update(bool isSelected)
{
   auto p = screenStart;
   for (int i = 0; i < nlines-2; i++)
   {
      if (p == vector->end())
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

template< typename VectorType >
void VectorListingWindow<VectorType>::processKey(int ch)
{
   switch (ch)
   {
      case KEY_UP:
	 if (cursorPos != vector->begin())
	 {
	    if (cursorPos == screenStart)
	    {
	       screenStart--;
	    }
	    cursorPos--;
	 }
	 break;
      case KEY_DOWN:
	 if (cursorPos != vector->end()-1)
	 {
	    if (cursorPos == screenStart+nlines-3)
	    {
	       screenStart++;
	    }
	    cursorPos++;
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

template< typename VectorType >
void VectorListingWindow<VectorType>::assignNewVector(std::vector<VectorType>* newVector)
{
   vector->clear();
   delete vector;
   vector = newVector;
   cursorPos = vector->begin();
   screenStart = vector->begin();
   wclear(window);
   update(false);
}

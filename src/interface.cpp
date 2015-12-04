#include "interface.h"
#include "play.h"

int sizeX;
int sizeY;
std::vector<Window*> windows;
VectorListingWindow<std::vector<Album*> >* albumsWindow;
VectorListingWindow<std::vector<Track*> >* tracksWindow;
Window* selectedWindow;


void initInterface()
{
   sizeX = getmaxx(stdscr);
   sizeY = getmaxy(stdscr);

   initscr();
   noecho();
   
   albumsWindow = new VectorListingWindow<std::vector<Album*> >(0, 40, 5, 40, getAlbums());
   tracksWindow = new VectorListingWindow<std::vector<Track*> >(0, 0, 5, 40, getTracks());
   selectedWindow = tracksWindow;
   windows.push_back(tracksWindow);
   windows.push_back(albumsWindow);
}

void updateWindows()
{
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
      case 'T':
	 selectedWindow = tracksWindow;
	 break;
      case 'A':
	 selectedWindow = albumsWindow;
	 break;
      default:
	 selectedWindow->processKey(ch);
   }
}

Window::Window(int startY, int startX, int nlines, int ncols) :
   startY(startY), startX(startX), nlines(nlines), ncols(ncols)
{
   window = newwin(nlines, ncols, startY, startX);
   keypad(window, true);
   nodelay(window, true);
}

Window::~Window()
{
   delwin(window);
}

void Window::displayFrame()
{
   char frame;
   if (this == selectedWindow)
   {
      frame = '$';
   }
   else
   {
      frame = '#';
   }
   
   for (int i = 0; i < ncols; i++)
   {
      wmove(window, 0, i);
      wprintw(window, "%c", frame);
      wmove(window, nlines-1, i);
      wprintw(window, "%c", frame);
   }
   for (int i = 0; i < nlines; i++)
   {
      wmove(window, i, 0);
      wprintw(window, "%c", frame);
      wmove(window, i, ncols-1);
      wprintw(window, "%c", frame);
   }
}

template< typename VectorType >
VectorListingWindow<VectorType>::VectorListingWindow(int startY, int startX, int nlines, int ncols, VectorType* vector) :
   Window(startY, startX, nlines, ncols), vector(vector)
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
      }
      wprintw(window, "%s", (*p)->name.c_str());
      wattroff(window, A_REVERSE);
      p++;
   }
   displayFrame();
   
   wmove(window, 0, 0);
   wrefresh(window);
}

template< typename VectorType >
void VectorListingWindow<VectorType>::processKey(int ch)
{
   switch (ch)
   {
      case ERR:
	 printw("ERR");
	 break;
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
      default:
	 break;
   }
}

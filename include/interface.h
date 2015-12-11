#pragma once

#include <ncurses.h>
#include <deque>

#include "data.h"

#define BORDERS_ALL        0b00001111
#define BORDER_TOP         0b00001000
#define BORDER_LEFT        0b00000100
#define BORDER_RIGHT       0b00000010
#define BORDER_BOTTOM      0b00000001
#define BORDERS_NONE       0b00000000
#define BORDERS_HORIZONTAL 0b00001001
#define BORDERS_VERTICAL   0b00000110

class Window;
template< typename DequeType > class DequeListingWindow;
class TracksListingWindow;
class PlaybackControlWindow;

extern int sizeX;
extern int sizeY;
extern std::deque<Window*> windows;
extern DequeListingWindow<Artist*>* artistsWindow;
extern DequeListingWindow<Album*>* albumsWindow;
extern TracksListingWindow* tracksWindow;
extern PlaybackControlWindow* playbackWindow;
extern Window* selectedWindow;

void initInterface();
void updateWindows();
void fullRefresh();
void readKey();

class Window
{
  public:
   int startX;
   int startY;
   int nlines;
   int ncols;
   WINDOW* window;
   Window* selectedSubWindow;
   std::deque<Window*> subWindows;
   
   Window(int startY, int startX, int nlines, int ncols, char borders);
   ~Window();
   void reshapeWindow(int newY, int newX, int newLines, int newColumns);
   virtual void update(bool isSelected);
   virtual void processKey(int ch) = 0;

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
 
   DequeListingWindow(int startY, int startX, int nlines, int ncols, char borders, std::deque<DequeType>* deque, void (*allocate)(DequeType), void (*select)(DequeType));
   void update(bool isSelected);
   void processKey(int ch);
   void assignNewDeque(std::deque<DequeType>* newDeque);

  protected:
   std::deque<DequeType>* deque;
   typename std::deque<DequeType>::iterator screenStart;

   void (*allocate)(DequeType);
   void (*select)(DequeType);
   void afterReshape();
};

class TracksListingWindow : public DequeListingWindow<Track*>
{
  public:
   TracksListingWindow(int startY, int startX, int nlines, int ncols, char borders, std::deque<Track*>* deque);
};

class PlaybackControlWindow : public Window
{
  public:
   PlaybackControlWindow(int startY, int startX, int nlines, int ncols, char borders);
   void update(bool isSelected);
   void processKey(int ch);

  protected:
   void afterReshape();
   void rewindForward();
   void rewindBackward();
};

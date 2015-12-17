#pragma once

#include <ncurses.h>
#include <deque>
#include <memory>

#include "data.hpp"

using namespace std;

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
extern deque<shared_ptr<Window> > windows;
extern shared_ptr<DequeListingWindow<shared_ptr<Artist> > > artistsWindow;
extern shared_ptr<DequeListingWindow<shared_ptr<Album> > > albumsWindow;
extern shared_ptr<TracksListingWindow> tracksWindow;
extern shared_ptr<PlaybackControlWindow> playbackWindow;
extern shared_ptr<Window> selectedWindow;

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
   WINDOW* window;
   Window* selectedSubWindow;
   deque<Window*> subWindows;
   
   Window(int startY, int startX, int nlines, int ncols, char borders);
   virtual ~Window();
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

// DequeType !MUST! be of deque<something> type
template< typename DequeType > 
class DequeListingWindow : public Window
{
  public:
   typename deque<DequeType>::iterator cursorPos;
 
   DequeListingWindow(int startY, int startX, int nlines, int ncols, char borders, deque<DequeType>* data, void (*allocate)(DequeType), void (*select)(DequeType));
   virtual ~DequeListingWindow() override;
   virtual void update(bool isSelected) override;
   virtual void processKey(int ch) override;
   void assignNewDeque(deque<DequeType>* newDeque);

  protected:
   deque<DequeType>* data;
   typename deque<DequeType>::iterator screenStart;

   void (*allocate)(DequeType);
   void (*select)(DequeType);
   virtual void afterReshape() override;
};

class TracksListingWindow : public DequeListingWindow<shared_ptr<Track> >
{
  public:
   TracksListingWindow(int startY, int startX, int nlines, int ncols, char borders, deque<shared_ptr<Track> >* data);
};

class PlaybackControlWindow : public Window
{
  public:
   PlaybackControlWindow(int startY, int startX, int nlines, int ncols, char borders);
   virtual void update(bool isSelected) override;
   virtual void processKey(int ch) override;

  protected:
   virtual void afterReshape() override;
   void rewindForward();
   void rewindBackward();
};

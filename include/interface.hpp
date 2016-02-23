#pragma once

#include <ncurses.h>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

#include "data.hpp"

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
class ArtistsListingWindow;
class AlbumsListingWindow;
class PlaybackControlWindow;

namespace interface
{
   extern std::recursive_mutex interfaceMutex;
   extern int sizeX;
   extern int sizeY;
   extern std::deque<std::shared_ptr<Window>> windows;
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

class Window
{
public:
   int startX;
   int startY;
   int nlines;
   int ncols;
   std::unique_ptr<WINDOW> window;
   std::deque<std::shared_ptr<Window>> subWindows;
   std::shared_ptr<Window> selectedSubWindow;
      
   Window(int startY, int startX, int nlines, int ncols, char borders);
   
   void reshapeWindow(int newY, int newX, int newLines, int newColumns);

   virtual void update(bool isSelected);
   virtual void processKey(int ch) = 0;
   
   virtual ~Window();

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
 
   DequeListingWindow(int startY, int startX, int nlines, int ncols, char borders, std::deque<DequeType> data);

   void assignNewDeque(std::deque<DequeType> newDeque);
   
   virtual void update(bool isSelected) override;
   virtual void processKey(int ch)      override;

   virtual ~DequeListingWindow()        override;

protected:
   std::deque<DequeType> data;
   typename std::deque<DequeType>::iterator screenStart;

   virtual void select() = 0;
   virtual void press()  = 0;
   
   // void (*allocate)(DequeType);
   // void (*select)(DequeType);
   
   virtual void afterReshape()          override;
};



class TracksListingWindow : public DequeListingWindow<std::shared_ptr<Track>>
{
  public:
   TracksListingWindow(int startY, int startX, int nlines, int ncols, char borders, std::deque<std::shared_ptr<Track>> data);

  protected:
   virtual void select() override;
   virtual void press()  override;
};

class AlbumsListingWindow : public DequeListingWindow<std::shared_ptr<Album>>
{
  public:
   AlbumsListingWindow(int startY, int startX, int nlines, int ncols, char borders, std::deque<std::shared_ptr<Album>> data);

  protected:
   virtual void select() override;
   virtual void press()  override;
};

class ArtistsListingWindow : public DequeListingWindow<std::shared_ptr<Artist>>
{
  public:
   ArtistsListingWindow(int startY, int startX, int nlines, int ncols, char borders, std::deque<std::shared_ptr<Artist>> data);

  protected:
   virtual void select() override;
   virtual void press() override;
};



class PlaybackControlWindow : public Window
{
   friend void playbackWindowThread(PlaybackControlWindow* win);
public:
   PlaybackControlWindow(int startY, int startX, int nlines, int ncols, char borders);

   virtual void update(bool isSelected) override;
   virtual void processKey(int ch)      override;

   virtual ~PlaybackControlWindow()     override;

protected:
   bool stopThread;
   std::unique_ptr<std::thread> winThread;

   void rewindForward();
   void rewindBackward();
   
   virtual void afterReshape()          override;
};

void playbackWindowThread(PlaybackControlWindow* win);

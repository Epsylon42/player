#pragma once

#include <ncurses.h>
#include <vector>

#include "data.h"

#define BORDERS_ALL   0b00001111
#define BORDER_TOP    0b00001000
#define BORDER_LEFT   0b00000100
#define BORDER_RIGHT  0b00000010
#define BORDER_BOTTOM 0b00000001
#define BORDERS_NONE  0b00000000

class Window;
template< typename VectorType > class VectorListingWindow;

extern int sizeX;
extern int sizeY;
extern std::vector<Window*> windows;
extern VectorListingWindow<Artist*>* artistsWindow;
extern VectorListingWindow<Album*>* albumsWindow;
extern VectorListingWindow<Track*>* tracksWindow;
extern Window* selectedWindow;

void initInterface();
void updateWindows();
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
   std::vector<Window*> subWindows;
   
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

// VectorType !MUST! be of std::vector<something> type
template< typename VectorType > 
class VectorListingWindow : public Window
{
  public:
   typename std::vector<VectorType>::iterator cursorPos;
 
   VectorListingWindow(int startY, int startX, int nlines, int ncols, char borders, std::vector<VectorType>* vector, void (*select)(VectorType) = NULL);
   void update(bool isSelected);
   void processKey(int ch);
   void assignNewVector(std::vector<VectorType>* newVector);

  protected:
   std::vector<VectorType>* vector;
   typename std::vector<VectorType>::iterator screenStart;

   void (*select)(VectorType);
   void afterReshape();
};


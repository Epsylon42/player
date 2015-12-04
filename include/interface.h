#pragma once

#include <ncurses.h>
#include <vector>

#include "data.h"

struct Window;
template< typename VectorType > struct VectorListingWindow;

extern int sizeX;
extern int sizeY;
extern std::vector<Window*> windows;
extern VectorListingWindow<std::vector<Album*> >* albumsWindow;
extern VectorListingWindow<std::vector<Track*> >* tracksWindow;
extern Window* selectedWindow;

void initInterface();
void updateWindows();
void readKey();

struct Window
{
   int startX;
   int startY;
   int nlines;
   int ncols;
   WINDOW* window;

   Window(int startY, int startX, int nlines, int ncols);
   ~Window();
   void displayFrame();
   virtual void update(bool isSelected) = 0;
   virtual void processKey(int ch) = 0;
};

// VectorType !MUST! be of std::map<something> type
template< typename VectorType > 
struct VectorListingWindow : Window
{
   VectorType* vector;
   typename VectorType::iterator cursorPos;
   typename VectorType::iterator screenStart;

   VectorListingWindow(int startY, int startX, int nlines, int ncols, VectorType* vector);
   void update(bool isSelected);
   void processKey(int ch);
};

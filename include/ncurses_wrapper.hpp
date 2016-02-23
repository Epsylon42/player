// this header wraps calls of ncurses functions,
// so that they lock interfaceMutex

#pragma once

#include "interface.hpp"
using interface::interfaceMutex;

#include <memory>
using std::unique_ptr;
using winptr = unique_ptr<WINDOW>

#include <ncurses.h>
   ; //NOTE: emacs tries to indent following code without this semicolon

template< typename ... Args >
inline int wprintw_(winptr& window, const char* fmtstr, Args... args);

template< typename ... Args >
inline int mvwprintw_(winptr& window, int y, int x, const char* fmtstr, Args... args);

inline int wmove_    (winptr& window, int y, int x);
inline int wclear_   (winptr& window);
inline int mvwin_    (winptr& window, int y, int x);
inline int wresize_  (winptr& window, int nlines, int ncols);
inline int wrefresh_ (winptr& window);
inline int wclrtoeol_(winptr& window);
inline int wattron_  (winptr& window, int options);
inline int wattroff_ (winptr& window, int options);


template< typename ... Args >
inline int wprintw_(winptr& window, const char* fmtstr, Args... args)
{
   interfaceMutex.lock();
   int ret = wprintw(window.get(), fmtstr, args...);
   interfaceMutex.unlock();
   return ret;
}

template< typename ... Args >
inline int mvwprintw_(winptr& window, int y, int x, const char* fmtstr, Args... args)
{
   interfaceMutex.lock();
   int ret = wmove_(window, y, x);
   if (ret != 0)
   {
      return ret;
   }
   ret = wprintw_(window, fmtstr, args...);
   interfaceMutex.unlock();
   return ret;
}

inline int wclear_(winptr& window)
{
   interfaceMutex.lock();
   int ret = wclear(window.get());
   interfaceMutex.unlock();
   return ret;
}

inline int wmove_(winptr& window, int y, int x)
{
   interfaceMutex.lock();
   int ret = wmove(window.get(), y, x);
   interfaceMutex.unlock();
   return ret;
}

inline int mvwin_(winptr& window, int y, int x)
{
   interfaceMutex.lock();
   int ret = mvwin(window.get(), y, x);
   interfaceMutex.unlock();
   return ret;
}

inline int wresize_(winptr& window, int nlines, int ncols)
{
   interfaceMutex.lock();
   int ret = wresize(window.get(), nlines, ncols);
   interfaceMutex.unlock();
   return ret;
}

inline int wrefresh_(winptr& window)
{
   interfaceMutex.lock();
   int ret = wrefresh(window.get());
   interfaceMutex.unlock();
   return ret;
}

inline int wclrtoeol_(winptr& window)
{
   interfaceMutex.lock();
   int ret = wclrtoeol(window.get());
   interfaceMutex.unlock();
   return ret;
}

inline int wattron_(winptr& window, int options)
{
   interfaceMutex.lock();
   int ret = wattron(window.get(), options);
   interfaceMutex.unlock();
   return ret;
}

inline int wattroff_(winptr& window, int options)
{
   interfaceMutex.lock();
   int ret = wattroff(window.get(), options);
   interfaceMutex.unlock();
   return ret;
}

#define wprintw   wprintw_
#define mvwprintw mvwprintw_
#define wmove     wmove_
#define wclear    wclear_
#define mvwin     mvwin_
#define wresize   wresize_
#define wrefresh  wrefresh_
#define wclrtoeol wclrtoeol_

#undef wattron
#undef wattroff
#define wattron   wattron_
#define wattroff  wattroff_

#include "data.hpp"
#include "play.hpp"
#include "interface.hpp"
#include "playlist.hpp"

#include "log.hpp"

#include <gstreamermm.h>

#include <algorithm>
#include <memory>
#include <string>
#include <iostream>

#include <ncurses.h>
#include <signal.h>

using namespace std;

void segfaultHandler(int)
{
    endwin();
    cerr << "Segmentation fault" << endl;
    log(LT::error, "Segmentation fault");
    exit(1);
}

int main(int argc, char** argv)
{
    signal(SIGSEGV, segfaultHandler);
    setlocale(LC_ALL, "");
    Gst::init(argc, argv);

    data::init();

    for (int i = 1; i < argc; i++)
    {
        data::addTrack(make_shared<data::Track>(argv[i]));
    }

    playback::init();

    interfaceLoop();
    
    playback::end();

    data::end();
}

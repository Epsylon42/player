#include "data.hpp"
#include "play.hpp"
#include "interface.hpp"
#include "playlist.hpp"

#include "log.hpp"

#include <gstreamermm.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>
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

    vector<string> files;
    files.reserve(argc);
    for (int i = 1; i < argc; i++)
    {
        files.push_back(argv[i]);
    }
    thread loadFiles(data::loadFiles, ref(files));

    playback::init();

    interfaceLoop();
    
    playback::end();

    loadFiles.detach();
    data::end();
}

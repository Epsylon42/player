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

void sortLibrary()
{
    using namespace data;

    artists.sort(
            [](shared_ptr<Artist> fst, shared_ptr<Artist> snd)
            {
                return fst->name < snd->name;
            });

    for (auto artist : artists)
    {
        artist->albums.sort(
                [](shared_ptr<Album> fst, shared_ptr<Album> snd)
                {
                    return fst->name < snd->name;
                });

        for (auto album : artist->albums)
        {
            album->tracks.sort(
                    [](shared_ptr<Track> fst, shared_ptr<Track> snd)
                    {
                        return fst->name < snd->name;
                    });
        }
    }
}

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
    sortLibrary();

    playback::init();

    interfaceLoop();
    
    playback::end();

    data::end();
}

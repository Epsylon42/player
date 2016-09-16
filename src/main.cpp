#include "data.hpp"
#include "play.hpp"
#include "interface.hpp"
#include "playlist.hpp"

#include <ao/ao.h>
#include <algorithm>
#include <memory>

#include <ncurses.h>

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

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "");

    av_log_set_level(AV_LOG_QUIET);
    av_register_all();
    ao_initialize();
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

    ao_shutdown();
}

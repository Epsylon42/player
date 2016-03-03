#include "data.hpp"
#include "play.hpp"
#include "interface.hpp"

#include <stdio.h>
#include <ao/ao.h>
#include <unistd.h>
#include <algorithm>
#include <memory>

using namespace std;

void sortLibrary()
{
    sort(data::artistsDeque.begin()+2, data::artistsDeque.end(), [](shared_ptr<Artist> fst, shared_ptr<Artist> snd)
	    {
	    return fst->name < snd->name;
	    });
    for (auto artist : data::artistsDeque)
    {
	sort(artist->albumsDeque.begin()+2, artist->albumsDeque.end(), [](shared_ptr<Album> fst, shared_ptr<Album> snd)
		{
		return fst->name < snd->name;
		});
	for (auto album : artist->albumsDeque)
	{
	    sort(album->tracksDeque.begin(), album->tracksDeque.end(), [](shared_ptr<Track> fst, shared_ptr<Track> snd)
		    {
		    return fst->name < snd->name;
		    });
	}
    }
}

int main(int argc, char* argv[])
{
    av_log_set_level(AV_LOG_QUIET);
    av_register_all();
    ao_initialize();
    initData();

    addTrack(make_shared<Track>(argv[1]));
    for (int i = 2; i < argc; i++)
    {
	addTrack(make_shared<Track>(argv[i]));
    }
    sortLibrary();

    initPlay();

    interfaceLoop();

    endPlay();

    data::artistsDeque.clear();
    data::artistsMap.clear();

    ao_shutdown();
}

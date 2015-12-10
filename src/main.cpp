#include "data.h"
#include "play.h"
#include "interface.h"

#include <stdio.h>
#include <ao/ao.h>
#include <unistd.h>
#include <algorithm>

void sortLibrary()
{
   std::sort(artistsDeque.begin()+2, artistsDeque.end(), [](Artist* fst, Artist* snd)
	     {
		return fst->name < snd->name;
	     });
   for (auto artist : artistsDeque)
   {
      std::sort(artist->albumsDeque.begin()+2, artist->albumsDeque.end(), [](Album* fst, Album* snd)
		{
		   return fst->name < snd->name;
		});
      for (auto album : artist->albumsDeque)
      {
	 std::sort(album->tracksDeque.begin(), album->tracksDeque.end(), [](Track* fst, Track* snd)
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

   Track* fstTrack = new Track(argv[1]);
   addTrack(fstTrack);
   for (int i = 2; i < argc; i++)
   {
      addTrack(new Track(argv[i]));
   }
   sortLibrary();

   // // TEST PRINT ARTISTS
   // for (auto artist : artistsDeque)
   // {
   //    if (artist != NULL)
   //    {
   // 	 artist->testPrint();
   //    }
   // }
   
   // // play(fstTrack);
   initInterface();
   while (true)
   {
      updateWindows();
   }
   // auto tracks = getTracks();
   // for (auto p = tracks->begin(); p != tracks->end(); p++)
   // {
   //    printf("%p\n", (*p));
   //    if ((*p) != NULL)
   //    {
   // 	 (*p)->testPrint();
   //    }
   // }
   
   ao_shutdown();
}

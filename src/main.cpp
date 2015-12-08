#include "data.h"
#include "play.h"
#include "interface.h"

#include <stdio.h>
#include <ao/ao.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
   av_register_all();
   ao_initialize();
   initData();

   Track* fstTrack = new Track(argv[1]);
   addTrack(fstTrack);
   for (int i = 2; i < argc; i++)
   {
      addTrack(new Track(argv[i]));
   }

   // TEST PRINT ARTISTS
   for (auto artist : artistsDeque)
   {
      if (artist != NULL)
      {
	 artist->testPrint();
      }
   }
   
   // play(fstTrack);
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

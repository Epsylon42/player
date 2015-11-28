#include "data.h"
#include "play.h"

#include <stdio.h>
#include <ao/ao.h>

void initLibs()
{
   av_register_all();
   ao_initialize();
}

void stop()
{
   ao_shutdown();
}

int main(int argc, char* argv[])
{
   initLibs();
   initData();

   // for (int i = 1; i < argc; i++)
   // {
   //    addTrack(new Track(argv[i]));
   // }
   // for (auto artistPair : artists)
   // {
   //    Artist* artist = pair.second;
   //    if (artist != NULL)
   //    {
   // 	 artist->testPrint();
   //    }
   // }
   // printf("\n\n\n");
   // for (auto artistPair : artists)
   // {
   //    for (auto album : *pair.second->getAlbums(false))
   //    {
   // 	 album->testPrint();
   //    }
   // }
   // printf("\n\n\n");
   
   // for (auto album : *getUnknownAlbums())
   // {
   //    for (auto trackPair : album->tracks)
   //    {
   // 	 trackPair.second->testPrint();
   //    }
   // }


   std::string fileName = argv[1];
   Track* track = new Track(fileName);
   play(track);
   
   stop();
}

# Player

## story of my life

This is a pet project of mine from 2015-2016 when I didn't have a music player with all the features I wanted,
like queuing a file after the current playlist for example. In fact, I still don't, but now I just use mplayer. It's pretty ok.

This is also my first big(-ish) project in modern(-ish) c++  
It was my first introduction to smart pointers, multithreading and lots of other stuffs.
If I remember correctly, I hadn't even known about vectors before this.

It was also my first introduction to [suffering](https://xkcd.com/979/).
Basically, audio processing is hard, C libraries are hard, and ffmpeg is hard in particular.
Apparently my StackOverflow questions have been deleted, but I still have that Tumbleweed badge.

It has been a long time since I looked at this code, but at first glance it seems to be not that bad.
The exceptions are really weird shit like using namespaces with global variables instead of singletons.
Now I'm better than this and so wouldn't use anything remotely resembling a singleton.

Anyway, however you ended up here, enjoy the emotional rollercoaster and character development in the commit history. Or don't.
Probably don't use the player though. Cool as it is, it's still kinda bad and buggy.

## building

You need `cmake`, `ncursesw` and `gstreamer` (`libncursesw-5` and `libgstreamermm-1.0` on Ubuntu), and `pkg-config` for cmake to find them.

Build like a regular cmake project

Note that on my computer it doesn't compile with `g++-9`, so you may need to use an older compiler. 
In my defence, the errors are somewhere in gstreamer headers.

## usage

    player <ALL-THE-MUSIC-FILES...>

If there's a lot of files you may need to wait a bit. There's a branch with asynchronous file loading but it's broken somewhat

### keybindings

#### anywhere
* `Q` - quit
* `hjkl` - navigate between windows
* `<` and `>` - change tracks
* `R` - refresh UI
* space - toggle playback
* `t` - toggle shuffle (may not work exactly how you expect)
* `e` and `E` - stop playback (there's a difference I think, but I don't remember what it is)

#### in the artists, albums and tracks windows
* `s` - clear the queue and play
* `S` - play at the very end
* `d` - play after the current track
* `D` - play after the current playlist (player doesn't have frontend for playlists but artists and albums do count as those)
* `f` - play immediately, but after that return to the current track

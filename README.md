# ChadQuake
So I asked myself, how would [Chad](http://knowyourmeme.com/memes/chad-thundercock) play [Quake](https://www.gog.com/game/quake_the_offering)?

ChadQuake is a fork of [Mark V Quake](http://quakeone.com/markv/) with the following changes:
### Software rendering only
Because that's how Quake was meant to be played. No texture filtering, no texture perspective correction, awesome [fluid effects](https://fdossena.com/?p=quakeFluids/i.md), better lighting. For a better comparison between Software Quake and GLQuake, see [this article](https://www.quaddicted.com/engines/software_vs_glquake).  
Code for other versions is still there, but it's untested and the build config has been removed.

### No animation interpolation
Choppy animations and movements are part of Quake, and they look better than interpolated ones that look like what a 12 year old would make in an evening playing around with Unity

### Quake defaults, while keeping support for mods
All the customization is hidden so that it can only be accessed by mods or the console. This keeps the original look and feel, as well as support for all mods that work with Mark V.

### MP3 only
It's 2018, Chad doesn't have a CD drive, why even support it? Music is only read from MP3 files instead.

### Windows only
Because it's the best and chaddest operating system. Unix supremacist can go eat a shambler's ass.  
Code for other versions is still there, but it's untested and the build config has been removed.

### No level selection menu
Chad plays through the whole game on nightmare, he doesn't cheat with the level selector

### The original menu
Mouse support for menus in a 1996 game? Heresy!

### Max resolution is 1280x1024
Do you think a 1996 game was meant to be played in 4K? Or even 1080p? Heresy! You play in 640x480 and thank your daddy for buying you that sweet Pentium MMX, otherwise you'd be stuck at 320x200 like everyone else. For mods support however, it's better to have a limit of 1280x1024

### Minor bugfixes
Because Baker never died while testing his port, reloading while dead was bugged in 1036 and is fixed in ChadQuake.

I did not see any other bug while playing through the entire game, expansion packs, and several mods, so it's unlikely that ChadQuake will ever be updated.

## How to use ChadQuake
It works the same as Mark V:
* Put your game pak files inside the ID1 folder
* Put your music files in ID1\Music named Track##.mp3
* Run the game
 
All command line options are the same as the Mark V.

## Building
ChadQuake, like Mark V can be build on Visual Studio 2008 SP1. Load the project and press Build. All dependencies are included.

## Credits
Baker: for making Mark V, the best and most badass Quake source port out there.

## License
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

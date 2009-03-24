Q3A Game Source. Copyright (C) 1999-2000 Id Software Incorporated

NOTE: The source MUST BE INSTALLED into the \quake3\ directory. So if you want to install it on C: it needs to be installed to c:\quake3. 

The Game Source is broken out into 3 areas.

game - governs the game, runs on the server side.
cgame - governs the client side of the game, runs on the client side.
ui - handles the ui on the client side.

Making a quick mod is very straightforward. This document assumes Microsoft Visual C++ v6.xx. It covers making a slight mod to the game source, recompiling for debugging and rebuilding the vm's for distribution.

Slow Rockets - TestMod
----------------------
1. Open up the Q3AGame.dsw in Microsoft Visual C++.
2. Set the "game" project as the active project.
3. Open the "g_local.h" file and change the GAMEVERSION define from "baseq3" to "TestMod"
4. Save "g_local.h"
5. Open the "g_missile.c" file.
6. Go to line 389 and change the 900 to 300. The old line reads:
	
	VectorScale( dir, 900, bolt->s.pos.trDelta );

The new line should read

	VectorScale( dir, 300, bolt->s.pos.trDelta );

7. Save "g_missile.c"
8. Perform a Build All command and it should build a DLL for the game. 

At this point you have two options. You can run the debugger, choosing 'Quake3.exe' as the executable host which will load your DLL for debugging or you can build the vm's for distribution. When you release mods, you will want to build new vm's.

Building the vm's requires two things.

1. The files contained in the bin_nt path must be available for execution ( lcc.exe and q3asm.exe )
2. There must be environment variables set for proper lib and include paths. Microsoft Visual C++ installs a batch file that does this for you called "VCVARS32.bat"

To build the sample vm for the slow rocket test, do the following:

1. Open a DOS window.
2. Make sure lcc.exe and q3asm.exe are available on the path.
3. Run VCVARS32.bat
4. Go to your mods game directory and run the 'game.bat' file.

This ultimately produces a 'qagame.qvm' in the \baseq3\vm\ path. 

5. Make a "TestMod" path under your Quake3 directory. This will be a sibling to 'baseq3'
6. Move 'qagame.qvm' to "\YourQuake3Path\TestMod\vm\"
7. Run Quake3 with the following command line "Quake3 +set fs_game TestMod"
8. "TestMod" should be the referenced game and you should be able to catch up with and outrun your rockets.


Each of the areas contain a batch file "game.bat", "cgame.bat", and "ui.bat" which will build the appropriate vm files.

	
	

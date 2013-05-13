Tremulous Zombie Mod
==================

Yes, the infamous Zombie mod for the game Tremulous!

As a warning, the code in this mod is at times hackish and brutally ugly, but it works.

You can learn more about the zombie mod at the xserverx community: http://xserverx.com/forums

Extra Dependencies
-----------------------
The Z QVM depends on MySQL/MySQL development libraries. Ensure these are installed before attempting to build.

Building
-----------------------
You can build the Z QVM the same way you build normally.

Make sure that the `MAKE_GAME_QVM` flag is set in the Makefile before building.

As normal there are shell scripts for building on Windows and Mac OSX. Windows requires MingW be used.

Disable MySQL before building if you do not wish to use the MySQL offerings of the Z mod.

Configuration
-----------------------
The Z QVM relies on addtional map layouts which can be found in the "layouts" directory.

If you are using the MySQL offerings, you will also need to specify your credentials in the server's config file.

Running
-----------------------
The Z QVM requires the xserverx customized tremulous dedicated server to run. You can find the code for this server here: https://github.com/AlienHoboken/Tremulous-xserverx-tremded

Furthermore, when running the Z mod will attempt to communicate with a MySQL database. Make sure you disable MySQL if you do cannot support this behavior.

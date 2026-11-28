WATCH-MAN audio files
=====================
Drop a file with the exact name below into the matching folder to replace that sound.
Accepted: .wav .mp3 .ogg .flac (any sample rate; stereo is mixed to mono because the game pans sounds itself).
A missing file = the game uses its built-in generated sound, so you can replace sounds one at a time.
Files are read when the game starts. Restart it after changing a file. Loops (marked LOOP) should be seamless.
The game looks in  assets/audio  in the folder you run it from, then next to the executable (and up to two folders above).
Console lines "audio: loaded ..." / "audio: FAILED ..." show what was picked up.

ambience/   (LOOP) rainWindow, rainOpen, wind, lampBuzz, liftHum, purr
music/      (LOOP) musicHall (building), musicRoof (roof)
rooms/      (LOOP, heard behind apartment doors) tvMatch (102), radioChess (108), opera (201), keyboard (202), fan (206), crtHum (207), piano (208)
events/     one-shots: knock, latch, liftDing, doorSlide, pageTurn, cluePing, pipeKnock, slam, lens
            random variants (replace any or all): thunder0-2, creakOpen0-2, creakClose0-2, meow0-2
footsteps/  random variants: footTile0-3, footStairs0-3, footWet0-3

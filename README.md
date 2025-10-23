# dmxplay

A simple terminal MIDI/MUS/XMI player based on Nuke.YKT's [WinMM driver](https://github.com/nukeykt/WinOPL3Driver) for DOS and for Linux/Windows/macOS with SDL2.

## Usage:

```
dmxplay [-bank <bank>] [-setup "<string>"] [-loop] <filename>
```

- `<filename>` - Path to music file to play. Required.
- `-bank <bank file name>` - Path to custom OP2 bank file.
- `-loop` - Enable looping of the opened music file.
- `-setup "<string>"` - Set a quoted space-separated setup string for synth in same as DMXOPTION environment variable.
- `-opl3` - enable OPL3 mode (by default the OPL2 mode).
- `-doom1` - Enable the Doom1 v1.666 mode (by default the v1.9 mode).
- `-doom2` - Enable the Doom2 v1.666 mode (by default the v1.9 mode).
- `-gain` - \[Non-DOS ONLY\] Set the gaining factor (default 2.0).
- `-wave <path.wav>` - \[Non-DOS ONLY\] Record output into WAV file of spcified path.
- `-towave` - \[Non-DOS ONLY\] Record output into WAV file in a place. The name for the WAV file will be taken from the music file directly, and result WAV file will be saved at the same directory.
- `-emu <name>` - \[Non-DOS ONLY\] Select playback chip emulator: `nuked`, `dosbox`, `java`, `opal`, `ymfm-opl2`, `ymfm-opl3`, `mame-opl2`, `lle-opl2`, `lle-opl3`
- `-addr <0xVAL>` - \[DOS ONLY\] Set the hardware OPL2/OPL3 address. Default is `0x388`.

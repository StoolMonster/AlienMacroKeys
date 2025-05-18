# AlienMacroKeys

I found this repo[https://github.com/mscreations/Alien-Macros], which is no longer updated for more than a year and only valid for `Alienware m17 R4`.

## About this repo

1. Still in early development.
2. Currently only valid for my Dell G16 7630.
3. Map the macro keys (A-E) to F13-F17.
4. No AWCC needed.
5. Use this along side the AHK to map the keys (F13-F17) to your desired function.

## Example AHK script

```
#Requires AutoHotkey v2.0

; Map F13 to
F13::Send("{Media_Prev}")

; Map F14 to
F14::Send "{Media_Play_Pause}"

; Map F15 to
F15::Send("{Media_Next}")

; Map F16 to a
F16::a

; Map F17 to b
F17::b 
```

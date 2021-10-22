---
title: Badge Software
date: Last Modified 
permalink: /software.html
eleventyNavigation:
  key: software 
  order: 2
  title: Badge Software
---
### Games

| <span style="color:grey;">name</span>     | <span style="color:grey;">description</span> |
| ----------- | ---------- |
| Lunar Rescue | a lunar lander type game in which you must rescue your fellow astronauts and transport them to the lunar base |
| Badge Monsters | you've heard of Pokemon Go? This is not at all like that. Not at all. |
| Space Tripper | Something akin the 1979 Super Startrek BASIC game, but running on the badge. |
| Smashout | Similar to Breakout on the ATARI VCS |
| Hacking Sim | Route the Inter Tubes to make the data flow |
| Spinning Cube | Does what it says on the tin |
| Game of Life | Conway's game of life |

### Other
| <span style="color:grey;">name</span>     | <span style="color:grey;">description</span> |
| ----------- | ---------- |
| Schedule    | The Schedule badge app shows you the schedule for the 2021 RVAsec conference. |
| About Badge | The About Badge badge app displays a link to this web page. |
| Blinkenlights | makes the badge LED lights blink |
| Conductor | allows you to make beep boop sounds |
| Sensors | what does this do? |
| Settings |
| Backlight |
| Led ||
| Buzzer ||
| Rotate ||
| User Name | Allows you to enter your name |
| Screensaver | various screensavers |




## Making your own Badge Apps
If you know C, and you've got a linux machine, making your own badge apps is not that difficult, although you'll need a special cable to be able to flash the badge firmware and actually run it on the badge. However, you can try your hand at making a badge app even without the special cable and without being able to flash the firmware using our handy dandy linux badge emulator. Maybe you can make something cool that will show up on next year's badge.

See the following documentation:
[linux/README.md](https://github.com/HackRVA/badge2020/blob/master/linux/README.md)
[Badge App HOWTO](https://github.com/HackRVA/badge2020/blob/master/BADGE-APP-HOWTO.md)


## The C interpreter

Believe it or not, the badge has a built-in C interpreter, and you can send it small C programs via it's USB serial interface to control the LEDs or the display.

TODO elaborate on this.

There are some examples at the bottom of this page: https://github.com/HackRVA/badge2020



# RedditWii
Yes, you read the title right. I made Reddit for the Wii!
It's not as good as you would first make it out to be though. Once you look at one image and attempt to look at another the system will crash.

## Installation
Download the zip in the releases tab. You will be able to run the .dol by executing it with Dolphin or by putting it on an SD Card which will go into your hacked Wii.

## Controls
Use the D-Pad to navigate in the front page, A to view a post. Once you have viewed a post go back using B.

WARNING: After exiting from a post to the front page, attempting to view another post will crash your entire system.

Pressing the Home Button will exit the app.

## Building from Source
First you need [devkitppc](https://devkitpro.org/wiki/Getting_Started). Then you need to install the GRRLIB API which you can get [here.](https://github.com/GRRLIB/GRRLIB)

After installing this, just execute "make" in the root directory and a .elf and .dol file should be created!

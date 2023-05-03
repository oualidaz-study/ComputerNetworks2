# ComputerNetworks2
For this project, we use SDL2 (https://www.libsdl.org/) to play audio. SDL2 is famously cross-
platform, and should work on just about any platform where you can get it to compile.
'SDL2 can be installed simply using:'
sudo apt install libsdl2-dev
Or any similar command using a package manager of your choice.
The provided Makefile should work on most Linux systems. This includes Windows Subsystem for
Linux, and WSLg (https://github.com/microsoft/wslg), which is included in the relatively up-to-
date Windows 11 builds.
Since SDL2 is not available on the LIACS systems in any meaningful way, a build has been provided
at /vol/share/groups/liacs/scratch/cn2022/. The provided Makefile includes provisions to build
using this, by invoking make as:
make SDL2_CONFIG=/vol/share/groups/liacs/scratch/cn2022/bin/sdl2-config

# IDA-SourceMod
 
Implements some of my SourceMod scripts in plugin-form.

### Purpose ###

I wanted to centralize all of my scripts into the IDA framework. It was actually a hassle trying to manage all of mine. I have a few I've released along with about a dozen that I haven't so it grew to be a bit of a headache.

I thought it'd better to just have them all completely implemented into a plugin that way I'm not scouring over folders and files trying to find the script. ~~Everything~~ A good few scripts are organized into a menu and yes, of course, bound to keyboard shortcuts.

I opted to make this in C++ rather than Python since I thought it'd be fun. It was not.

Regardless, this makes "those tedious IDA things" just *that* much easier to deal with when it comes to SourceMod stuff.

### Features ###

5 translated scripts ready to go.

- MakeSig
- MakeSig from here
- Get offset from function
- Validate signature
- Jump to signature

There's also a halfass decent API. Script addition is fairly streamlined, so if you want to add something yourself you shouldn't have that hard of a time depending on complexity.

The hardest part would probably be building 🙃.

### Planned ###

Translate the rest if not a lot of the scripts I've written in Python. Some of those are gonna *suck*.

Support for the absolute maniacs that run x64 SRCDS.

Complex menus (submenus for scripts/subplugins).

Keyboard shortcut configuration (everything is hardcoded at the moment).

### Installation ###

Current builds are against IDA 7.5's SDK. If you want a version lower you're going to have to build it yourself. (Good luck with that).

### Building Yourself ###

I don't own a Linux version of IDA so I didn't bother trying to set up a build for that. The main thing you need to know is that this uses C++20 features so don't forget to add the switches and update MSVC (16.10+ I think).

I also didn't bother putting and x64 build because half the scripts don't even work on x64. Why would you want x64? Who runs 64 bit SRCDS? What kind of maniac would do something like that?

#### Windows ####

Make sure you are running MSVC 16.10+.

Open up src/idasourcemod.sln.

I'm assuming MSVC isn't absolutely stupid and will save my options. Otherwise, follow the instructions found in `install_visual.txt` in your IDA SDK directory.

Click Project > idasourcemod Properties

Make sure your configuration is ida32 Release (or ida64 Release 🙄).

Click C/C++ > General

On the right, you'll see Additional Include Directories. Add your IDA SDK path to that (it'll probably have mine in there already)

Apply

Click Linker > Input

On the right, you'll see Additional Dependencies. Add `{Your IDA SDK DIR}\lib\x64_win_vc_32\ida.lib`; replace mine obviously. Or if you're somehow running 32bit in 2021 put `x86_win_vc_32`. Do the obvious if you're building for x64.

Apply

You should be good to go. Otherwise, follow the instructions found in `install_visual.txt` in your IDA SDK directory. And if that still doesn't work, try everything again until it does. Make sure you have the C++20 switches!
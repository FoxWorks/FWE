FoxWorks Editor
--------------------------------------------------------------------------------
FWE is a specialized CAD system for creating draft designs of aerospace vessels.
It's primary use is to define theoretical shapes of vessels and define general layout
of their internal systems.

FWE is based around [EVDS](http://evds.wireos.com/) and IVSS simulation libraries,
and will have built-in simulation support (scripted/defined with Lua). This CAD
system is also used for entering data for
[FoxWorks Aerospace Simulator](http://foxworks.wireos.com/).

The primary modelling approach in FWE is creating 3D shapes as defined by a set
of arbitrary cross-sections (circles, rectangles, polygons, bezier curves).
It will be possible to export the entire model in a variety of different 3D formats.

The editor uses the EVDS file format for storing the vehicles, fully compatible with
any application using EVDS for aerospace simulation.


Features
--------------------------------------------------------------------------------
 - Determines physical parameters of the vessel in realtime during editing
 

Compiling
--------------------------------------------------------------------------------
All the dependencies should be downloaded automatically with the Git repository,
otherwise use the following command to include submodules:
```
git clone --recursive https://github.com/FoxWorks/FWE.git
```

FoxWorks Editor requires Qt 4.8.5 to be compiled.

The build files and all required Qt files must be generated from scratch using
[Premake5 or Premake4.4](http://industriousone.com/premake):
```
cd support
premake4 vs2008
premake4 vs2010
premake4 gmake
premake4 evdsdoc
```

Use the generated `sln` files under Windows. They will include all required
dependencies (which are present as submodules in repository).

Use makefiles under Linux or other platforms:
```
cd support/gmake
make
```
 

Screenshots
--------------------------------------------------------------------------------
![FoxWorks Editor Screenshot](http://i.imgur.com/3txyy8T.png)

![FoxWorks Editor Screenshot](http://i.imgur.com/qtYESXH.jpg)

![FoxWorks Editor Screenshot](http://i.imgur.com/zK3Izm7.jpg)

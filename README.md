# FCPP XC Processes
Project for developing and demonstrating eXchange Calculus processes in FCPP.

All commands below are assumed to be issued from the cloned git repository folder.
For any issues with reproducing the experiments, please contact [Giorgio Audrito](mailto:giorgio.audrito@unito.it).

## References

- FCPP main website: [https://fcpp.github.io](https://fcpp.github.io).
- FCPP documentation: [http://fcpp-doc.surge.sh](http://fcpp-doc.surge.sh).
- FCPP presentation paper: [http://giorgio.audrito.info/static/fcpp.pdf](http://giorgio.audrito.info/static/fcpp.pdf).
- FCPP sources: [https://github.com/fcpp/fcpp](https://github.com/fcpp/fcpp).


## Setup

The next sections contain the setup instructions for the various supported OSs. Jump to the section dedicated to your system of choice and ignore the others.

### Windows

Pre-requisites:
- [MSYS2](https://www.msys2.org)
- [Asymptote](http://asymptote.sourceforge.io) (for building the plots)
- [Doxygen](http://www.doxygen.nl) (for building the documentation)

At this point, run "MSYS2 MinGW x64" from the start menu; a terminal will appear. Run the following commands:
```
pacman -Syu
```
After updating packages, the terminal will close. Open it again, and then type:
```
pacman -Sy --noconfirm --needed base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-make git
```
The build system should now be available from the "MSYS2 MinGW x64" terminal.

### Linux

Pre-requisites:
- Xorg-dev package (X11)
- G++ 9 (or higher)
- CMake 3.18 (or higher)
- Asymptote (for building the plots)
- Doxygen (for building the documentation)

To install these packages in Ubuntu, type the following command:
```
sudo apt-get install xorg-dev g++ cmake asymptote doxygen
```
In Fedora, the `xorg-dev` package is not available. Instead, install the packages:
```
libX11-devel libXinerama-devel.x86_6 libXcursor-devel.x86_64 libXi-devel.x86_64 libXrandr-devel.x86_64 mesa-libGL-devel.x86_64
```

### MacOS

Pre-requisites:
- Xcode Command Line Tools
- CMake 3.18 (or higher)
- Asymptote (for building the plots)
- Doxygen (for building the documentation)

To install them, assuming you have the [brew](https://brew.sh) package manager, type the following commands:
```
xcode-select --install
brew install cmake asymptote doxygen
```
### Virtual Machines

If you use a VM with a graphical interface, refer to the section for the operating system installed on it.

**Warning:** the graphical simulations are based on OpenGL, and common Virtual Machine software (e.g., VirtualBox) has faulty support for OpenGL. If you rely on a virtual machine for graphical simulations, it might work provided that you select hardware virtualization (as opposed to software virtualization). However, it is recommended to use the native OS whenever possible.

### Graphical User Interface

Executing a graphical simulation will open a window displaying the simulation scenario, initially still: you can start running the simulation by pressing `P` (current simulated time is displayed in the bottom-left corner). While the simulation is running, network statistics may be periodically printed in the console, and be possibly aggregated in form of an Asymptote plot at simulation end. You can interact with the simulation through the following keys:

- `Esc` to end the simulation
- `P` to stop/resume
- `O`/`I` to speed-up/slow-down simulated time
- `L` to show/hide connection links between nodes
- `G` to show/hide the grid on the reference plane and node pins
- `M` enables/disables the marker for selecting nodes
- `left-click` on a selected node to open a window with node details
- `C` resets the camera to the starting position
- `Q`,`W`,`E`,`A`,`S`,`D` to move the simulation area along orthogonal axes
- `right-click`+`mouse drag` to rotate the camera
- `mouse scroll` for zooming in and out
- `left-shift` added to the camera commands above for precision control
- any other key will show/hide a legenda displaying this list

Hovering on a node will also display its UID in the top-left corner.

## Simulations

### Message Delivery (Graphic) 

```./make.sh gui run -DNOTREE -O graphic```

Runs a single test simulation with GUI for the spherical scenario, single message sent.

```./make.sh gui run -DNOTREE -DMULTI_TEST -O graphic```

Runs a single test simulation with GUI for the spherical scenario, multiple messages sent.

```./make.sh gui run -DNOSPHERE -O graphic```

Runs a single test simulation with GUI for the tree scenario, single message sent (comparing spawnXC and spawnFC).

For all three commands above, produced graphics can be found in `plot/graphic.pdf`.

#### Parameters (cf. plots)

- **dens** density of the network as avg number of neighbours
- **hops** network diameter
- **speed** maximum speed of devices as a percentage of the communication speed
- **tvar** variance of the round durations, as a percentage of the avg

#### Metrics (cf. plots)

- `dcount` (delivery count): number of messages that arrived to destination 
- `aproc` (average processes): average number of process instances (i.e., for a single process, the average number of devices running it)
- `adel` (average delay)

See also the namespace `tag` in file `lib/generals.hpp`.

### Message Delivery (Batch) 

```./make.sh run -DNOTREE -O batch```

Runs 100 simulations for the spherical scenario, single message sent.

```./make.sh run -DNOTREE -DMULTI_TEST -O batch```

Runs 100 simulations for the spherical scenario, multiple messages sent.

```./make.sh run -DNOSPHERE -O batch```

Runs 100 simulations for the tree scenario, single message sent (comparing spawnXC and spawnFC).

For all three commands above, produced graphics can be found in `plot/batch.pdf`.

For *parameters* and *metrics* see the previous section.

### Replicated PastCTL (Graphic) 

```./make.sh gui run -O replicated_pastctl```

Runs a single test simulation with GUI for the replicated PastCTL scenario.

Produced graphics can be found in `plot/replicated_pastctl.pdf`.

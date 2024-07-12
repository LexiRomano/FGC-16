# FGC-16

The FGC-16 is a 16-bit computer architecture based on absolutely nothing. The purpose of this project was to try to design an architecture with little prior knowledge and
learn by failing... dramatically. This is only one part of a much larger project named F-Suite where I will try to build everything from the ground up with
little to no help or training.

This is an emulator for the FGC-16 architecture. I doubt I will ever make this in real hardware as that is a massive leap from what's going on here. I'm leaning *heavily*
on C and windows to do most of the hard work for me.

## Usage

There are 2 versions of the `FGC-16` executable and a helper program called `binWriter`. The regular `FGC-16` executable is the default program that will execute code at full
speed with no regards to your feelings as it's bound to sniff out your bugs like a bloodhound and promptly crash with little explination. The `DEBUG` version of the executable
is, as you could guess, a debugging version of the executable. It will halt at Every. Single. Command. and print out the contents of the registers. This is really usefull
for getting a feel for the architecture with small test programs but is in no way usefull for anything larger as it will take ages to get there and there aren't any
breakpoints. The `binWriter` is a helper program that can take hex ascii numbers and write them directly into a binary file which is then used by the main program to run.

### The Basics

On boot, the computer will start executing from address `0x0000` which is the start of `rom`. The files where the program will read from are stored in the data folder. To start
making programs, take a look at the datasheet linked at the bottom of this readme. The first page contains all supported instructions, a dictionary explaining my horrible
terminology, a memory layout, and IO ports. The second page contains information about `Fssembly` which I won't talk about here. For that documentation, check out the `Fssembly`
repo. After you have a program written, launch the `binWriter` program to add it to the data files. For now, select manual mode (`M`). You will then be prompted for what
file you would like to edit. Select rom (`r`). You will then enter the "editor" were you can type or paste in the hexadecimal code. Numbers out of range will simply be ignored
while etering a non-hex number will exit the editor. Once you enter a number, you cannot delete it without exiting and restarting the process, so I recommend pasting in from
a text document. After you're done, enter a non-hex character to exit the editor. You'll be prompted for if you want to exit the program or start again. Type `Y`. Your program
is now ready to run! Simply launch either `FGC-16` or `FGC-16 DEBUG` and watch your program catch fire!

### Advanced programs

When writing more advanced programs, you can use disks which are writeable and persist between sessions. You can also pre-populate these disks with programs using the `binWriter`.
Once you have your program written as before, launch the `binWriter`, select manual mode, and select which disk you would like to write to. You will then need to choose what sector
of the disk you will write to. You can also format the disk if the file somehow breaks or you just want to start fresh. As before, type or paste in your program and exit when
you're done. Your program will then be loaded into that sector of the disk. Be sure to write a `BIOS` program to load your disk sectors into `ram` as this is not done automatically.

Once you're a pro and start writing accross several disks, it can get a bit tiring to have to manually paste in each and every sector. To solve this issue, there is a batch mode
in the binWriter. To use it, select `B` in the main menu. Type in the path of the batch file and it will automatically do everything for you! Below is a sample batch file so you
can see the syntax:

        rx;
        0x;
        r fbn/rom.fbn;
        00 fbn/disks/0/s0.fbn;
        01 fbn/disks/0/s1-0.fbn fbn/disks/0/s1-1.fbn fbn/disks/0/s0-2.fbn;

What this will do is format `rom` and `disk0`. It will then take the contents of the file `fbn/rom.fbn` and write it to `rom`. It does the same for `disk0:sector0`,
and takes 3 files and puts them all sequentially into `disk0:sector1`.

## Platforms

Currently, this program only works on windows. Maybe I'll make it work on Linux when I get a pc that uses Linux, but until then, suffer.

## Datasheet

View datasheet [here](https://docs.google.com/spreadsheets/d/1bagL_yX_ullKfEMETFIMV0RIFsRq73ULrNGZyCADKzc/edit?usp=sharing)

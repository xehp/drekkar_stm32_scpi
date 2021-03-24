An SCPI adapter. This code is used for reading voltage and current from an SCPI compatible voltage meter. The hardware is assumed to be an STM32 custom made board.


This is the SW for "...CableAgeing2/kicad/controlboard_nucleo_4_inv/output.pdf"
when that board is used to drive BOTH a buck converter AND a H bridge (the inverter). 
See .../hardware/hdriver500/hdriver500.pdf

Or
This is the SW for "...CableAgeing2/kicad/controlboard_nucleo_4_buck_full/output.pdf"
when that board is used to drive the inverter H bridge. 


The H bridge may be located in another "BOX" with the board running this program being
located in the buck converter box. 


A compiler will be needed. Its possible that all you need to do is to install gcc-arm-none-eabi like this:
  sudo apt install gcc-arm-none-eabi



However prior to this program I experimented with MBED and Zephyr and during that process I installed many more tools.
So if something is still missing then one of these might be needed also:
  sudo apt install git cmake cutecom eclipse-cdt eclipse-jdt eclipse-egit screen gperf ninja-build 
  sudo apt install vscode python2
    Install python2. NOT PYTHON3 
  Perhaps install also mercurial:
    sudo add-apt-repository ppa:team-gcc-arm-embedded/ppa
    sudo apt-get update
    sudo apt install gcc-arm-embedded python-pip python-setuptools mercurial
  This was installed earlier when using MBED but its installed so it might be needed for this also (who knows).
    Install pip for python2. Sometimes called pip2. (On arch python2-pip)
    Install pip2 install mdev-cli
    Symlink python2 and pip2 to pip and python in ~/.bin
  These may have another name on Ubuntu/Debian, anyway on Arch it was like this:
    sudo pacman -S arm-none-eabi-newlib arm-none-eabi-gcc arm-none-eabi-binutils 
  This contradicts the warning to avoid python3 above. Try and see what works.
    sudo apt install python3-pip
    pip3 install --user pyelftools
  Install DTC, If dtc is not found its probably called device-tree-compiler instead.
    sudo pacman -S dtc
    sudo apt install device-tree-compiler


st-link used to copy code to target via CLI (in long run much faster than mouse clicking).
  sudo apt install libusb-1.0 cmake
  cd ~/git/
  git clone https://github.com/texane/stlink.git
  cd stlink
  Read doc/compiling.md and compile it.


This program tries to avoid RTOS and HAL but use some header files from it.
The header files are OK to distribute like this as far as I can understand.

To Compile do cd to same folder as the Makefile then:
make

Download code to device:
sudo /home/henrik/git/stlink/build/st-flash --format ihex write binary.hex
sudo /home/henrik/git/stlink/build/Release/st-flash --format ihex write binary.hex

For more see comments in "src/main.c".



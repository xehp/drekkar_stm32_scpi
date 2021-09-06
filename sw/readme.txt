
This is the SW for "...CableAgeing2/kicad/rs-232_opto_adapter_voltage/


A compiler will be needed. Its possible that all you need to do is to install gcc-arm-none-eabi like this:
  sudo apt install gcc-arm-none-eabi




If using Nucleo the device will show up as an USB flash drive when connected.
Just copy and paste the binary.bin file over. 

If all goes well it shall appear and then disapear after about 5 seconds.
Just reboot the device after that.



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


See also github:
https://github.com/xehp/drekkar_stm32_scpi


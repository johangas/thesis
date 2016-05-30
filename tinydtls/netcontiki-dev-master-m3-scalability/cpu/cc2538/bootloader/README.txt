


Build bootloader and first firmware image
=========================================
change working directory to bootloader:
cd cpu/cc2538/bootloader

Build any firmware image with IMAGE=1, for instance udp-client from examples felicia:
(cd ../../../examples/ipv6/felicia/rpl-udp/ ; make clean IMAGE=1 TARGET=felicia TARGET_VARIANT=felicia udp-client.bin)

Now build bootloader:

make clean all IMAGE=0 

Create rescue image, consisting of bootloader mfg-area, firmware image
and customer configuration area (cca).

A Felicia with pushbutton on pin D6 and a LED with anode on C0 and
cathode on C1 can use manufacturing file mfg-felicia-felicia-led.hex

Still in bootloader directory, create rescue image:

../../../tools/yanzi/createRescueImage.sh -B bootloader -m mfg-felicia-felicia.hex -i ../../../examples/ipv6/felicia/rpl-udp/felicia.1.flash

Now there is a rescueimage.bin in current directory.

Write rescue image to device
============================
start JLinkExe. To access USB it has to run as root, so use sudo:

sudo /opt/SEGGER/JLink/JLinkExe -if jtag -device CC2538SF53

First erase chip with command "erase".
Keep trying erase until the response is "Erasing done."
JLinkExe might have to be restarted a few times.

When erase is done, write resueimage:

loadbin rescueimage.bin 0x00200000

if "J-Link>" propmpt return immediately without any respons, restart JLinkExe.

The respons should be
"Info: J-Link: Flash download: Flash programming performed for 3 ranges (49152 bytes)"

Where 49152 the the sum of the size of all sectors modified. This
number depends on the size of the firmware image.

That is it.
Toggle power to restart node.

If the mfg-area configures a LED and your board has one,
it will flash once when loading image 1 and twice when loading image 2.


================================================================
================================================================

Information for manually write images.
======================================

Addresses
=========
0x00280000  End of flash.
0x0027ffd0  cca, custmer configuration area.
0x0027f7ff  End of second image.
0x00242000  Start of second image.
0x00241fff  End of first image.
0x00204000  Start of First image.
0x00200000  Bootloader and mfg-area


Using Jlink
============
start JLinkExe with:

sudo /opt/SEGGER/JLink/JLinkExe -if jtag -device CC2538SF53

Usefull JLinkExe commands:
--------------------------
erase                   Erase all flash areas
r                       reset target
halt                    halt target
loadbin <file> Address  Write a binary file to a given address. Flash is erased as needed.
                        Filename MUST end in .bin
			Address is always hex, even without initial 0x.

When writing bootloader and mfg-area; concatinate them before writing.
Alternatively if bootloader binary is smaller than 4kBytes, mfg binary can be
written to addres 0x201000, and updated individually from bootloader.
Sector size is 2048 bytes.

If chip is erased, cca (customer configuration area) must be written to enable execution from flash.

Write the following 48 bytes to address 0x0027ffd0:

ff ff ff ff ff ff ff f3  00 00 00 00 00 00 20 00
ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff
ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff

Create cca binary with :

echo "fffffffffffffff30000000000002000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff" | java -jar ../../../tools/yanzi/BinFileTool.jar -o cca.bin

Then write it with JLink command:
loadbin cca.bin 0x0027ffd0

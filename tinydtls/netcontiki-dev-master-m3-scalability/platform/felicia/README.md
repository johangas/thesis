Getting Started with NetContiki for Yanzi Felicia
=================================================

This guide's aim is to help you start using NetContiki for Yanzi
Felicia nodes. The platform supports various types of boards based on
TI CC2538 System-on-Chip.

* Felicia - the base board
* IoT-U10 - board with leds, buttons, temperature sensor
* IoT-U10+ - board with leds, buttons, temperature sensor, and CC2592 Range Extender.

Requirements
============

To start using NetContiki, you will need the following:

* A toolchain to compile for CC2538.

* Drivers so that your OS can communicate with the hardware via USB. Many OS already have the drivers pre-installed while others might require you to install additional drivers. Please read the section ["Drivers" in the CC2538DK readme](https://github.com/contiki-os/contiki/tree/master/platform/cc2538dk#for-the-cc2538em-usb-cdc-acm). Only the driver for CC2538EM (USB CDC-ACM) is needed.


Use the platform
================

Starting border router
--------------------------------------------------

To update a node using the over-the-air upgrade mechanism, it needs to be connected in an IPv6 network.
The border router with support for OTA is under yanzi/products and it needs to be started to setup a network for the nodes to join.

    > cd yanzi/products/yanzi-border-router
    > make connect-high

You need to have an updated IoT-U10/IoT-U10+ node with the serial radio
software connected via USB. You might need to specify a different USB
port if the node is not connected to the first port.

    > make connect-high PORT=/dev/ttyACM1

Build the image of the remote mote
--------------------------------------------------

Switch to the folder of the project you want to build. For example, to
build an image for the IoT-U10 node, switch to yanzi/products/iot-u10/
The Makefile contains all the necessary information for building the
image(s). You need the following options when building the images:

 FOR: For felicia platform, you must select the target variant,
 e.g. iot-u10 or iot-u10plus.

 INC: The sequence number of the images. When doing an over-the-air
 upgrade, the device will reboot to the latest image based on date. If
 both images have the same date, the device will boot to the image
 with the highest INC number. The default INC number when not
 specified is 0.

Example:

    > cd yanzi/products/iot-u10/
    > make clean FOR=iot-u10 images INC=3

The image archive is created in the working directory and named
"feliciaFirmware.jar".

Over the air upgrade
--------------------------------------------------

There is a convenient make rule to upload to a node:

    > make FOR=iot-u10 images
    > make FOR=iot-u10 upload DST=<aaaa::XXXX:XXXX:XXXX>

DST should be the IPv6 address to the node to update. After the update, the node will reboot and at startup it will blink once with the yellow led if it boots on the first image and twice if it boots on the second image. To see the status on a node and which image is currently used:

    > make FOR=iot-u10 status DST=<aaaa::XXXX:XXXX:XXXX>

Over the air upgrade tool
--------------------------------------------------

The tool for over the air upgrade is located under /tools/yanzi and is
a jar file. The command to start the over-the-air upgrade is as
follows:

    > java -jar ../../../tools/yanzi/TLVupgrade.jar -i [the image archive] -a aaaa::XXXX:XXXX:XXXX

You will be able to monitor the update process in your terminal. The
upgrade performs a write of the images and then performs a read. If
the read succeeds, the device will reboot and the bootloader will boot
from the newest image. If the read fails, while the write succeeded,
you have managed to upgrade the firmware, but the device will not
automatically reboot.

If you only want to check the image status of the node, you type:

    > java -jar ../../../tools/yanzi/TLVupgrade.jar -s -a
aaaa::XXXX:XXXX:XXXX

There is a nice "usage" command on this jar file, so you can see how,
for example you can change the speed of the upgrade process (using the
-t option and modifying the upgrade timing details, e.g. chunck sizes,
inter-chunk times etc.)

Note that over-the-air upgrade assumes that you have a border router
running (so the tun0 interface is up) and the remote mote has joined
the RPL instance of the border router. For that, the serial radio must
be tuned to the channel and PAN id of the remote mote. By default
channel 26 and PAN id 0xabcd are used by both the serial radio and the
remote Yanzi mote products (IoT-U10, IoT-U10+, etc.).

To verify connectivity, try to ping the remote mote before attempting
a firmware upgrade.

Updating the serial radio
------------------------------------------------------------

The serial radio and border router with support for OTA is under
yanzi/products/yanzi-serial-radio and yanzi/products/yanzi-border-router.
To use these you need an updated IoT-U10/IoT-U10+ serial radio.

    > cd yanzi/products/yanzi-serial-radio
    > make FOR=iot-u10 clean images INC=1
    > make FOR=iot-u10 upload DST=127.0.0.1

There is a convenient make rule for updating a locally connected serial radio with higher speed:

    > make FOR=iot-u10 upload-local

To upgrade you need to have the border router running and connected to the serial-radio.

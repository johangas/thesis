#!/bin/bash -e
#
# Edit the grub.conf file (/media/mmcblk0p1/boot/grub/grub.conf), 
# append the following parameter to the kernel command line 
# (the line beginning with "kernel" for the relevant grub entry), 
# and reboot:
#	
# intel_qrk_plat_galileo_gen2.gpio_cs=1
#
# This is to let the SPI driver use IO10 as chip-select control.
# For reason why we need to configure this boot time kernel parameter to enable IO10 as SPI Chip Select pin, 
# please refer to the following webpage for more explanations: 
# http://www.emutexlabs.com/component/content/article?id=203:getting-started-with-intel-galileo-gen-2
#

echo "Setting up Spidev1.0 for Galileo-Gen2..."

#pin10 (SPI_CS) (OUT)
echo 26 > /sys/class/gpio/export || echo "gpio26 already exported"
echo 74 > /sys/class/gpio/export || echo "gpio74 already exported"
echo 27 > /sys/class/gpio/export || echo "gpio27 already exported"
echo low > /sys/class/gpio/gpio26/direction || echo "Failed to set gpio26 low"
echo "0" > /sys/class/gpio/gpio74/value || echo "Failed to set gpio74 low"
echo in > /sys/class/gpio/gpio27/direction || echo "Failed to set gpio27 direction in"

#pin11 (SPI_MOSI) (OUT)
echo 24 > /sys/class/gpio/export || echo "gpio24 already exported"
echo 44 > /sys/class/gpio/export || echo "gpio44 already exported"
echo 72 > /sys/class/gpio/export || echo "gpio72 already exported"
echo 25 > /sys/class/gpio/export || echo "gpio25 already exported"
echo low > /sys/class/gpio/gpio24/direction || echo "Failed to set gpio24 low"
echo high > /sys/class/gpio/gpio44/direction || echo "Failed to set gpio44 high"
echo "0" > /sys/class/gpio/gpio72/value || echo "Failed to set gpio72 low"
echo in > /sys/class/gpio/gpio25/direction || echo "Failed to set gpio25 direction in"

#pin12 (SPI_MISO) (IN)
echo 42 > /sys/class/gpio/export || echo "gpio42 already exported"
echo 43 > /sys/class/gpio/export || echo "gpio43 already exported"
echo high > /sys/class/gpio/gpio42/direction || echo "Failed to set gpio42 low"
echo in > /sys/class/gpio/gpio43/direction || echo "Failed to set gpio43 direction in"

#pin13 (SPI_SCK) (OUT)
echo 30 > /sys/class/gpio/export || echo "gpio30 already exported"
echo 46 > /sys/class/gpio/export || echo "gpio46 already exported"
echo 31 > /sys/class/gpio/export || echo "gpio31 already exported"
echo low > /sys/class/gpio/gpio30/direction || "Failed to set gpio30 low"
echo high > /sys/class/gpio/gpio46/direction || "Failed to set gpio46 high"
echo in > /sys/class/gpio/gpio31/direction || "Failed to set gpio31 direction in"

#echo "Spi ready"


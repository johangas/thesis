echo Configure the Intel Galileo GPIO MUX to SPI functions

### IO10_MUX = 0 (SPI_CS)
echo -n "42" > /sys/class/gpio/export
echo -n "out" > /sys/class/gpio/gpio42/direction
echo -n "strong" > /sys/class/gpio/gpio42/drive
echo -n "0" > /sys/class/gpio/gpio42/value

### IO11_MUX = 0 (SPI_MOSI)
echo -n "43" > /sys/class/gpio/export
echo -n "out" > /sys/class/gpio/gpio43/direction
echo -n "strong" > /sys/class/gpio/gpio43/drive
echo -n "0" > /sys/class/gpio/gpio43/value

### IO12_MUX = 0 (SPI_MISO)
echo -n "54" > /sys/class/gpio/export
echo -n "out" > /sys/class/gpio/gpio54/direction
echo -n "strong" > /sys/class/gpio/gpio54/drive
echo -n "0" > /sys/class/gpio/gpio54/value

### IO13_MUX = 0 (SPI_SCK)
echo -n "55" > /sys/class/gpio/export
echo -n "out" > /sys/class/gpio/gpio55/direction
echo -n "strong" > /sys/class/gpio/gpio55/drive
echo -n "0" > /sys/class/gpio/gpio55/value

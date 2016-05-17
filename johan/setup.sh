#compile and flash

if($WITH_DTLS) then
  cd dtls-authority
else 
  cd authority
fi
make authority.flash TARGET=nrf52dk NRF52_JLINK_SN= #TODO

cd ..

if($WITH_DTLS) then
  cd dtls-keypad
else
  cd keypad
fi

make keypad.flash TARGET=nrf52dk ADDR=2001:db8::225:40ff:fef0:8bf0 NRF52_JLINK_SN= #TODO

cd ..

#setup bluetooth driver
sudo mount -t debugfs none /sys/kernel/debug

sudo modprobe bluetooth_6lowpan

sudo echo 35 > /sys/kernel/debug/bluetooth/6lowpan_psm

#Connect
sudo echo "connect 00:00:F7:D1:AB:65 1" > /sys/kernel/debug/bluetooth/6lowpan_control
#TODO wait needed?
sleep 3s
sudo echo "connect 00:25:40:F0:8B:F0 1" > /sys/kernel/debug/bluetooth/6lowpan_control

#forwarding
sudo echo 1 > /proc/sys/net/ipv6/conf/all/forwarding
sudo sysctl -w net.ipv6.conf.all.forwarding=1

#restartd routing daemon
sudo service radvd restart

sudo ifconfig bt0 add 2001:db8::1/64
sudo ifconfig bt1 add 2001:db8::1/64
sudo ifconfig bt2 add 2001:db8::1/64

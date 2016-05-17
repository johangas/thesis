#Compile and flash
#Make requires correct directory
export WITH_DTLS=1

cd dtls-authority
make erase TARGET=nrf52dk NRF52_JLINK_SN=682049342
make softdevice.flash TARGET=nrf52dk NRF52_JLINK_SN=682049342
make authority.flash TARGET=nrf52dk NRF52_JLINK_SN=682049342
cd ..

cd dtls-keypad
make erase TARGET=nrf52dk NRF52_JLINK_SN=682531037
make softdevice.flash TARGET=nrf52dk NRF52_JLINK_SN=682531037
make keypad.flash TARGET=nrf52dk ADDR=2001:db8::225:40ff:fef0:8bf0 NRF52_JLINK_SN=682531037
cd ..

#setup bluetooth driver
sudo mount -t debugfs none /sys/kernel/debug
sudo modprobe bluetooth_6lowpan
sudo echo 35 > /sys/kernel/debug/bluetooth/6lowpan_psm

#Connect
sudo echo "connect 00:00:F7:D1:AB:65 1" > /sys/kernel/debug/bluetooth/6lowpan_control
sleep 2s
sudo echo "connect 00:25:40:F0:8B:F0 1" > /sys/kernel/debug/bluetooth/6lowpan_control

#IPv6 forwarding
sudo echo 1 > /proc/sys/net/ipv6/conf/all/forwarding
sudo sysctl -w net.ipv6.conf.all.forwarding=1

#Restart routing daemon
sudo service radvd restart

#Set routing prefix
sudo ifconfig bt0 add 2001:db8::1/64
sudo ifconfig bt1 add 2001:db8::1/64
sudo ifconfig bt2 add 2001:db8::1/64

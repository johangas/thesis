#setup bluetooth driver
mount -t debugfs none /sys/kernel/debug

modprobe bluetooth_6lowpan

echo 35 > /sys/kernel/debug/bluetooth/6lowpan_psm

#forwarding
echo 1 > /proc/sys/net/ipv6/conf/all/forwarding
sysctl -w net.ipv6.conf.all.forwarding=1

#restartd routing daemon
service radvd restart

ifconfig bt0 add 2001:db8::1/64

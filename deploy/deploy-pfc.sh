echo "Install pmacFilterControl"
ssh root@172.23.107.226 'mkdir -p /root/prefix/'
scp -r arm_prefix/* root@172.23.107.226:/root/prefix/
echo "Install to /.readonly"
ssh root@172.23.107.226 'mount -o remount,rw / && mkdir -p /.readonly/root/prefix/'
scp -r arm_prefix/* root@172.23.107.226:/.readonly/root/prefix/
ssh root@172.23.107.226 'mount -o remount,ro /'
echo "Reboot"
ssh root@172.23.107.226 'reboot'

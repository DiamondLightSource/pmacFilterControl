echo "Install procServ"
scp /dls_sw/prod/targetOS/tar-files/procServ-2.7.0.tar.gz root@172.23.107.226:/tmp
ssh root@172.23.107.226 'cd /tmp && tar -xzf procServ-2.7.0.tar.gz && rm procServ-2.7.0.tar.gz && cd procServ-2.7.0 && mkdir -p prefix && ./configure --enable-access-from-anywhere && make install'
echo "Install procServ init.d script"
ssh root@172.23.107.226 'mount -o remount,rw /'
scp $(dirname "$(readlink -fn "$0")")/procServ root@172.23.107.226:/.readonly/etc/init.d/procServ
ssh root@172.23.107.226 'chmod a+x /.readonly/etc/init.d/procServ'
ssh root@172.23.107.226 'mount -o remount,ro /'
echo "Reboot"
ssh root@172.23.107.226 'reboot'

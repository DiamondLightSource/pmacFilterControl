# Make ssh key

```{bash}
❯ ssh-keygen
Generating public/private rsa key pair.
Enter file in which to save the key (/home/mef65357/.ssh/id_rsa): id_ppmac
Enter passphrase (empty for no passphrase):
Enter same passphrase again:
Your identification has been saved in id_ppmac.
Your public key has been saved in id_ppmac.pub.
...

❯ ssh-copy-id -f -i ~/.ssh/id_ppmac.pub root@<IP>
/usr/bin/ssh-copy-id: INFO: Source of key(s) to be installed: "/home/mef65357/.ssh/id_ppmac.pub"
Loading keys:
    /home/mef65357/.ssh/id_ppmac
All keys already loaded
root@172.23.107.175's password:

Number of key(s) added: 1

Now try logging into the machine, with:   "ssh 'root@172.23.107.175'"
and check to make sure that only the key(s) you wanted were added.

❯ ssh root@172.23.107.175
All keys already loaded
Linux ppmac 4.1.18-ipipe #102 SMP Mon Mar 19 11:24:48 PDT 2018 armv7l

The programs included with the Debian GNU/Linux system are free software;
the exact distribution terms for each program are described in the
individual files in /usr/share/doc/*/copyright.

Debian GNU/Linux comes with ABSOLUTELY NO WARRANTY, to the extent
permitted by applicable law.
Last login: Wed Feb 12 17:26:42 2020 from 172.23.245.118
root@172.23.107.175:/opt/ppmac#
```

You may need to fix the permissions on the ppmac:

```{bash}
root@172.23.107.175:/opt/ppmac# chmod g-w $HOME
root@172.23.107.175:/opt/ppmac# chmod o-w $HOME
root@172.23.107.175:/opt/ppmac# ls -l /
total 41
...
drwxr-xr-x  2 root root  4096 Jan  1  1970 home
...
```

You can then run `copy_pmac_libs.sh` without having to enter the password for every
file:

```{bash}
❯ thirdparty/copy_ppmac_libs.sh 172.23.107.175
libmath.so    100%   69KB    6.8MB/s   00:00
libopener.a   100%   139KB   7.6MB/s   00:00
cipcommon.h   100%   3879    1.4MB/s   00:00
...
```

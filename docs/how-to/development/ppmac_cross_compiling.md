# Notes on cross compiling for ARM PowerPMAC

## Linker Flags

To make sure the binary will run when copied across to the ppmac, the ARM libs are
copied as well, but they will not be used by default. Linker flags are provided to
embed the dynamic linker and the library paths within the binary. These are:

- `-Wl,--disable-new-dtags,-rpath` | Set search paths for libraries
- `-Wl,-dynamic-linker` | Set dynamic linker

The `--disable-new-dtags` flag is used to set `RPATH` rather than `RUNPATH`, which seems
to be necesary to acheive the desired behaviour when running on the ppmac. The binary
can be expected with readelf to confirm the RPATH is set:`

```bash
root@172.23.107.175:/opt/ppmac# readelf -d /root/prefix/bin/pmacFilterControl

Dynamic section at offset 0x8fbc8 contains 36 entries:
  Tag        Type                         Name/Value
 0x00000001 (NEEDED)                     Shared library: [libzmq.so.5]
 0x00000001 (NEEDED)                     Shared library: [libpthread.so.0]
 0x00000001 (NEEDED)                     Shared library: [libppmac.so]
 0x00000001 (NEEDED)                     Shared library: [libpthread_rt.so.1]
 0x00000001 (NEEDED)                     Shared library: [libxenomai.so.0]
 0x00000001 (NEEDED)                     Shared library: [libstdc++.so.6]
 0x00000001 (NEEDED)                     Shared library: [libgcc_s.so.1]
 0x00000001 (NEEDED)                     Shared library: [libc.so.6]
 0x00000001 (NEEDED)                     Shared library: [ld-linux-armhf.so.3]
 0x0000000f (RPATH)                      Library rpath: [/root/prefix/lib/:/root/libzmq/prefix/lib/:/root/lib]
...
```

`ldd` is also useful to see what the flags change:

- With the correct flags (and the application runs without setting `LD_LIBRARY_PATH):
```bash
root@172.23.107.175:/opt/ppmac# ldd /root/prefix/bin/pmacFilterControl
    libzmq.so.5 => /root/libzmq/prefix/lib/libzmq.so.5 (0x76e4a000)
    libpthread.so.0 => /root/lib/libpthread.so.0 (0x76e24000)
    libppmac.so => /root/prefix/lib/libppmac.so (0x76ab9000)
    libpthread_rt.so.1 => /root/prefix/lib/libpthread_rt.so.1 (0x76a9f000)
    libxenomai.so.0 => /root/prefix/lib/libxenomai.so.0 (0x76a89000)
    libstdc++.so.6 => /root/lib/libstdc++.so.6 (0x7693f000)
    libgcc_s.so.1 => /root/lib/libgcc_s.so.1 (0x76916000)
    libc.so.6 => /root/lib/libc.so.6 (0x76817000)
    /root/lib/ld-linux-armhf.so.3 => /lib/ld-linux-armhf.so.3 (0x54b12000)
    libdl.so.2 => /root/lib/libdl.so.2 (0x76804000)
    libmath.so => /opt/ppmac/libmath/libmath.so (0x767e5000)
    librt.so.1 => /root/lib/librt.so.1 (0x767ce000)
    libm.so.6 => /root/lib/libm.so.6 (0x76765000)
root@172.23.107.175:/opt/ppmac# /root/prefix/bin/pmacFilterControl
Must pass control_port and data_endpoint - e.g. '10000 127.0.0.1:10000'
```

- Without `--disable-new-dtags`:
```bash
root@172.23.107.175:/opt/ppmac# ldd /root/prefix/bin/pmacFilterControl
/root/prefix/bin/pmacFilterControl: /lib/arm-linux-gnueabihf/libm.so.6: version GLIBC_2.29 not found (required by /root/lib/libstdc++.so.6)
    libzmq.so.5 => /root/libzmq/prefix/lib/libzmq.so.5 (0x76e1e000)
    libpthread.so.0 => /root/lib/libpthread.so.0 (0x76df8000)
    libppmac.so => /root/prefix/lib/libppmac.so (0x76a8d000)
    libpthread_rt.so.1 => /root/prefix/lib/libpthread_rt.so.1 (0x76a73000)
    libxenomai.so.0 => /root/prefix/lib/libxenomai.so.0 (0x76a5d000)
    libstdc++.so.6 => /root/lib/libstdc++.so.6 (0x76913000)
    libgcc_s.so.1 => /root/lib/libgcc_s.so.1 (0x768ea000)
    libc.so.6 => /root/lib/libc.so.6 (0x767eb000)
    /root/lib/ld-linux-armhf.so.3 => /lib/ld-linux-armhf.so.3 (0x54b36000)
    libdl.so.2 => /lib/arm-linux-gnueabihf/libdl.so.2 (0x767d2000)
    libmath.so => /opt/ppmac/libmath/libmath.so (0x767b3000)
    librt.so.1 => /lib/arm-linux-gnueabihf/librt.so.1 (0x7679c000)
    libm.so.6 => /lib/arm-linux-gnueabihf/libm.so.6 (0x76728000)
```

- Without rpath or dynamic linker set:
```bash
root@172.23.107.175:/opt/ppmac# ldd /root/prefix/bin/pmacFilterControl
/root/prefix/bin/pmacFilterControl: /usr/lib/arm-linux-gnueabihf/libstdc++.so.6: version GLIBCXX_3.4.22 not found (required by /root/prefix/bin/pmacFilterControl)
/root/prefix/bin/pmacFilterControl: /usr/lib/arm-linux-gnueabihf/libstdc++.so.6: version GLIBCXX_3.4.21 not found (required by /root/prefix/bin/pmacFilterControl)
	libzmq.so.5 => not found
	libpthread.so.0 => /lib/arm-linux-gnueabihf/libpthread.so.0 (0x76e95000)
	libppmac.so => not found
	libpthread_rt.so.1 => /usr/xenomai/lib/libpthread_rt.so.1 (0x76e7b000)
	libxenomai.so.0 => /usr/xenomai/lib/libxenomai.so.0 (0x76e65000)
	libstdc++.so.6 => /usr/lib/arm-linux-gnueabihf/libstdc++.so.6 (0x76dae000)
	libgcc_s.so.1 => /lib/arm-linux-gnueabihf/libgcc_s.so.1 (0x76d84000)
	libc.so.6 => /lib/arm-linux-gnueabihf/libc.so.6 (0x76c94000)
	/lib/ld-linux-armhf.so.3 (0x54b3c000)
	librt.so.1 => /lib/arm-linux-gnueabihf/librt.so.1 (0x76c7e000)
	libm.so.6 => /lib/arm-linux-gnueabihf/libm.so.6 (0x76c0a000)
```

## Wrapped symbols

The ppmac libraries are linked using the flags `-Wl,--wrap,<function>`, which invoke some
linker magic to allow the linker to ignore resolving the given symbol and it can then be
provided by another library when linking the final binary. This seems to be used to
patch kernel functions with real-time code using xenomai. Client applications must be
linked against these xenomai libs - without it gcc will give link errors such as, e.g.,
`libppmac.so: undefined reference to __wrap_listen`.

### Example build through Power PMAC IDE

The following snippet from the build log in the PowerPMAC IDE shows these flags being
used and linking against `xenomai`.

```bash
arm-omron49-linux-gnueabihf-gcc -mhard-float -funsigned-char --sysroot=/opt/armv71-4.1.18-ipipe -I/usr/local/dtlibs/rtpmac -I/usr/local/dtlibs/libppmac -I/usr/local/dtlibs/libopener -I/opt/armv71-4.1.18-ipipe/usr/xenomai/include -I/opt/armv71-4.1.18-ipipe/usr/xenomai/include/posix  -I/usr/local/dtlibs/libmath -D_GNU_SOURCE -D_REENTRANT -D__XENO__  -DOPENER_SUPPORT_64BIT_DATATYPES  -g3 -c capp1.c -o capp1.o
arm-omron49-linux-gnueabihf-gcc -o "../../../Bin/Debug/capp1.out" capp1.o -ldl -lppmac -lpthread_rt -lxenomai -lpthread -lgcc_s -lmath -lm -L/usr/local/dtlibs/libmath --sysroot=/opt/armv71-4.1.18-ipipe -L../../../Bin/Debug/ -L/opt/armv71-4.1.18-ipipe/usr/xenomai/lib -L/usr/local/dtlibs/libppmac -L/usr/local/dtlibs/rtpmac -Wl,-rpath,/var/ftp/usrflash/Project/C\ Language/Libraries -Wl,-rpath,/var/ftp/usrflash/Project/Bin/Debug -Wl,-rpath,/opt/ppmac/libppmac -Wl,-rpath,/opt/ppmac/libmath -Wl,-rpath-link,/opt/armv71-4.1.18-ipipe/lib/arm-linux-gnueabihf -Wl,--wrap,shm_open -Wl,--wrap,pthread_create -Wl,--wrap,pthread_create -Wl,--wrap,pthread_setschedparam -Wl,--wrap,pthread_getschedparam -Wl,--wrap,pthread_yield -Wl,--wrap,sched_yield -Wl,--wrap,pthread_kill -Wl,--wrap,sem_init -Wl,--wrap,sem_destroy -Wl,--wrap,sem_post -Wl,--wrap,sem_timedwait -Wl,--wrap,sem_wait -Wl,--wrap,sem_trywait -Wl,--wrap,sem_getvalue -Wl,--wrap,sem_open -Wl,--wrap,sem_close -Wl,--wrap,sem_unlink -Wl,--wrap,clock_getres -Wl,--wrap,clock_gettime -Wl,--wrap,clock_settime -Wl,--wrap,clock_nanosleep -Wl,--wrap,nanosleep -Wl,--wrap,pthread_mutexattr_init -Wl,--wrap,pthread_mutexattr_destroy -Wl,--wrap,pthread_mutexattr_gettype -Wl,--wrap,pthread_mutexattr_settype -Wl,--wrap,pthread_mutexattr_getprotocol -Wl,--wrap,pthread_mutexattr_setprotocol -Wl,--wrap,pthread_mutexattr_getpshared -Wl,--wrap,pthread_mutexattr_setpshared -Wl,--wrap,pthread_mutex_init -Wl,--wrap,pthread_mutex_destroy -Wl,--wrap,pthread_mutex_lock -Wl,--wrap,pthread_mutex_trylock -Wl,--wrap,pthread_mutex_timedlock -Wl,--wrap,pthread_mutex_unlock -Wl,--wrap,pthread_condattr_init -Wl,--wrap,pthread_condattr_destroy -Wl,--wrap,pthread_condattr_getclock -Wl,--wrap,pthread_condattr_setclock -Wl,--wrap,pthread_condattr_getpshared -Wl,--wrap,pthread_condattr_setpshared -Wl,--wrap,pthread_cond_init -Wl,--wrap,pthread_cond_destroy -Wl,--wrap,pthread_cond_wait -Wl,--wrap,pthread_cond_timedwait -Wl,--wrap,pthread_cond_signal -Wl,--wrap,pthread_cond_broadcast -Wl,--wrap,mq_open -Wl,--wrap,mq_close -Wl,--wrap,mq_unlink -Wl,--wrap,mq_getattr -Wl,--wrap,mq_setattr -Wl,--wrap,mq_send -Wl,--wrap,mq_timedsend -Wl,--wrap,mq_receive -Wl,--wrap,mq_timedreceive -Wl,--wrap,mq_notify -Wl,--wrap,open -Wl,--wrap,socket -Wl,--wrap,close -Wl,--wrap,ioctl -Wl,--wrap,read -Wl,--wrap,write -Wl,--wrap,recvmsg -Wl,--wrap,sendmsg -Wl,--wrap,recvfrom -Wl,--wrap,sendto -Wl,--wrap,recv -Wl,--wrap,send -Wl,--wrap,getsockopt -Wl,--wrap,setsockopt -Wl,--wrap,bind -Wl,--wrap,connect -Wl,--wrap,listen -Wl,--wrap,accept -Wl,--wrap,getsockname -Wl,--wrap,getpeername -Wl,--wrap,shutdown -Wl,--wrap,timer_create -Wl,--wrap,timer_delete -Wl,--wrap,timer_settime -Wl,--wrap,timer_getoverrun -Wl,--wrap,timer_gettime -Wl,--wrap,ftruncate -Wl,--wrap,ftruncate64 -Wl,--wrap,close -Wl,--wrap,shm_open -Wl,--wrap,shm_unlink -Wl,--wrap,mmap -Wl,--wrap,mmap64 -Wl,--wrap,munmap -Wl,--wrap,select
```

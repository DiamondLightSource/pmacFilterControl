# Notes on cross compiling for ARM PowerPMAC

The flags `-Wl,--wrap,<function>` invoke some linker magic to patch the given function.
This seems to be used to patch real-time code using xenomai. Client applications must be
linked against these xenomai libs - without it gcc will give link errors such as, e.g.,
`libppmac.so: undefined reference to __wrap_listen`.

## Example build through Power PMAC IDE

```{bash}
arm-omron49-linux-gnueabihf-gcc -mhard-float -funsigned-char --sysroot=/opt/armv71-4.1.18-ipipe -I/usr/local/dtlibs/rtpmac -I/usr/local/dtlibs/libppmac -I/usr/local/dtlibs/libopener -I/opt/armv71-4.1.18-ipipe/usr/xenomai/include -I/opt/armv71-4.1.18-ipipe/usr/xenomai/include/posix  -I/usr/local/dtlibs/libmath -D_GNU_SOURCE -D_REENTRANT -D__XENO__  -DOPENER_SUPPORT_64BIT_DATATYPES  -g3 -c capp1.c -o capp1.o
arm-omron49-linux-gnueabihf-gcc -o "../../../Bin/Debug/capp1.out" capp1.o -ldl -lppmac -lpthread_rt -lxenomai -lpthread -lgcc_s -lmath -lm -L/usr/local/dtlibs/libmath --sysroot=/opt/armv71-4.1.18-ipipe -L../../../Bin/Debug/ -L/opt/armv71-4.1.18-ipipe/usr/xenomai/lib -L/usr/local/dtlibs/libppmac -L/usr/local/dtlibs/rtpmac -Wl,-rpath,/var/ftp/usrflash/Project/C\ Language/Libraries -Wl,-rpath,/var/ftp/usrflash/Project/Bin/Debug -Wl,-rpath,/opt/ppmac/libppmac -Wl,-rpath,/opt/ppmac/libmath -Wl,-rpath-link,/opt/armv71-4.1.18-ipipe/lib/arm-linux-gnueabihf -Wl,--wrap,shm_open -Wl,--wrap,pthread_create -Wl,--wrap,pthread_create -Wl,--wrap,pthread_setschedparam -Wl,--wrap,pthread_getschedparam -Wl,--wrap,pthread_yield -Wl,--wrap,sched_yield -Wl,--wrap,pthread_kill -Wl,--wrap,sem_init -Wl,--wrap,sem_destroy -Wl,--wrap,sem_post -Wl,--wrap,sem_timedwait -Wl,--wrap,sem_wait -Wl,--wrap,sem_trywait -Wl,--wrap,sem_getvalue -Wl,--wrap,sem_open -Wl,--wrap,sem_close -Wl,--wrap,sem_unlink -Wl,--wrap,clock_getres -Wl,--wrap,clock_gettime -Wl,--wrap,clock_settime -Wl,--wrap,clock_nanosleep -Wl,--wrap,nanosleep -Wl,--wrap,pthread_mutexattr_init -Wl,--wrap,pthread_mutexattr_destroy -Wl,--wrap,pthread_mutexattr_gettype -Wl,--wrap,pthread_mutexattr_settype -Wl,--wrap,pthread_mutexattr_getprotocol -Wl,--wrap,pthread_mutexattr_setprotocol -Wl,--wrap,pthread_mutexattr_getpshared -Wl,--wrap,pthread_mutexattr_setpshared -Wl,--wrap,pthread_mutex_init -Wl,--wrap,pthread_mutex_destroy -Wl,--wrap,pthread_mutex_lock -Wl,--wrap,pthread_mutex_trylock -Wl,--wrap,pthread_mutex_timedlock -Wl,--wrap,pthread_mutex_unlock -Wl,--wrap,pthread_condattr_init -Wl,--wrap,pthread_condattr_destroy -Wl,--wrap,pthread_condattr_getclock -Wl,--wrap,pthread_condattr_setclock -Wl,--wrap,pthread_condattr_getpshared -Wl,--wrap,pthread_condattr_setpshared -Wl,--wrap,pthread_cond_init -Wl,--wrap,pthread_cond_destroy -Wl,--wrap,pthread_cond_wait -Wl,--wrap,pthread_cond_timedwait -Wl,--wrap,pthread_cond_signal -Wl,--wrap,pthread_cond_broadcast -Wl,--wrap,mq_open -Wl,--wrap,mq_close -Wl,--wrap,mq_unlink -Wl,--wrap,mq_getattr -Wl,--wrap,mq_setattr -Wl,--wrap,mq_send -Wl,--wrap,mq_timedsend -Wl,--wrap,mq_receive -Wl,--wrap,mq_timedreceive -Wl,--wrap,mq_notify -Wl,--wrap,open -Wl,--wrap,socket -Wl,--wrap,close -Wl,--wrap,ioctl -Wl,--wrap,read -Wl,--wrap,write -Wl,--wrap,recvmsg -Wl,--wrap,sendmsg -Wl,--wrap,recvfrom -Wl,--wrap,sendto -Wl,--wrap,recv -Wl,--wrap,send -Wl,--wrap,getsockopt -Wl,--wrap,setsockopt -Wl,--wrap,bind -Wl,--wrap,connect -Wl,--wrap,listen -Wl,--wrap,accept -Wl,--wrap,getsockname -Wl,--wrap,getpeername -Wl,--wrap,shutdown -Wl,--wrap,timer_create -Wl,--wrap,timer_delete -Wl,--wrap,timer_settime -Wl,--wrap,timer_getoverrun -Wl,--wrap,timer_gettime -Wl,--wrap,ftruncate -Wl,--wrap,ftruncate64 -Wl,--wrap,close -Wl,--wrap,shm_open -Wl,--wrap,shm_unlink -Wl,--wrap,mmap -Wl,--wrap,mmap64 -Wl,--wrap,munmap -Wl,--wrap,select
```
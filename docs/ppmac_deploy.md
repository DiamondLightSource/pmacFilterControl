# Bulding pmacFilterControl for x86

pmacFilterControl includes architecture-specific code blocks and the build detects the
architecture at compile time. This means the same code can be built and deployed for the
target and built for host to test against locally. The build uses cmake - this is
configured in `.vscode/settings.json` so can be built using the [CMake extension](https://marketplace.visualstudio.com/items?itemName=twxs.cmake).
To build manually:

```
$ mkdir prefix
$ mkdir build && cd build
$ cmake .. -DCMAKE_INSTALL_PREFIX=../prefix
$ make install
```

# Bulding pmacFilterControl for ppmac

A docker container is defined in `tools/Dockerfile` to provide a cross-compiling
toolchain for ARM. This can be used to build pmacFilterControl for the ppmac. There are
vscode tasks to build the container itself and for running commands inside the
container.

To build the container run the `Build ARM Container` task - this will create an
`arm-crosscompiler` container.

To build pmacFilterControl for ARM run the `Run command in ARM container` task and
select `build` - it should be installed into `arm_prefix`.

# Deploying pmacFilterControl to ppmac

The `arm_prefix` directory can be copied to the ppmac to run pmacFilterControl.

- Run the `Run command in ARM container` task and select `bash`
- In the container bash prompt, run
  - `$ scp -r arm_prefix/* root@<ppmac ip>:/root/prefix/`

Make sure the application isn't already running or the copy may fail with the following
error:

```{bash}
$ scp -r arm_prefix/* root@172.23.107.175:/root/prefix/
...
scp: /root/prefix/bin/pmacFilterControl: Text file busy
```

The zmq libraries should also be copied to `/root/` so that the application can find
them at runtime.

  - `$ scp -r libzmq/ root@<ppmac ip>:/root/`

Unfortunately the ARM toolchain does not exactly match the ppmac, so binaries compiled
in the container will not run against the libraries on the ppmac. To get around this,
all standard libs for ARM inside the container (`/usr/arm-linux-gnueabihf/lib/`) need to
be copied to the ppmac `/root/lib` directory:

- Run the `Run command in ARM container` task and select `bash`
- In the container bash prompt, run (substituting the ppmac ip address)
  - `$ scp -r /usr/arm-linux-gnueabihf/lib/ root@<ppmac ip>:/root/lib`

Then pmacFilterControl can be run on the ppmac:

```
# LD_LIBRARY_PATH=/root/lib/ /root/prefix/bin/pmacFilterControl 10001 172.23.245.118:10000
Listening on tcp://172.23.245.118:10000
Request received: {"command": "shutdown"}
Received shutdown command
Shutting down
Finished run
```

## Persistent Deployment

For testing the relevant directories can be copied directly to the `/root/` directory
and run from there, however this will be wiped if the ppmac is rebooted. This means any
test changes to `/root` can be reverted by rebooting, but it also means an extra step is
required to deploy a new version persistently.

The contents of `/.readonly/root` are copied to `/root` on boot, so to make any changes
to `/root` persistent they should be duplicated in `/.readonly/root`. This directory is
mounted read-only and must be remounted with write permissions:

```{bash}
# mount -o remount,rw /
```

then the files can simply be copied over:

```{bash}
# cp -r /root/<directory> /.readonly/root/<directory>
```

Now after a reboot the changes will remain in `/root/`.

# Power PMAC Scripts

There is a plc and a motion program that must be deployed on the ppmac. The plc is used
for monitoring and the motion program is run by the C++ application to move to the set
demands.

## Building

The plc contains macros that must be expanded using `msi` before downloading to the
ppmac. To build filter_control.plc run make in `src/plc`:

```{bash}
$ make -C src/plc
make: Entering directory `/dls_sw/work/tools/RHEL7-x86_64/pmacFilterControl/src/plc'
mkdir -p build
msi filter_control.psub > build/filter_control.plc
```

The expanded file will appear in `src/plc/build`. The motion program contains no macros
so is already valid script.

## Deployment

This is painful. There are various possible ways, but none of them work well. The manual
way is:

```{bash}
$ scp src/plc/example.plc root@<ppmac ip>:"/var/ftp/usrflash/Project/PMAC\ Script\ Language/PLC\ Programs/plc3_filter_control.plc"
# pproj -l
```

If a new file is added then it must be included in `/var/ftp/usrflash/Project/Configuration/pp_proj.ini`
and the `last_file_number` incremented to include the file number.

If the files are invalid, it may produce some information about the code snippets it
doesn't like, or it may just give a vague error. In the latter case, there might be some
useful information in the log files, e.g.:

```{bash}
# pproj -l
...
Error: projpp errors = 1
root@172.23.107.175:/opt/ppmac# cat /var/ftp/usrflash/Project/Log/pp_error.log
[PMAC_PROJECT]
/var/ftp/usrflash/Project/PMAC Script Language/PLC Programs/plc3_filter_control.plc:168:1: error #31: invalid data:  while (M140 != 1) and (M240 != 1) and (M340 != 1) and (M440 != 1) {}
Error: downloading preprocessed File: "/var/ftp/usrflash/Temp/pp_proj.pma"
```

Uploading files in this way will conflict with use of the IDE to upload things to the
ppmac and things may be overwritten if the project is uploaded from there.

#!/bin/bash

if [ -z "$1" ]; then
exit 1
fi

PPMAC_IP=$1

mkdir -p thirdparty/ppmac/
mkdir -p thirdparty/ppmac/libmath/
scp -i ~/.ssh/id_ppmac root@${PPMAC_IP}:/opt/ppmac/libmath/libmath.so thirdparty/ppmac/libmath/
mkdir -p thirdparty/ppmac/libopener/
scp -i ~/.ssh/id_ppmac root@${PPMAC_IP}:/opt/ppmac/libopener/libopener.a thirdparty/ppmac/libopener/
scp -i ~/.ssh/id_ppmac root@${PPMAC_IP}:/opt/ppmac/libopener/{cipcommon,cipconnectionobject,cipepath,cipqos,ciptypes,enipmessage,networkhandler,nvqos,opener_api,platform_network_includes,cipconnectionmanager,cipelectronickey,ciperror,ciptcpipinterface,doublylinkedlist,nvdata,nvtcpip,opener_user_conf,typedefs}.h thirdparty/ppmac/libopener/
mkdir -p thirdparty/ppmac/libppmac/
scp -i ~/.ssh/id_ppmac root@${PPMAC_IP}:/opt/ppmac/libppmac/libppmac.so thirdparty/ppmac/libppmac/
scp -i ~/.ssh/id_ppmac root@${PPMAC_IP}:/opt/ppmac/libppmac/{bkgthread,cmdthread,eipthread,gplib,mbserverlib,rtithread,rtpmaclib,status,tinyxml2_wrapper,cmdprocessor,eip,ethernetip,GpPmacStr,locations,modbus,rtpmacapi,semaphores,tinyxml2,userthread}.h thirdparty/ppmac/libppmac/
mkdir -p thirdparty/ppmac/rtpmac/
scp -i ~/.ssh/id_ppmac root@${PPMAC_IP}:/opt/ppmac/rtpmac/{ecrt,ethercat,ethercatslave,pRtGpShm,RtGpShm}.h thirdparty/ppmac/rtpmac/
mkdir -p thirdparty/xenomai/
scp -i ~/.ssh/id_ppmac root@${PPMAC_IP}:/usr/xenomai/lib/libxenomai.so.0 thirdparty/xenomai/
scp -i ~/.ssh/id_ppmac root@${PPMAC_IP}:/usr/xenomai/lib/libpthread_rt.so.1 thirdparty/xenomai/
cd thirdparty/xenomai
ln -sf libxenomai.so.0 libxenomai.so
ln -sf libpthread_rt.so.1 libpthread_rt.so

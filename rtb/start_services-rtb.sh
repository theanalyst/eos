#!/usr/bin/env bash
set -e
set -x

image=""
n_fst=6
with_qdb=1

CLT=eos-cli1
cSize=0		    # fst volume size
vol=false
nativeMGM=false
cleanFS=false
while getopts 'mc:i:n:s:vz' flag; do
  case "${flag}" in
    c) CLT=$CLT$OPTARG ;;
    i) image="${OPTARG}" ;;
    m) nativeMGM=true ;;
    n) n_fst="${OPTARG}" ;;
    q) with_qdb=1 ;;
    s) cSize="${OPTARG}" ;;	# fst volume size
    v) vol=true ;;
    z) cleanFS=true;;
    *) echo "Unexpected option ${flag}"; exit 1;;
  esac
done

if [[ $image == "" ]]; then
  echo "Usage: -i <name of the docker image>"
  exit 1
fi

xx=($(hostname -I))
HOSTIP=${xx[0]}
NET=host

#docker network create eoscluster.cern.ch

Pmgm=1097
Pmq=2097
TTTsysConfig=$(mktemp)
TTT1=$(mktemp)
TTT2=$(mktemp)


# docker run -dit -h eos-krb-test.eoscluster.cern.ch --name eos-krb-test --net=$NET --net-alias=eos-krb-test $image
# docker run -dit -h eos-krb-test.eoscluster.cern.ch --name eos-krb-test --net=$NET $image
# docker exec -i eos-krb-test /kdc.sh

# docker run -dit -h eos-mq-test.eoscluster.cern.ch --name eos-mq-test --net=$NET --net-alias=eos-mq-test $image
docker run -dit -h eos-mq.eoscluster.cern.ch -p $HOSTIP:$Pmgm:$Pmgm --name eos-mq --net=$NET $image
docker cp eos-mq:/etc/sysconfig/eos $TTTsysConfig
#sed "s/eos-mq.eoscluster.cern.ch:1097/$(uname -n):$Pmgm/" -i $TTTsysConfig
sed "s/eos-mgm1.eoscluster.cern.ch/$(uname -n)/" -i $TTTsysConfig
sed "s/eos-mq.eoscluster.cern.ch:1097/$(uname -n):$Pmq/" -i $TTTsysConfig
sed "/EOS_INSTANCE_NAME/aexport EOS_FS_FULL_SIZE_IN_GB=1" -i $TTTsysConfig
docker cp $TTTsysConfig eos-mq:/etc/sysconfig/eos
docker cp eos-mq:/etc/xrd.cf.mq $TTT1
sed "s/xrd.port 1097/xrd.port $Pmq/" -i $TTT1
docker cp $TTT1 eos-mq:/etc/xrd.cf.mq

docker exec -i eos-mq /eos_mq_setup.sh

# docker run --privileged -dit -h eos-mgm-test.eoscluster.cern.ch -p $HOSTIP:1094:1094 --name eos-mgm-test --net=$NET --net-alias=eos-mgm-test $image
docker run --privileged -dit  --name eos-mgm1 --net=$NET $image

# TMP_EOS_KEYTAB=$(mktemp)
# docker cp eos-krb-test:/root/eos.keytab $TMP_EOS_KEYTAB
# docker cp $TMP_EOS_KEYTAB eos-mgm-test:/etc/eos.krb5.keytab
# rm -f $TMP_EOS_KEYTAB

docker cp $TTTsysConfig eos-mgm1:/etc/sysconfig/eos
docker cp eos-mgm1:/usr/sbin/service $TTT2
sed "s/VERSION=/exit 0 #V ERSION=/" -i $TTT2
docker cp $TTT2 eos-mgm1:/usr/sbin/service

docker cp /etc/krb5.keytab eos-mgm1:/etc/eos.krb5.keytab
docker cp /etc/krb5.conf eos-mgm1:/etc/krb5.conf


docker cp eos-mgm1:/etc/xrd.cf.mgm $TTT1
sed "s/TEST.EOS/CERN.CH/" -i $TTT1
sed "s/#mgmofs.trace/xrd.trace/" -i $TTT1
sed "s/sec.protbind localhost.localdomain unix sss/sec.protbind localhost.localdomain sss unix/" -i $TTT1
sed "s/sec.protbind localhost unix sss/sec.protbind localhost sss unix/" -i $TTT1
sed "s/<host>/$(uname -n)/" -i $TTT1
set -x
[[ $with_qdb == 1 ]] && { 
    sed "s/libEosNsInMemory.so/libEosNsQuarkdb.so/g" -i $TTT1
    sed "s/eos-qdb.eoscluster.cern.ch/$(uname -n)/g" -i $TTT1

    if false; then 
      v=QDB
	  docker volume inspect $v > /dev/null 2>&1 && {
	      mp=/var/lib/docker/volumes/$v/_data
	      [[ -d $mp ]] && umount $mp || echo $mp not unmounted
          $cleanFS && docker volume rm $v
	  }
	  docker volume inspect $v > /dev/null 2>&1 || \
	      docker volume create --driver local --opt o=sizelimit=10M  --opt type=loop --opt device=tmpfs $v
#	      docker volume create --driver local --opt o=size=10M  --opt type=tmpfs --opt device=tmpfs $v
    else
      v=/var/quarkdb
      [[ -d $v ]] || mkdir $v
      $cleanFS && rm -rf $v/*
    fi
    
	qqq="--volume ${v}:/var/quarkdb"
    docker run $qqq --privileged -dit --name eos-qdb -p $HOSTIP:7777:7777  --net=$NET $image
    $cleanFS || {
        docker cp eos-mgm1:/eos_qdb_setup.sh $TTT2
        sed "s/quarkdb-create/#quarkdb-create/" -i $TTT2
        docker cp $TTT2 eos-qdb:/eos_qdb_setup.sh
    }

    docker exec -i eos-qdb /eos_qdb_setup.sh
}


/sbin/iptables -n -L INPUT|grep ' 1094,' || {
    # set up iptables for connections from outside 
    /sbin/iptables -n -L INPUT|grep 1094,
    /sbin/iptables -I INPUT 5 -p tcp -m multiport --dports 1094,1100,11095,21095,31095,41095,51095,61095 -m state --state NEW -j ACCEPT
}


if $nativeMGM; then
    docker cp eos-mgm1:/eos_mgm_setup.sh /
    docker cp eos-mgm1:/eos_mgm_fs_setup.sh /
    cp $TTT1 /etc/xrd.cf.mgm
    cp -p $TTTsysConfig /etc/sysconfig/eos
    cp -p /etc/krb5.keytab /etc/eos.krb5.keytab
    docker cp eos-mgm1:/etc/eos.keytab /etc/eos.keytab
    chown daemon /etc/eos.krb5.keytab /etc/eos.keytab
    DXM=
    /eos_mgm_setup.sh
else
    docker cp $TTT1 eos-mgm1:/etc/xrd.cf.mgm
    DXM="docker exec -t -i eos-mgm1 "
    docker exec -di eos-mgm1 /eos_mgm_setup.sh
    $DXM sh -c "groupadd -g 1028 c3; useradd -u 17748 -g c3 tobbicke"
    $DXM sh -c "sed -i -e '/^rtb:/ d; $ a $(getent passwd rtb)' /etc/passwd"
fi



sleep 5

$DXM eos space define default

# Start and configure the FSTs in parallel
FAILURE=0
PIDS=""
DATADIR=(/data01 /data01 /data01 /ddd02 /ddd02 /ddd02)
$cleanFS && {
    $vol || rm -rf /data01/fst/* /ddd02/fst/*
}
for (( i=1; i<=$n_fst; i++))
do
    FSTHOSTNAME=eos-fst${i}
    ((Pfst=($i*10000)+1095))
    # docker run --privileged -dit -h $FSTHOSTNAME.eoscluster.cern.ch -p $HOSTIP:$Pfst:$Pfst --name $FSTHOSTNAME --net=$NET --net-alias=$FSTHOSTNAME $image &
    #docker run --volume ${DATADIR[$(($i-1))]}/fst:/home/data --privileged -dit  -p $HOSTIP:$Pfst:$Pfst --name $FSTHOSTNAME --net=$NET $image &
    $vol && {
        v=${DATADIR[$(($i-1))]}
	    v=${v:1}-$i
	    docker volume inspect $v > /dev/null 2>&1 && {
	        mp=/var/lib/docker/volumes/$v/_data
	        [[ -d $mp ]] && umount $mp || echo $mp not unmounted
            $cleanFS && docker volume rm $v
	    }
	    docker volume inspect $v > /dev/null 2>&1 || \
	        docker volume create --driver local --opt o=size=${cSize}G  --opt type=tmpfs --opt device=tmpfs $v
#	        docker volume create --driver local --opt o=sizelimit=${cSize}G  --opt type=loop --opt device=loop $v
	    vvv="--volume ${v}:/home/data"
    }
    $vol || {
        vvv="--volume ${DATADIR[$(($i-1))]}/fst:/home/data"
    }
    #[[ "$cSize" -gt 0 ]] && { sss="--storage-opt size=${cSize}G"; }
    docker run $vvv $sss --privileged -dit  -p $HOSTIP:$Pfst:$Pfst --name $FSTHOSTNAME --net=$NET $image &

    PIDS="${PIDS} $!"
    sleep 0.1
done

for PID in ${PIDS}; do
  wait ${PID} || let "FAILURE=1"
done

if [ "${FAILURE}" == "1" ]; then
  echo "Failed to start one of the fsts";
  exit 1
fi

PIDS=""



for (( i=1; i<=$n_fst; i++))
do
  FSTHOSTNAME=eos-fst${i}
  ((Pfst=($i*10000)+1095))
  docker cp $TTTsysConfig $FSTHOSTNAME:/etc/sysconfig/eos
  docker cp $FSTHOSTNAME:/etc/xrd.cf.fst $TTT1
  sed "s/xrd.port 1095/xrd.port $Pfst/" -i $TTT1
  docker cp $TTT1 $FSTHOSTNAME:/etc/xrd.cf.fst
  docker cp eos-fst${i}:/eos_fst_setup.sh $TTT1
  sed "s/eos-mgm1.eoscluster.cern.ch/$(uname -n)/" -i $TTT1
  sed "s/:1095/:$Pfst/" -i $TTT1
  docker cp $TTT1 $FSTHOSTNAME:/eos_fst_setup.sh
  docker exec -i $FSTHOSTNAME /eos_fst_setup.sh $i &
  PIDS="${PIDS} $!"
  sleep 0.1
done

for PID in ${PIDS}; do
  wait ${PID} || let "FAILURE=1"
done

if [ "${FAILURE}" == "1" ]; then
  echo "Failed to confgure one of the fsts"
  exit 1
fi


$DXM/eos_mgm_fs_setup.sh $n_fst
#docker run --privileged --pid=host -dit -h $CLT.eoscluster.cern.ch --name $CLT --net=eoscluster.cern.ch --net-alias=$CLT $image
docker run --privileged --pid=host -dit -h $CLT.eoscluster.cern.ch --name $CLT --net=$NET $image
#docker exec -i eos-krb-test cat /root/admin1.keytab | docker exec -i eos-client-test bash -c "cat > /root/admin1.keytab"
#docker exec -i eos-client-test kinit -kt /root/admin1.keytab admin1@TEST.EOS
#docker exec -i eos-client-test kvno host/eos-mgm-test.eoscluster.cern.ch

$cleanFS && {
  $DXM eos -b mkdir -p /eos/user
  $DXM eos -b mkdir -p /eos/u1
  $DXM eos -b mkdir -p /eos/u2
  $DXM eos -b mkdir -p /eos/rtb
  $DXM eos -b attr set sys.eval.useracl=1 /eos/rtb
  $DXM eos -b mkdir -p /eos/rtb/tobbicke
  $DXM eos -b chown 17748 /eos/rtb/tobbicke
  $DXM eos -b chmod 1777 /eos/rtb
  $DXM eos -b mkdir -p /eos/t/tobbicke/eosfusetests
  $DXM eos -b chmod -r 1777 /eos/t
  $DXM eos -b attr set sys.forced.layout=replica /eos/rtb
  $DXM eos -b attr set sys.forced.nstripes=2 /eos/rtb
  $DXM eos -b attr set sys.forced.layout=replica /eos/t
  $DXM eos -b attr set sys.forced.nstripes=2 /eos/t
}

$DXM eos -b vid enable krb5
$DXM eos -b vid add gateway rtb-eos-t2 unix
docker cp /etc/krb5.conf $CLT:/etc/krb5.conf
docker exec -i $CLT mkdir -p /eos
true && for n in user u1 u2; do
#true && for n in user; do
    true && docker exec -i $CLT mkdir -p /eos/$n
    false && { 
	docker exec -i $CLT env EOS_MGM_URL=root://$(uname -n) env EOS_FUSE_LOG_PREFIX=u1 eos fuse mount /eos/u1
    }
    docker exec -i $CLT sed -i "s/eos-mgm1.eoscluster.cern.ch/$(uname -n)/" -i /etc/eos/fuse.mount-1.conf
    true && {
	cat >$TTT1 <<EOSFUSECONFIG
{ "name": "$n", "hostport": "rtb-eos-t2:1094", "remotemountdir": "/eos/", "mdzmqtarget": "tcp://rtb-eos-t2:1100",
"auth" : { "shared-mount" : 1, "krb5" : 1 },
"options" : { "mkdir-is-sync" : 1, "create-is-sync" : 1, "debuglevel" : 7 , "debug": 7, "lowleveldebug": 7},
"cache": {  "type": "disk", "size-mb": 10000, "location": "/var/tmp/eosxd-cache/$n/", "journal": "/var/tmp/eosxd-journal/$n/" } }
EOSFUSECONFIG
	docker cp $TTT1 $CLT:/etc/eos/fuse.$n.conf
	docker exec -di $CLT mount -t fuse eosxd -ofsname=$n /eos/$n
    docker exec -di $CLT /bin/bash -c 'mkdir /eos1/; mount -t fuse -ofsname=mount-1 eosxd /eos1/'
    }
done

docker cp $(dirname $0)/fuseTests $CLT:/

rm -rf $TTT1 $TTT2 $TTTsysConfig

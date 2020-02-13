#!/bin/bash -ve

function usage() {
  echo "usage: $(basename $0) --type 'docker'|'k8s <k8s_namespace>' [--only-client]"
  echo "        docker : script runs in a Docker based setup"
  echo "        k8s    : script runs in a Kubernetes setup and requires a namespace argument"
  echo "<k8s_namespace>: must be a DNS-1123 label and must consist of lower case alphanumeric characters or '-', and must start and end with an alphanumeric character"
}

# Forward the given command to the proper executor Docker or Kubernetes. Gets
# as argument a container name and a shell command to be executed
function exec_cmd() {
  if [[ $IS_DOCKER == true ]]; then
    exec_cmd_docker "$@"
  else
    exec_cmd_k8s "$@"
  fi
}

# Execute command in Docker setup where the first argument is the name of the container
# and the rest is the command to be executed
function exec_cmd_docker() {
  set -o xtrace
  docker exec -i $1 /bin/bash -l -c "${@:2}"
  set +o xtrace
}

# Execute command in Kubernetes setup where the first argument is the name of the Pod
# and the rest is the command to be executed
function exec_cmd_k8s() {
  set -o xtrace
  kubectl exec --namespace=$K8S_NAMESPACE $(kubectl get pods --namespace=$K8S_NAMESPACE --no-headers -o custom-columns=":metadata.name" -l app=$1) -- /bin/bash -l -c "${@:2}"
  set +o xtrace
}

# Set up global variables
IS_DOCKER=""
K8S_NAMESPACE=""


if [[ "$1" != "--type" ]]; then
  echo "error: unknown argument \"$1\""
  usage && exit 1
fi

if [[ "$2" == "docker" ]]; then
  IS_DOCKER=true

elif [[ "$2" == "k8s" ]]; then

  IS_DOCKER=false
  # For the Kubernetes setup we also need a namespace argument
  if [[ -z $3 ]]; then
    echo "error: missing Kubernetes namespace argument"
    usage && exit 1
  else
    if [[ $3 =~ ^[a-z0-9]([-a-z0-9]*[a-z0-9])?$ ]]; then
	  K8S_NAMESPACE=$3
    else
      usage && exit 1
    fi
  fi

else
  echo "error: unknown type of executor \"$2\""
  usage
  exit 1
fi


exec_cmd eos-cli1 "git clone https://gitlab.cern.ch/dss/eosclient-tests.git"
exec_cmd eos-cli1 'atd; at now <<< "mkdir /eos1/; mount -t fuse eosxd -ofsname=mount-1 /eos1/; mkdir /eos2/; mount -t fuse eosxd -ofsname=mount-2 /eos2/;"'
exec_cmd eos-cli1 'count=0; while [[ $count -le 10 ]] && ( [[ ! -d /eos1/dockertest/ ]] || [[ ! -d /eos2/dockertest/ ]] ); do echo "Wait for mount... $count"; (( count++ )); sleep 1; done;'
exec_cmd eos-cli1 "su - eos-user -c 'mkdir /eos1/dockertest/fusex_tests/; cd /eos1/dockertest/fusex_tests/; /usr/sbin/fusex-benchmark'" # workaround for docker exec '-u' flag

# @todo(esindril): run "all" tests in schedule mode once these are properly supported
# if [ "$CI_PIPELINE_SOURCE" == "schedule" ];
# then
# 	exec_cmd eos-mgm1 'eos vid add gateway "eos-cli1.eos-cli1.${NAMESPACE}.svc.cluster.local" unix';
# 	exec_cmd eos-cli1 'env EOS_FUSE_NO_ROOT_SQUASH=1 python /eosclient-tests/run.py --workdir="/eos1/dockertest /eos2/dockertest" ci';
# fi
# until then just run the "ci" tests
exec_cmd eos-cli1 "su - eos-user -c 'python /eosclient-tests/run.py --workdir=\"/eos1/dockertest /eos2/dockertest\" ci'"

# Don't execute client tests against the old FUSE client as support is dropped
#if [ "$CI_JOB_NAME" != k8s_ubuntu_system ]; then
#	exec_cmd eos-cli1 'eos fuse mount /eos_fuse; eos fuse mount /eos_fuse2;';
#	exec_cmd eos-cli1 'python /eosclient-tests/run.py --workdir="/eos_fuse/dockertest /eos_fuse2/dockertest" ci';
#fi





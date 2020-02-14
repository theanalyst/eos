#!/bin/bash -ve

source /home/gitlab-runner/coe-cluster-config/st-gitlab-k8s-02/env_st-gitlab-k8s-02.sh # get access configs for the cluster
export K8S_NAMESPACE=$(echo ${CI_JOB_NAME}-${CI_JOB_ID}-${CI_PIPELINE_ID} | tr _ -) # must consist of lower case alphanumeric characters or '-', and must start and end with an alphanumeric character
./eos-on-k8s/collect_logs.sh ${K8S_NAMESPACE} eos-logs/
./eos-on-k8s/delete-all.sh ${K8S_NAMESPACE}
rm -rf eos-on-k8s/
docker images -q | xargs docker rmi || true # remove unused images

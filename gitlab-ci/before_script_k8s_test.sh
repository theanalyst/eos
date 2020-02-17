#!/bin/bash -ve

source /home/gitlab-runner/coe-cluster-config/st-gitlab-k8s-02/env_st-gitlab-k8s-02.sh # get access configs for the cluster
git clone https://gitlab.cern.ch/eos/eos-on-k8s.git
export K8S_NAMESPACE=$(echo ${CI_JOB_NAME}-${CI_JOB_ID}-${CI_PIPELINE_ID} | tr _ -)

export IMAGE_TAG="${BASETAG}${CI_COMMIT_TAG}${CI_PIPELINE_ID}";
export CLI_IMAGE_TAG="${BASETAG}${CLI_BASETAG}${CI_COMMIT_TAG}${CI_PIPELINE_ID}"; # always only one between $CI_COMMIT_TAG $CI_PIPELINE_ID exists

if [[ "$CI_JOB_NAME" =~ "k8s_unit" ]]; then
    ./eos-on-k8s/create-mgm.sh -i ${IMAGE_TAG} -n ${K8S_NAMESPACE} -q;
else
    ./eos-on-k8s/create-all.sh -i ${IMAGE_TAG} -u ${CLI_IMAGE_TAG} -n ${K8S_NAMESPACE} -q;
fi;

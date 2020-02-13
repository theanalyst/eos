#!/bin/bash -ve

. /home/gitlab-runner/coe-cluster-config/st-gitlab-k8s-02/env_st-gitlab-k8s-02.sh # get access configs for the cluster
git clone https://gitlab.cern.ch/eos/eos-on-k8s.git
NAMESPACE=$(echo ${CI_JOB_NAME}-${CI_JOB_ID}-${CI_PIPELINE_ID} | tr _ -)
if [[ -n "$CI_COMMIT_TAG" ]]; then
    IMAGE_TAG="${BASETAG}${CI_COMMIT_TAG}"; CLI_IMAGE_TAG="${BASETAG}${CLI_BASETAG}${CI_COMMIT_TAG}";
else
    IMAGE_TAG="${BASETAG}${CI_PIPELINE_ID}"; CLI_IMAGE_TAG="${BASETAG}${CLI_BASETAG}${CI_PIPELINE_ID}";
fi;
if [[ "$CI_JOB_NAME" =~ "k8s_unit" ]]; then
    ./eos-on-k8s/create-mgm.sh -i ${IMAGE_TAG} -n ${NAMESPACE} -q;
else
    ./eos-on-k8s/create-all.sh -i ${IMAGE_TAG} -u ${CLI_IMAGE_TAG} -n ${NAMESPACE} -q;
fi;

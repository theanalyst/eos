#!/bin/bash -ve

sudo ./eos-docker/scripts/shutdown_services.sh
./eos-docker/scripts/remove_unused_images.sh

# always only one between $CI_COMMIT_TAG $CI_PIPELINE_ID exists
export IMAGE_TAG="eos:${BASETAG}${CI_COMMIT_TAG}${CI_PIPELINE_ID}";
export CLI_IMAGE_TAG="eos:${BASETAG}${CLI_BASETAG}${CI_COMMIT_TAG}${CI_PIPELINE_ID}";

docker pull gitlab-registry.cern.ch/dss/${IMAGE_TAG}
docker pull gitlab-registry.cern.ch/dss/${CLI_IMAGE_TAG}
sudo ./eos-docker/scripts/start_services.sh -q -i gitlab-registry.cern.ch/dss/${IMAGE_TAG} -u gitlab-registry.cern.ch/dss/${CLI_IMAGE_TAG};
#docker pull gitlab-registry.cern.ch/dss/eos:1422185
#docker pull gitlab-registry.cern.ch/dss/eos:ubuntu_bionic_client_1422185
#sudo ./eos-docker/scripts/start_services.sh -q -i gitlab-registry.cern.ch/dss/eos:1422185 -u gitlab-registry.cern.ch/dss/eos:ubuntu_bionic_client_1422185;

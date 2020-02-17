#!/bin/bash -ve

sudo ./eos-docker/scripts/shutdown_services.sh
./eos-docker/scripts/remove_unused_images.sh
docker pull gitlab-registry.cern.ch/dss/${EOS_IMG_NAME}${CI_COMMIT_TAG-$CI_PIPELINE_ID}
#sudo ./eos-docker/scripts/start_services.sh -q -i gitlab-registry.cern.ch/dss/${EOS_IMG_NAME}${CI_COMMIT_TAG-$CI_PIPELINE_ID};
#sudo ./eos-docker/scripts/start_services.sh -q -i gitlab-registry.cern.ch/dss/eos:${CI_COMMIT_TAG-$CI_PIPELINE_ID} -u gitlab-registry.cern.ch/dss/${UBUNTU_EOS_IMG_NAME}${CI_COMMIT_TAG-$CI_PIPELINE_ID};
sudo ./eos-docker/scripts/start_services.sh -q -i gitlab-registry.cern.ch/dss/${EOS_IMG_NAME}${CI_COMMIT_TAG-$CI_PIPELINE_ID} -u gitlab-registry.cern.ch/dss/${CLI_EOS_IMG_NAME}${CI_COMMIT_TAG-$CI_PIPELINE_ID};

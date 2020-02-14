#!/bin/bash -ve

sudo ./eos-docker/scripts/shutdown_services.sh
./eos-docker/scripts/remove_unused_images.sh
#docker pull gitlab-registry.cern.ch/dss/${EOS_IMG_NAME}${CI_COMMIT_TAG-$CI_PIPELINE_ID}
#sudo ./eos-docker/scripts/start_services.sh -q -i gitlab-registry.cern.ch/dss/${EOS_IMG_NAME}${CI_COMMIT_TAG-$CI_PIPELINE_ID};
docker pull gitlab-registry.cern.ch/dss/${EOS_IMG_NAME}1414091
sudo ./eos-docker/scripts/start_services.sh -q -i gitlab-registry.cern.ch/dss/${EOS_IMG_NAME}1414091;

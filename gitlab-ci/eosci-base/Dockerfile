FROM cern/cc7-base

LABEL maintainer="Fabio Luchetti, faluchet@cern.ch, CERN 2020"

ARG MYVAR

RUN env; echo $MYVAR

RUN yum install --nogpg -y gcc-c++ cmake3 make rpm-build which git yum-plugin-priorities tar ccache sl-release-scl

RUN pwd; ls -l; env; \
    cd /builds/dss/eos/; \
    source gitlab-ci/export_branch.sh; \
    git submodule update --init --recursive; \
    mkdir build; cd build/;\
    cmake3 ../ -DPACKAGEONLY=1 ${CMAKE_OPTIONS}; \
    make srpm; \
    cd ../; \
    echo -e '[eos-depend]\nname=EOS dependencies\nbaseurl=http://storage-ci.web.cern.ch/storage-ci/eos/'${BRANCH}'-depend/el-7/x86_64/\ngpgcheck=0\nenabled=1\npriority=2\n' >> /etc/yum.repos.d/eos-depend.repo; \
    yum-builddep --nogpgcheck --setopt="cern*.exclude=xrootd*" -y build/SRPMS/*

ENTRYPOINT /bin/bash
eosfuse SELinux
===============

The eosfuse SELinux policy is built via the following steps:
1. Write the new `eosfuse.te` file -- Remember to increase the version!
2. Run `checkmodule -M -m -o eosfuse.mod eosfuse.te`
3. Run `semodule_package -o eosfuse.pp -m eosfuse.mod`
4. Test the policy: `semodule -i eosfuse.pp`
5. Version control the `eosfuse.te` file from this directory.

Building eosfuse
----------------

The build process provides the `make eosfuse-selinux`,
which will create and install the necessary `eosfuse.pp` module.


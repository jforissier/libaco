This is a simplified fork of [libaco](https://github.com/hnes/libaco/), adding
support for Aarch64 and removing what is not strictly necessary for use in
U-Boot. The idea is to introduce some kind of cooperative multi-tasking in the
core U-Boot in order to reduce boot time. I chose this libaco framework for its
simplicity and because it is very lightweight. I plan to use it as a RFC.


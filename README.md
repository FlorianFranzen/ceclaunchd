ceclaunchd
==========

A libcec based daemon that allows you to launch binaries and scripts via HDMI CEC commands.


Build
-----

Just run make.


Install
-------

The deamon comes with it own init script.
Put this script (ceclaunchd) into the /etc/init.d/ folder and make sure the previously build binary is in /usr/local/bin/.

Then you can run the daemon with '/etc/init.d/ceclaunchd start' and stop it with '/etc/init.d/ceclaunchd stop' 


Config
------

The example config file should be pretty self explanatory. Each button mapping needs it own section. 

Make sure that the conf file is located at /etc/ceclaunch.conf and is accessible.
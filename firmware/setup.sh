source ../../../devel/setup.bash
rm -rf libraries/ros_lib
rosrun rosserial_arduino make_libraries.py libraries

arduino --upload --board VersaVIS:samd:VersaVIS --port /dev/versavis versavis_rtc/versavis_rtc.ino

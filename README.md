# Thermostat-controller
A friend's home has two heaters with separate wall-mounted control panels. This device measures the room temperature independently and uses a PID controller to achieve the desired temperature. A wireless RF link is used to command sub-modules to electronically "push" the buttons on the old remote controllers via some optoisolators.

Thermostat_temperature.ino is the master. It reads a thermometer, and figures out the optimal setting for the heaters. It has a knob and a 2x16 LCD and some LEDs for interface. It uses some custom digital characters to show in interface in Japanese. The master pushes buttons on the heater control panel as well as sends the desired settings for the slave via RF.

thermostat_receive.ino is the slave who receives commands and pushes the heater controls.

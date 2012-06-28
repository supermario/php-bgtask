PHP Background Task
===================

A simple PHP interface for executing unattended (non-blocking) system calls, monitoring their output and exit status.

Usage
-----

 See demo.php in the demo folder for usage example and basic documentation

Security
--------

In it's plain form this class allows a developer to run any desired command via PHP's exec()

Allowing any user input to this class is allowing direct execution of user controlled input on your server

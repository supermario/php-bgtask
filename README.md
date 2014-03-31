PHP Background Task
===================

A simple PHP interface for executing unattended (non-blocking) system calls, monitoring their output and exit status.

Usage
-----

See demo.php in the demo folder for usage example and basic documentation

Requirements
------------

Your environment must have one of the [shell_exec](http://php.net/manual/en/function.shell-exec.php), [exec](http://php.net/manual/en/function.exec.php) or [system](http://php.net/manual/en/function.system.php) PHP functions available in order for this class to work.

If you're unsure, run demo.php for a quick test.

Security
--------

In it's plain form this class allows a developer to run any desired command via PHP's exec()

Allowing any user input to this class is allowing direct execution of user controlled input on your server

Compatibility
-------------

bgTask should run on any *nix OS, and has been tested on OS X, and Debian/Ubuntu.

Windows is currently unsupported.

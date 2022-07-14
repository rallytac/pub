# Rallypoint Monitor (rpm)
rpm is a little tool written in Python to monitor the status file produced by the Rallypoint and present that status in an easy-to-read format.

## Dependencies
rpm needs the `colorama` Python module installed.  Typically you install it as follows:
```shell
$ sudo pip install colorama
```

>NOTE: Its fine if you don't install colorama - `rpm` will still work but won't be as pretty.

## Running rpm
To run rpm, simply fire it up using Python and pass the name of the status file to be monitored.  For example:
```shell
$ python rpm.py /tmp/rp01_status.json
```

You can also specify a change to the polling interval of 5 seconds with the optional `-i:` command-line parameter.  For example:
```shell
$ python rpm.py /tmp/rp01_status.json -i:30
```

... will change the polling interval to 30 seconds.

You should see output like this:
```shell
-----------------------------------------------------------------------------------------------------------------------------------------------
Rallypoint Monitor v0.1
Copyright (c) 2021 Rally Tactical Systems, Inc.

Monitoring /tmp/rp01_status.json at 5 second intervals
Last check at 2021/12/05 19:05:43 █ uptime 11 seconds █ updated 7 seconds ago
---------------------------------------------------------------------------------------------
### Colors
If you have `colorama` installed, `rpm` will color-code the display as follows:

#### Header Section
- **GRAY**: Status file is (pretty) recent.
- **RED**: If status file has not been updated by Rallypoint for at least two polling intervals.  For example: assuming a polling interval of 5 seconds (the default); if the file has not been updated for 10 seconds, the header section will display in red.  If the polling interval is set to `30` using the `-i:` command-line parameter, the header section will display in **RED** after a minute has passed wityhout an update.

#### Links Detail Rows
...TODO...
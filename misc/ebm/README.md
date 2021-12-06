# Engage Bridge Monitor (ebm)
ebm is a little tool written in Python to monitor the status file produced by the Engage Bridge Service (engagebridged) and present that status in an easy-to-read format.

## Dependencies
ebm needs the `colorama` Python module installed.  Typically you install it as follows:
```shell
$ sudo pip install colorama
```

>NOTE: Its fine if you don't install colorama - `ebm` will still work but won't be as pretty.

## Running ebm
To run ebm, simply fire it up using Python and pass the name of the status file to be monitored.  For example:
```shell
$ python ebm.py /tmp/bridgeserver01_status.json
```

You can also specify a change to the polling interval of 5 seconds with the optional `-i:` command-line parameter.  For example:
```shell
$ python ebm.py /tmp/bridgeserver01_status.json -i:30
```

... will change the pollig interval to 30 seconds.

You should see output like this:
```shell
-----------------------------------------------------------------------------------------------------------------------------------------------
Engage Bridge Service Monitor v0.1
Copyright (c) 2021 Rally Tactical Systems, Inc.

Monitoring /tmp/bridgeserver01_status.json at 5 second intervals
Last check at 2021/12/05 19:05:43 █ uptime 11 seconds █ updated 7 seconds ago
-----------------------------------------------------------------------------------------------------------------------------------------------
                                                                                                    ------ Packets ------ ------- Bytes -------
                             Bridge ID      State                               Group ID      State         RX         TX         RX         TX
-------------------------------------- ---------- -------------------------------------- ---------- ---------- ---------- ---------- ----------
                                   1+2         OK {ac197b82-0f45-86e8-bf53-02ceea2e977c}         OK         34         17       5848       2924
                                                  {f38a3c6f-201c-648e-4dbe-1e8565afe0c5}         OK         17         34       2924       5848

                                   2+3         OK {f38a3c6f-201c-648e-4dbe-1e8565afe0c5}         OK       7215       9166     519480      659952
                                                  {d54e51d0-1cc1-e130-f955-35b8f763cf9b}         OK       9166       7215     659952      519480
```                                                                                     
Each bridge entry will consist of 2 or more rows with the first row containing the bridge ID and it's state.  On that same row, as well as subsequent rows for that bridge, you should also see a group ID and the state of that group.

### Colors
If you have `colorama` installed, `ebm` will color-code the display as follows:

#### Header Section
- **GRAY**: Status file is (pretty) recent.
- **RED**: If status file has not been updated by Engage Bridge Service for at least two polling intervals.  For example: assuming a polling interval of 5 seconds (the default); if the file has not been updated for 10 seconds, the header section will display in red.  If the polling interval is set to `30` using the `-i:` command-line parameter, the header section will display in **RED** after a minute has passed wityhout an update.

#### Bridge Group Detail Rows
- **GRAY**: The group is connected and all seems well.
- **YELLOW**: Group connection attempt is in progress.
- **RED**: The group has experienced an error.

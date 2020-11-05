# Engage Bridge Monitor (ebm)
ebm is a little tool written in Python to monitor the status file produced by the Engage Bridge Service (engagebridged) and present that status in an easy-to-read format.

## Dependencies
ebm needs the `colorama` Python module installed.  Typically you install it as follows:
```shell
$ sudo pip install colorama
```

## Running ebm
To run ebm, simply fire it up using Python and pass the name of the status file to be monitored.  For example:
```shell
$ python ebm.py /tmp/bridgeserver01_status.json
```

You should see output like this:
```shell
---------------------------------------------------------------------------------------------------
Engage Bridge Service Monitor v0.1
Copyright (c) 2020 Rally Tactical Systems, Inc.

Monitoring /tmp/bridgeserver01_status.json at 5 second intervals
Last check at 2020/07/29 08:44:06, uptime 11 seconds
---------------------------------------------------------------------------------------------------
                             Bridge ID      State                               Group ID      State
-------------------------------------- ---------- -------------------------------------- ----------
                                   1+2         OK {ac197b82-0f45-86e8-bf53-02ceea2e977c}         OK
                                                  {f38a3c6f-201c-648e-4dbe-1e8565afe0c5}         OK

                                   2+3         OK {f38a3c6f-201c-648e-4dbe-1e8565afe0c5}         OK
                                                  {d54e51d0-1cc1-e130-f955-35b8f763cf9b}         OK
```

Each bridge entry will consist of 2 or more rows with the first row containing the bridge ID and it's state.  On that same row, as well as subsequent rows for that bridge, you should also see a group ID and the state of that group.
# Engage Activity Recorder Monitor (earm)
earm is a little tool written in Python to monitor the status file produced by the Engage Acvtivity Recorder Service (eard) and present that status in an easy-to-read format.

## Dependencies
earm needs the `colorama` Python module installed.  Typically you install it as follows:
```shell
$ sudo pip install colorama
```

## Running earm
To run earm, simply fire it up using Python and pass the name of the status file to be monitored.  For example:
```shell
$ python earm.py /tmp/recserver01_status.json
```

You should see output like this:
```shell
---------------------------------------------------------------------------------------------
Engage Activity Recorder Service Monitor v0.1
Copyright (c) 2020 Rally Tactical Systems, Inc.

Monitoring /private/tmp/earserver01_status.json at 5 second intervals
Last check at 2020/12/17 23:27:38, uptime 10 seconds
---------------------------------------------------------------------------------------------
                              Group ID                                   Name           State
-------------------------------------- -------------------------------------- ---------------
{0e1a2b98-900a-8bdd-bcd5-49f6b043c679} Mission Control                                     OK
{143f2f9c-6279-a3c3-a9a4-bd991138fa07} Group 5                                             OK
{1c4029da-e80a-4d81-ce71-b78d9e91ef31} Group 1                                             OK
{3b717bd8-5591-0808-e5f5-63f3f111912d} Group 14                                            OK
{5626d17d-da06-4c22-652a-d9f6f2aa77bb} Group 4                                             OK
{580cc68a-de09-2074-bf49-18105f5c98a3} Group 3                                             OK
{5d885b6b-8f9e-2da4-0a57-5fddb6ab604d} Group 15                                            OK
{5e02617f-5e6a-a115-4a51-61afde4e834b} Group 13                                            OK
{67600e7e-e612-0425-1dd1-114b3067dacb} Group 9                                             OK
{6b4b2cc0-3525-d2e9-aa90-d0b9d5985fab} Group 16                                            OK
{7e3ed70f-9bf6-ae72-6351-5a6380c8ba23} Group 10                                            OK
{7f7967d4-9607-0c0b-784e-9eabee3aab11} Group 12                                            OK
{c01ed578-16f3-733e-7b25-c8b3e4aa53f8} Group 8                                             OK
{d4dc79ed-a983-ab72-b156-e4a55bda1d74} Group 11                                            OK
{ded9ced9-8ead-bc33-48dc-1a099163c1b1} Group 2                                             OK
{f16d04e3-04fe-4463-2bac-156c6cd27008} Group 6                                             OK
{fcb787c2-ed2c-fe9c-c029-f6db366ac4f6} Group 7                                             OK
```

Each line shows the ID of the group, the name, and state.
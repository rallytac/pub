# Engage Mission Packer/Unpacker (empu)
empu is a little tool written in Python to pack and unpack mission files for Engage-based applications.

## Dependencies
ebm needs Python version or higher installed module installed.  Make sure you have it on your machine.

## Running empu
To run empu, simply fire it up using Python and pass the operation you want (`pack` or `unpack`) and the
name of the file.  For example, packing `mymission.json` results in the following:

```shell
$ python empu.py pack mymission.json

af)AAAAAXLgw>YHlC*.4T?1}JGD*~)R`ymea#II8Dv?NNz^Z:A*CDS_0*|uT;::<%$=F*L.Nc[Gv:{|`TJ]|mul[EA=fD{gL.PQo$+(TzzZNgMeEa@mCe09fo[EPTwRaGL,kyJ+1eREH:)B=`g;;?=O~{>WSmC@@o[,tt?cUD6[tZV3RDb+0suL`TVM)bN%c`C/6D7dz5rH!ZrIgi;F?UzHtBC`*=pfSsQZ0*K@{Lah*rIhodDNVv><|^33X9[x]K=V|K|=@3jAh.dI9$OVIu?!H}T]*H9DYJD&.kMjs(ro92Ep(qqP_<V4)7Ow3i[J~s4x^"@s(mH=#)+=|l@k@2(/%2qYYAAA
```

You can/should redirect that packed output to a file, as follows:
```shell
$ python empu.py pack mymission.json > mymission.mission
```

To go the other way, we'll use the `unpack` operation and, of course, provide the name of the packed file.  For example.

```shell
$ python empu.py unpack mymission.mission

{
  "id": "{9282a495-da1b-42d4-985f-fb9ee587cfa1}",
  "name": "My Mission",
  "groups": [
    {
      "id": "{d0518233-9226-417a-bf23-5c50511eb3fd}",
      "type": 1,
      "name": "Engage Group 1",
      "rx": {
        "address": "239.42.42.42",
        "port": 27000
      },
      "tx": {
        "address": "239.42.42.42",
        "port": 27000
      },
      "txAudio": {
        "encoder": 25,
        "framingMs": 20,
        "maxTxSecs": 30
      },
      "txOptions": {
        "priority": 4,
        "ttl": 42
      }
    }
  ]
}
```


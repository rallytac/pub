# Network Impairment on Linux

Very often we need to simulate different network conditions in order to gauge how our software under adverse conditions.  While there's plenty of commercial and free tools available to do this, we at RTS like to get our hands dirty.  So we built a little script (``impair.sh``) that runs on Linux and impairs traffic on a network interface.

These impairments are grouped into various "profiles" that we've developed over years of working with commercial and government customers.  Feel free to add or modify profiles as you see fit.

## How it works
Impairments work by using the Linux ``tc`` (Traffic Control) tool that "shapes" traffic on a particular network interface. Once the impairment is in place, it will remain until you restart the computer or run ``impair.sh`` with the `none` parameter.

(You can find out more about this stuff at this [HOWTO](https://tldp.org/HOWTO/Traffic-Control-HOWTO/intro.html).)

## Profiles
The profiles offered by ``impair.sh`` are currently as follows:

|Name|Description|Delay|Delay Variation|Loss %|Duplication %|
|-|-|-|-|-|-|
|none|No impairments|0|0|0|0|
|lan|Local Area Network|5|5|0.5|0.1|
|wifi|WiFi|20|10|1|0.5|
|wan|Wide Area Network|80|20|1.2|0.8|
|isp|Internet Service Provider|120|30|1.5|0.9|
|3g|3G Cellular|300|50|2.5|1.0|
|lte|LTE Cellular|150|25|0.9|0.5|
|sat|Satellite|900|100|5.0|0.8|
|cecom1|CECOM Standard|750|80|12.0|0.1|
|cecom2|CECOM 2|1500|160|25.0|0.5|
|awful|Truly Awful|3000|300|30.0|3.0|

Most of the above are fairly self-explanatory with the exception of `cecom1` and `cecom2`.  These are profiles we developed in conjunction with the US Army's [Communications Electronics Command](https://www.army.mil/cecom) to simulate the kinds of environments that could be encountered on a typical military enterprise network (`cecom1`) as well as tactical battlefield field environments (`cecom2`).

>Please note that these numbers do not necessarily reflect the exact network conditions that may be encountered in the military or government space as that information could be classified.

## Using
To impair a network interface, simply invoke ``impair.sh`` with 2 parameters.  The first is the name of the network interface such as `ens33`; the second is the impairment profile such as `wifi`.

For example:  

To impair `ens33` with a `wifi` profile:
```shell
$ ./impair.sh ens33 wifi
```

To remove all impairments on `ens33`:
```shell
$ ./impair.sh ens33 none
```

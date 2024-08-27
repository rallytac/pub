# Engage-Cmd Automated Transmitter
This little utility is used to generate voice traffic on an Engage-based system using our `engage-cmd` tool.  The primary goal is to created a a scalable, automated test environment to simulate traffic on your network.

## What'll I Need?
In addition to the files in this directory, you'll also need `engage-cmd` for your platform - Linux/Windows/Mac.  Cruise over to https://hq.rallytac.com/builds and grab the version for you platform.

Once you have that binary, place it along with the files from this directory in a directory of their own and open a terminal window.

## How Do I Run It?
To produce traffic ...:

- over multicast:
```shell
$ ./ectx P mi-16
```
- via a Rallypoint
```shell
$ ./ectx P mi-16 rp
```

To receive traffic:
- over multicast:
```shell
$ ./ectx C mi-16
```
- via a Rallypoint
```shell
$ ./ectx C mi-16 rp
```

What we've done here is invoke the `ectx` script telling it either to `P`roduce or `C`onsume (the first parameter).  Next, we've told it to use a mission file named `mi-16` (it's actually `mi-16.json` but the script will append that for you).  Finally, if we want to go through a Rallypoint, we provide it with the file named `rp` (in actuality `rp.json`).

These parameters are passed to `engage-cmd` along with details of the Lua script named `ectx.lua`. (Lua is `engage-cmd`'s scripting language).  This Lua script is actually the thing driving production of audio.  On the consumer side, there's no script.

That's pretty much it.  Modify the mission file as you please, as well as the RP file as needed.  And certainly feel free to modify both the shell script (`ectx`) and the Lua script (`ectx.lua`) to fit your needs.

>By the way.  If you need help invoking `ectx`, just run it without parameters to get some (hopefully) helpful information.

Happy Hacking!


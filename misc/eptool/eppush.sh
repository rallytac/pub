#!/bin/bash
adb push engage-sd7.ep${1} /sdcard/Download/engage-sd7.ep
adb shell am broadcast -a "com.rallytac.provisioning.UPDATED"

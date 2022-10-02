#
# Rename EBS group
# Copyright (c) 2022 Rally Tactical Systems, Inc.
#

import argparse
from ast import Or
import json

HIDDEN_WORK_AREA = '__Hidden__'

parser = argparse.ArgumentParser(description='')
parser.add_argument('-ebscfg', required=True, type=str, help='Path to the Engage Bridge Service configuration file')
parser.add_argument('-hide', nargs='+', help='Group IDs to hide')
parser.add_argument('-reveal', nargs='+', help='Group IDs to reveal')
hara = parser.add_mutually_exclusive_group()
hara.add_argument('-hideall', action='store_true', help='Hide all groups')
hara.add_argument('-revealall', action='store_true', help='Reveal all groups')
args = parser.parse_args()

if __name__ == "__main__":
    if((args.hideall or args.revealall) and (args.hide != None or args.reveal != None)):
        print('cannot -hideall or -revealall along with -hide or -reveal')
        exit(1)

    with open(args.ebscfg) as coreFile:
        coreCfg = json.load(coreFile)

    with open(coreCfg['bridgingConfigurationFileName']) as bridgeFile:
        bridgeConfig = json.load(bridgeFile)

    if not HIDDEN_WORK_AREA in bridgeConfig:
        bridgeConfig[HIDDEN_WORK_AREA] = []

    ok = False

    if args.hideall or args.hide != None:
        if args.hideall:
            while len(bridgeConfig['groups']) > 0:
                for group in bridgeConfig['groups']:
                    bridgeConfig[HIDDEN_WORK_AREA].append(group)
                    bridgeConfig['groups'].remove(group)
                    ok = True
                    break;

        else:
            for gid in args.hide:
                for group in bridgeConfig['groups']:
                    if group['id'] == gid:
                        bridgeConfig[HIDDEN_WORK_AREA].append(group)
                        bridgeConfig['groups'].remove(group)
                        ok = True
                        break

    if args.revealall or args.reveal != None:
        if args.revealall:
            while len(bridgeConfig[HIDDEN_WORK_AREA]) > 0:
                for group in bridgeConfig[HIDDEN_WORK_AREA]:
                    bridgeConfig['groups'].append(group)
                    bridgeConfig[HIDDEN_WORK_AREA].remove(group)
                    ok = True
                    break
                
        else:
            for gid in args.reveal:
                for group in bridgeConfig[HIDDEN_WORK_AREA]:
                    if group['id'] == gid:
                        bridgeConfig['groups'].append(group)
                        bridgeConfig[HIDDEN_WORK_AREA].remove(group)
                        ok = True
                        break

    if ok:
        with open(coreCfg['bridgingConfigurationFileName'], "w") as outFile:
                    outFile.write(json.dumps(bridgeConfig, indent=4))
                    outFile.flush()


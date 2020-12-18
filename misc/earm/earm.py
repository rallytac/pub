#
# Engage Activity Recorder Monitor
# Copyright (c) 2020 Rally Tactical Systems, Inc.
#

import json
import time
import colorama
import datetime
import sys

appVersion = '0.1'
statusFile = ''
interval = 5

colorama.init()
pos = lambda y, x: colorama.Cursor.POS(x, y)

# --------------------------------------------------------------------------
def loadInput():
    with open(statusFile) as f:
        return json.load(f)

# --------------------------------------------------------------------------
def groupState(s):
    switcher = {
        -1  : "Error",
        0   : "None",
        1   : "OK",
        2   : "Creating",
        3   : "Joining",
        4   : "Left",
        5   : "Disconnected",
        6   : "Deleted"
    }

    return switcher.get(s, "Unknown")

# --------------------------------------------------------------------------
def setCursor(r, c):
    print('%s' % pos(r, c))

# --------------------------------------------------------------------------
def clearScreen():
    print(colorama.ansi.clear_screen())
    setCursor(0, 0)

# --------------------------------------------------------------------------
def printHeadline(db):
    now = datetime.datetime.now()
    print('---------------------------------------------------------------------------------------------')
    print('Engage Activity Recorder Service Monitor v%s' % (appVersion))
    print('Copyright (c) 2020 Rally Tactical Systems, Inc.')
    print('')
    print('Monitoring %s at %d second intervals' % (statusFile, interval))
    print('Last check at %s, uptime %d seconds' % (now.strftime("%Y/%m/%d %H:%M:%S"), db['uptime']))
    print('---------------------------------------------------------------------------------------------')
    print('%38s %38s %15s' % ('Group ID', 'Name', 'State'))
    print('-------------------------------------- -------------------------------------- ---------------')

# --------------------------------------------------------------------------
def getGroup(db, id):
    for groupInfo in db['groups']['detail']:
        if groupInfo['id'] == id:
            return groupInfo

    return None

# --------------------------------------------------------------------------
def printGroups(db):
    groups = db['groups']
    
    for groupDetail in db['groups']['detail']:
        print('%38s %-38s %15s' % (groupDetail['id'], groupDetail['name'], groupState(groupDetail['state'])))        
        #print(' ')

# --------------------------------------------------------------------------
def main():
    while 1:
        db = loadInput()        
        clearScreen()
        printHeadline(db)
        
        if len(db['groups']) > 0:
            printGroups(db)            
        else:
            print('No groups')

        time.sleep(interval)

    colorama.deinit()

# --------------------------------------------------------------------------
if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('usage: python eearm.py <name_of_recorder_service_status_file>')
        exit()
    
    statusFile = sys.argv[1]
    main()

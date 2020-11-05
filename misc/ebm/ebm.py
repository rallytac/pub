#
# Engage Bridge Monitor
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
def bridgeState(s):
    switcher = {
        -1  : "Error",
        0   : "None",
        1   : "OK",
        2   : "Creating",
        3   : "Deleted"
    }

    return switcher.get(s, "Unknown")

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
    print('---------------------------------------------------------------------------------------------------')
    print('Engage Bridge Service Monitor v%s' % (appVersion))
    print('Copyright (c) 2020 Rally Tactical Systems, Inc.')
    print('')
    print('Monitoring %s at %d second intervals' % (statusFile, interval))
    print('Last check at %s, uptime %d seconds' % (now.strftime("%Y/%m/%d %H:%M:%S"), db['uptime']))
    print('---------------------------------------------------------------------------------------------------')
    print('%38s %10s %38s %10s' % ('Bridge ID', 'State', 'Group ID', 'State'))
    print('-------------------------------------- ---------- -------------------------------------- ----------')

# --------------------------------------------------------------------------
def getGroup(db, id):
    for groupInfo in db['groups']['detail']:
        if groupInfo['id'] == id:
            return groupInfo

    return None

# --------------------------------------------------------------------------
def printBridges(db):
    bridges = db['bridges']
    
    for bridgeDetail in db['bridges']['detail']:
        firstLine = 1

        for groupId in bridgeDetail['groups']:
            groupInfo = getGroup(db, groupId)
            if firstLine:
                print('%38s %10s %38s %10s' % (bridgeDetail['id'], bridgeState(bridgeDetail['state']), groupInfo['id'], groupState(groupInfo['state'])))
            else:
                print('%49s %38s %10s' % (' ', groupInfo['id'], groupState(groupInfo['state'])))

            firstLine = 0
        
        print(' ')

# --------------------------------------------------------------------------
def main():
    while 1:
        db = loadInput()        
        clearScreen()
        printHeadline(db)
        
        if len(db['bridges']) > 0:
            printBridges(db)            
        else:
            print('No bridges')

        time.sleep(interval)

    colorama.deinit()

# --------------------------------------------------------------------------
if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('usage: python ebm.py <name_of_bridge_service_status_file>')
        exit()
    
    statusFile = sys.argv[1]
    main()

#
# Engage Bridge Monitor
# Copyright (c) 2020 Rally Tactical Systems, Inc.
#

import json
import time
import datetime
import sys
import os

try:
    import colorama
    haveColorama = True
except ImportError as e:
    haveColorama = False
    pass

appVersion = '0.1'
statusFile = ''
interval = 5

if haveColorama:
    colorama.init()
    pos = lambda y, x: colorama.Cursor.POS(x, y)
    
# --------------------------------------------------------------------------
def colorNone():
    if haveColorama:
        return colorama.Style.RESET_ALL
    else:
        return ''

# --------------------------------------------------------------------------
def colorError():
    if haveColorama:
        return colorama.Fore.RED
    else:
        return ''

# --------------------------------------------------------------------------
def colorWarning():
    if haveColorama:
        return colorama.Fore.YELLOW
    else:
        return ''

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
    if haveColorama:
        print('%s' % pos(r, c))

# --------------------------------------------------------------------------
def clearScreen():
    if haveColorama:
        print(colorama.ansi.clear_screen())
        setCursor(0, 0)
    else:
        if os.name == 'nt':
            os.system('cls')
        else:
            os.system('clear')

# --------------------------------------------------------------------------
def timeDesc(seconds):
    d, s = divmod(seconds, 86400)
    h, s = divmod(s, 3600)
    m, s = divmod(s, 60)

    rc = ''

    if(d > 0):
        rc = str(d) + ' days'

    if(h > 0 or len(rc) > 0):
        if(len(rc) > 0):
            rc += ', '

        rc += '{0:d} hours'.format(h)

    if(m > 0 or len(rc) > 0):
        if(len(rc) > 0):
            rc += ', '

        rc += '{0:d} minutes'.format(m)

    if(len(rc) > 0):
        rc += ', '

    rc += '{0:d} seconds'.format(s)
    
    return rc

# --------------------------------------------------------------------------
def printHeadline(db):
    tsdelta = time.time() - db['ts']
    if tsdelta > (interval * 2):
        clr = colorError()
        msg = '(*POSSIBLY OFFLINE*)'
    else:
        clr = colorNone()
        msg = ''

    now = datetime.datetime.now()

    print(clr, end = '')
    print('-------------------------------------------------------------------------------------------------------------------------')
    print('Engage Bridge Service Monitor v%s' % (appVersion))
    print('Copyright (c) 2020 Rally Tactical Systems, Inc.')
    print('')
    print('Monitoring %s at %d second intervals' % (statusFile, interval))
    print('Last check at %s | uptime %s | updated %d seconds ago %s' % (now.strftime("%Y/%m/%d %H:%M:%S"), timeDesc(db['uptime']), tsdelta, msg))
    print('-------------------------------------------------------------------------------------------------------------------------')
    print('                                                                                                    ------ Packets ------')
    print('%38s %10s %38s %10s%10s %10s' % ('Bridge ID', 'State', 'Group ID', 'State', "RX", "TX"))
    print('-------------------------------------- ---------- -------------------------------------- ---------- ---------- ----------')
    print(colorNone(), end = '')

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

            if bridgeDetail['state'] == 1 and groupInfo['state'] == 1:
                clr = colorNone()
            else:
                if bridgeDetail['state'] == -1 or groupInfo['state'] == -1:
                    clr = colorError()
                else:
                    clr = colorWarning()

            print(clr, end = '')

            if firstLine:
                print('%38s %10s %38s %10s %10s %10s' % (bridgeDetail['id'], bridgeState(bridgeDetail['state']), groupInfo['name'], groupState(groupInfo['state']), groupInfo["rxTraffic"]["packets"], groupInfo["txTraffic"]["packets"]))
            else:
                print('%49s %38s %10s %10s %10s' % (' ', groupInfo['name'], groupState(groupInfo['state']), groupInfo["rxTraffic"]["packets"], groupInfo["txTraffic"]["packets"]))

            print(colorNone(), end = '')

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

    if haveColorama:
        colorama.deinit()

# --------------------------------------------------------------------------
if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('usage: python ebm.py <name_of_bridge_service_status_file>')
        exit()
    
    statusFile = sys.argv[1]
    main()
    #print(timeDesc(23000))


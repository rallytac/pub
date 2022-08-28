#
# Engage Activity Recorder Monitor
# Copyright (c) 2020 Rally Tactical Systems, Inc.
#

import json
import time
import colorama
import datetime
import sys

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
        if d == 1:
            rc = str(d) + ' day'
        else:
            rc = str(d) + ' days'

    if(h > 0 or len(rc) > 0):
        if(len(rc) > 0):
            rc += ', '

        if h == 1:
            rc += '{0:d} hour'.format(h)
        else:
            rc += '{0:d} hours'.format(h)

    if(m > 0 or len(rc) > 0):
        if(len(rc) > 0):
            rc += ', '

        if m == 1:
            rc += '{0:d} minute'.format(m)
        else:
            rc += '{0:d} minutes'.format(m)

    if(len(rc) > 0):
        rc += ', '

    if s == 1:
        rc += '{0:d} second'.format(s)
    else:
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
    print('---------------------------------------------------------------------------------------------')
    print('Engage Activity Recorder Service Monitor v%s' % (appVersion))
    print('Copyright (c) 2020 Rally Tactical Systems, Inc.')
    print('')
    print('Monitoring %s at %d second intervals' % (statusFile, interval))
    print('Last check at %s | uptime %s | updated %d seconds ago %s' % (now.strftime("%Y/%m/%d %H:%M:%S"), timeDesc(db['uptime']), tsdelta, msg))
    print('---------------------------------------------------------------------------------------------')
    print('%38s %38s %15s' % ('Group ID', 'Name', 'State'))
    print('-------------------------------------- -------------------------------------- ---------------')
    print(colorNone(), end = '')

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
        if groupInfo['state'] == 1:
            clr = colorNone()
        else:
            clr = colorWarning()

        print(clr, end = '')
        print('%38s %-38s %15s' % (groupDetail['id'], groupDetail['name'], groupState(groupDetail['state'])))
        print(colorNone(), end = '')
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

    if haveColorama:
        colorama.deinit()

# --------------------------------------------------------------------------
if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('usage: python eearm.py <name_of_recorder_service_status_file>')
        exit()
    
    statusFile = sys.argv[1]
    main()

#
# Rallypoint Monitor
# Copyright (c) 2020 Rally Tactical Systems, Inc.
#

from __future__ import print_function
import json
import time
import datetime
import sys
import os
import signal

try:
    import colorama
    haveColorama = True
except ImportError as e:
    haveColorama = False
    pass

try:
    headerSepChar = chr(9608)
except:    
    headerSepChar = '*'

appVersion = '0.2'
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
def peerType(s):
    switcher = {
        1   : 'Core',
        2   : 'Leaf'
    }

    return switcher.get(s, 'Unknown')


# --------------------------------------------------------------------------
def peerState(s):
    switcher = {
        0   : 'NotLinked',
        1   : 'InProgress',
        2   : 'LinkedOutbound',
        3   : 'LinkedInbound'
    }

    return switcher.get(s, 'Unknown')


# --------------------------------------------------------------------------
def setColor(c):
    if haveColorama:
        print(c, end = '')


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
    tsdelta = int(time.time() - db['ts'])
    if tsdelta > (interval * 2):
        clr = colorError()
        msg = '  (*POSSIBLY OFFLINE*)'
    else:
        clr = colorNone()
        msg = ''

    now = datetime.datetime.now()

    print(clr, end = '')
    print('-----------------------------------------------------------------------------------------------------------------------------------------------')
    print('Rallypoint Monitor v%s' % (appVersion))
    print('Copyright (c) 2022 Rally Tactical Systems, Inc.')
    print('')
    print('Monitoring %s at %d second intervals' % (statusFile, interval))
    print('Last check at %s %s uptime %s %s updated %s ago,' % (now.strftime('%Y/%m/%d %H:%M:%S'), headerSepChar, timeDesc(db['uptime']), headerSepChar, timeDesc(tsdelta)))
    print('')
    print('Basics-----------------------------------------------------------------------------------------------------------------------------------------')
    print('System-Wide CPU: %7s%% %s Clients: %6s %s Peers: %6s %s Streams: %6s %s Paths: %6s %s RX: %10s packets %s TX: %10s packets' % (db['systemCpuLoad'], headerSepChar, db['links']['clients']['count'], headerSepChar, db['links']['peers']['count'], headerSepChar, db['routing']['streams'], headerSepChar, db['routing']['paths'], headerSepChar, db['rx']['packets'], headerSepChar, db['tx']['packets']))

    print('')
    print('Peers------------------------------------------------------------------------------------------------------------------------------------------')
    print('%30s %30s %5s %15s' % ('ID', 'Address', 'Type', 'State'))
    print('-----------------------------------------------------------------------------------------------------------------------------------------------')
    for peer in db['links']['peers']['list']:
        print('%30s %30s %5s %15s' % (peer['id'], peer.get('address', '?'), peerType(peer['type']), peerState(peer['state'])))

    print(colorNone(), end = '')


# --------------------------------------------------------------------------
def showSyntax():
    print('usage: python rpm.py <name_of_rallypoint_status_file> [-i:<polling_interval>]')


# --------------------------------------------------------------------------
def ctrlcHandler(sig, frame):
    print('... exiting')
    sys.exit(0)


# --------------------------------------------------------------------------
def main():
    while 1:
        try:
            db = loadInput()
            clearScreen()

            try:
                printHeadline(db)            
                time.sleep(interval)
            except Exception as ie:
                print('error encountered - will retry shortly')
                time.sleep(interval * 2)
                pass

        except Exception as e:
            print(e)
            showSyntax()
            exit()

    if haveColorama:
        colorama.deinit()


# --------------------------------------------------------------------------
if __name__ == '__main__':
    if len(sys.argv) < 2:
        showSyntax()
        exit()
    else:
        try:
            for arg in sys.argv:
                if arg.startswith('-i:'):
                        interval = int(arg[3:])
        except:
            print('invalid argument ' + arg)
            showSyntax()
            exit()
    
    signal.signal(signal.SIGINT, ctrlcHandler)
    statusFile = sys.argv[1]
    main()

#
# RTS Network State Machine (nsm)
# Copyright (c) 2022 Rally Tactical Systems, Inc.
#

import socket
import struct
import threading
from threading import Thread, Lock
import time
import select
import os
import sys
import json
from datetime import datetime, timedelta
import random
import signal
import uuid
import argparse
import colorama

RX_INTERVAL_SECS = 1

LOG_FATAL = 0
LOG_ERROR = 1
LOG_WARN = 2
LOG_INFO = 3
LOG_DEBUG = 4

ST_NONE = 0
ST_IDLE = 1
ST_GOING_ACTIVE = 2
ST_ACTIVE = 3

global running
global rx
global tx
global mutex

global configuration
global trackers
global fixedToken
global packetsTx
global bytesTx
global packetsRx
global bytesRx
global errorsRx
global maxRunSecs

try:
    import colorama
    haveColorama = True
except ImportError as e:
    haveColorama = False
    pass

if haveColorama:
    colorama.init()
    pos = lambda y, x: colorama.Cursor.POS(x, y)

mutex = Lock()
packetsTx = 0
bytesTx = 0
packetsRx = 0
bytesRx = 0
errorsRx = 0
running = False
maxRunSecs = 0

parser = argparse.ArgumentParser(description='')
parser.add_argument('--config-file', type=str, help='Override the configuration file to use (default is nsm_conf.json)')
parser.add_argument('--id', type=str, help='Override the ID for this instance')
parser.add_argument('--log-level', type=int, help='Override the logging level in the configuration file')
parser.add_argument('--fixed-token', type=int, help='The global voting token to use (default is random per resource)')
parser.add_argument('--max-run-secs', type=int, help='Maximum number of seconds to run (generally only useful for testing')
args = parser.parse_args()

# --------------------------------------------------------------------------
def colorNone():
    if haveColorama:
        return colorama.Style.RESET_ALL
    else:
        return ''


# --------------------------------------------------------------------------
def colorRed():
    if haveColorama:
        return colorama.Fore.RED
    else:
        return ''


# --------------------------------------------------------------------------
def colorYellow():
    if haveColorama:
        return colorama.Fore.YELLOW
    else:
        return ''


# --------------------------------------------------------------------------
def colorGreen():
    if haveColorama:
        return colorama.Fore.GREEN
    else:
        return ''


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


# ---------------------------------------------------------------
def generateToken():
        global fixedToken

        if fixedToken >= 0:
                return fixedToken
        else:
                return random.randint(1, 100000)


# ---------------------------------------------------------------
def logThis(lvl, msg):
        if not configuration['logging']['dashboard']:
                if lvl <= configuration['logging']['level']:
                        if lvl == LOG_FATAL:
                                t = 'F'
                        elif lvl == LOG_ERROR:
                                t = 'E'
                        elif lvl == LOG_WARN:
                                t = 'W'
                        elif lvl == LOG_INFO:
                                t = 'I'
                        elif lvl == LOG_DEBUG:
                                t = 'D'

                        print(str(datetime.now()) + ' ' + t + ' : ' + msg)


# ---------------------------------------------------------------
def printErrorAndExit(msg):
        print('ERROR: ' + msg)
        sys.exit(1)


# ---------------------------------------------------------------
def setDefaultConfigurationValue(key, value, subkey=None, subvalue=None):
        global configuration

        if not key in configuration:
                configuration[key] = value

        if subkey != None:
                if not subkey in configuration[key]:
                        configuration[key][subkey] = subvalue


# ---------------------------------------------------------------
def loadConfiguration(path):
        global configuration
        global trackers

        trackers = {}
        
        with open(path) as f:
                configuration = json.load(f)

        setDefaultConfigurationValue('id', '')

        setDefaultConfigurationValue('networking', {})
        setDefaultConfigurationValue('networking', {}, 'interfaceName', '')
        setDefaultConfigurationValue('networking', {}, 'address', '')
        setDefaultConfigurationValue('networking', {}, 'port', 0)
        setDefaultConfigurationValue('networking', {}, 'ttl', 0)
        setDefaultConfigurationValue('networking', {}, 'txOversend', 0)
        setDefaultConfigurationValue('networking', {}, 'rxLossPercentage', 0)
        setDefaultConfigurationValue('networking', {}, 'txLossPercentage', 0)

        setDefaultConfigurationValue('resources', [])

        setDefaultConfigurationValue('run', {})
        setDefaultConfigurationValue('run', {}, 'onIdle', '')
        setDefaultConfigurationValue('run', {}, 'onGoingActive', '')
        setDefaultConfigurationValue('run', {}, 'onActive', '')

        setDefaultConfigurationValue('timing', {})
        setDefaultConfigurationValue('timing', {}, 'txIntervalSecs', 1)
        setDefaultConfigurationValue('timing', {}, 'transitionWaitSecs', 5)

        setDefaultConfigurationValue('logging', {})
        setDefaultConfigurationValue('logging', {}, 'level', 4)
        setDefaultConfigurationValue('logging', {}, 'dashboard', False)

        for key in configuration['resources']:
                trackers[key] = {}
                trackers[key]['res'] = key
                trackers[key]['state'] = ST_NONE
                trackers[key]['token'] = 0
                trackers[key]['goActiveTime'] = (datetime.now() + timedelta(seconds=configuration['timing']['transitionWaitSecs']))
                trackers[key]['owner'] = ''


# ---------------------------------------------------------------
def getIpAddressForInterface(nm):
        cmd = 'ifconfig ' + nm + ' | grep "inet " | awk \'{print $2}\''
        f = os.popen(cmd)
        rc = f.read()
        rc = rc.replace('\n', '')
        rc = rc.replace(' ', '')
        return rc


# ---------------------------------------------------------------
def checkConfiguration():
        global configuration
        
        if configuration['id'] == '':
                configuration['id'] = str(uuid.uuid1())

        if configuration['networking']['interfaceName'] == '':
                printErrorAndExit('no networking.interfaceName defined')

        configuration['networking']['interfaceAddress'] = getIpAddressForInterface(configuration['networking']['interfaceName'])
        if configuration['networking']['interfaceAddress'] == '':
                printErrorAndExit('cannot determine ip address for "' + configuration['networking']['interfaceName'] + '"')                        

        if configuration['networking']['address'] == '':
                printErrorAndExit('no networking.address defined')

        if configuration['networking']['port'] <= 0:
                printErrorAndExit('invalid networking.port')

        if configuration['networking']['ttl'] <= 0:
                printErrorAndExit('invalid networking.ttl')

        if len(configuration['resources']) == 0:
                printErrorAndExit('no resources defined')

        if configuration['run']['onIdle'] == '':
                printErrorAndExit('no run.onIdle defined')

        if configuration['run']['onGoingActive'] == '':
                printErrorAndExit('no run.onGoingActive defined')

        if configuration['run']['onActive'] == '':
                printErrorAndExit('no run.onActive defined')

        if configuration['timing']['txIntervalSecs'] <= 0:
                printErrorAndExit('invalid timing.txIntervalSecs')

        if configuration['timing']['transitionWaitSecs'] <= 0:
                printErrorAndExit('invalid timing.transitionWaitSecs')

        if configuration['timing']['transitionWaitSecs'] <= (RX_INTERVAL_SECS * 3):
                printErrorAndExit('timing.transitionWaitSecs cannot be less than ' + str(RX_INTERVAL_SECS * 3))

        if configuration['logging']['level'] < 0 or configuration['logging']['level'] > 4:
                printErrorAndExit('invalid logging.level')


# ---------------------------------------------------------------
def goActiveDesc(gat):
        if gat == datetime.max:
                return 'until further notice'
        else:
                return 'until at least ' + str(gat)


# ---------------------------------------------------------------
def stateDesc(s):
        if s == ST_NONE:
                return 'none'
        elif s == ST_IDLE:
                return 'idle'
        elif s == ST_GOING_ACTIVE:
                return 'going active'
        elif s == ST_ACTIVE:
                return 'active'

        return 'unknown'


# ---------------------------------------------------------------
def stateChange(tracker, newState):
        global configuration
        global state
        global trackers

        cmdToRun = ''

        if newState != tracker['state']:
                if newState == ST_IDLE:
                        logThis(LOG_DEBUG, 'idle ' + goActiveDesc(tracker['goActiveTime']))
                        cmdToRun = configuration['run']['onIdle']
                        tracker['token'] = 0

                elif newState == ST_GOING_ACTIVE:
                        logThis(LOG_DEBUG, 'going active ' + goActiveDesc(tracker['goActiveTime']))
                        cmdToRun = configuration['run']['onGoingActive']
                        tracker['token'] = generateToken()

                elif newState == ST_ACTIVE:
                        logThis(LOG_DEBUG, 'active ' + goActiveDesc(tracker['goActiveTime']))
                        cmdToRun = configuration['run']['onActive']

                else:
                        logThis(LOG_FATAL, '***STATE ERROR')
                        tracker['token'] = 0

                tracker['state'] = newState

        if cmdToRun != '':
                runCmd(tracker, cmdToRun)


# ---------------------------------------------------------------
def getState(res):
        global trackers

        mutex.acquire()
        rc = trackers[res]['state']
        mutex.release()
        return rc


# ---------------------------------------------------------------
def rxThread():
        global configuration
        global packetsRx
        global bytesRx
        global errorsRx

        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_LOOP, 1)
        sock.setblocking(False)

        #sock.bind((configuration['networking']['interfaceAddress'], configuration['networking']['port']))
        sock.bind(('', configuration['networking']['port']))
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton(configuration['networking']['interfaceAddress']))

        mreq = struct.pack('=4s4s', socket.inet_aton(configuration['networking']['address']), socket.inet_aton(configuration['networking']['interfaceAddress']))
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

        while running:
                ready = select.select([sock], [], [], RX_INTERVAL_SECS)

                if ready[0]:
                        data = sock.recv(10240)
                        try:
                                obj = json.loads(data)

                                if obj['i'] != configuration['id']:
                                        packetsRx = packetsRx + 1
                                        bytesRx = bytesRx + len(data)

                                if (configuration['networking']['rxLossPercentage'] == 0 or configuration['networking']['rxLossPercentage'] <= random.randint(1, 100)):
                                        processRx(obj)
                                else:
                                        processRx(None)
                        except:
                                errorsRx = errorsRx + 1
                                processRx(None)

                else:
                        processRx(None)


# ---------------------------------------------------------------
def txThread():
        global configuration
        global trackers
        global packetsTx
        global bytesTx

        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, configuration['networking']['ttl'])
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
        sock.bind((configuration['networking']['interfaceAddress'], configuration['networking']['port']))

        while running:
                time.sleep(configuration['timing']['txIntervalSecs'])

                sendTheMsg = False

                msg = '{'
                msg += '"i":"' + configuration['id'] + '"'
                msg += ',"r":['

                cnt = 0

                mutex.acquire()

                for res in trackers:
                        st = trackers[res]['state']

                        if st == ST_GOING_ACTIVE or st == ST_ACTIVE:
                                sendTheMsg = True
                                cnt = cnt + 1
                                if cnt > 1:
                                        msg += ','

                                msg += '{'
                                msg += '"r":"' + str(res) + '"'
                                msg += ',"t":' + str(trackers[res]['token'])
                                msg += ',"s":' + str(st)
                                msg += '}'

                mutex.release()                                

                msg += ']}'

                if sendTheMsg and (configuration['networking']['txLossPercentage'] == 0 or configuration['networking']['txLossPercentage'] <= random.randint(1, 100)):
                        for x in range(0, (configuration['networking']['txOversend'] + 1)):
                                logThis(LOG_DEBUG, 'sending (' + str(x + 1) + ') ' + msg)
                                sock.sendto(msg.encode(), (configuration['networking']['address'], configuration['networking']['port']))
                                packetsTx = packetsTx + 1
                                bytesTx = bytesTx + len(msg)


# ---------------------------------------------------------------
def processRx(obj):
        global trackers

        mutex.acquire()

        if obj != None:
                remoteId = obj['i']

                if remoteId != configuration['id']:
                        remoteTrackers = obj['r']

                        for res in trackers:
                                tracker = trackers[res]
                                remoteTracker = None
                                for remoteTracker in remoteTrackers:
                                        if remoteTracker['r'] == res:
                                                break

                                if remoteTracker != None:
                                        if remoteTracker['s'] == ST_ACTIVE:
                                                tracker['goActiveTime'] = datetime.max
                                                tracker['owner'] = remoteId
                                                logThis(LOG_DEBUG, res + ' @ ' + remoteId + ' is active, i will go or stay idle')
                                                stateChange(tracker, ST_IDLE)

                                        elif remoteTracker['s'] == ST_GOING_ACTIVE:
                                                if remoteTracker['t'] > tracker['token']:
                                                        tracker['goActiveTime'] = datetime.max
                                                        tracker['owner'] = remoteId
                                                        logThis(LOG_DEBUG, res + ' @ ' + remoteId + ' is going active with a higher token, i will go or stay idle')
                                                        stateChange(tracker, ST_IDLE)
                                                else:
                                                        tracker['owner'] = ''
                                                        logThis(LOG_DEBUG, res + ' @ ' + remoteId + ' is going active with a lower token, i will continue going or staying active')

        for res in trackers:
                tracker = trackers[res]

                if tracker['state'] == ST_NONE:
                        stateChange(tracker, ST_IDLE)

                else:
                        if tracker['goActiveTime'] != datetime.max:
                                if datetime.now() >= tracker['goActiveTime'] :
                                        if tracker['state'] == ST_IDLE:
                                                tracker['goActiveTime'] = (datetime.now() + timedelta(seconds=configuration['timing']['transitionWaitSecs']))
                                                stateChange(tracker, ST_GOING_ACTIVE)

                                        elif tracker['state'] == ST_GOING_ACTIVE:
                                                tracker['goActiveTime'] = datetime.max
                                                stateChange(tracker, ST_ACTIVE)

                        else:
                                if tracker['state'] == ST_IDLE:
                                        tracker['goActiveTime'] = (datetime.now() + timedelta(seconds=configuration['timing']['transitionWaitSecs']))

        mutex.release()


# ---------------------------------------------------------------
def startRxTx():
        global rx
        global tx

        rx = threading.Thread(target=rxThread)
        tx = threading.Thread(target=txThread)

        rx.start()
        tx.start()


# ---------------------------------------------------------------
def stopRxTx():
        global rx
        global tx

        tx.join()
        rx.join()


# ---------------------------------------------------------------
def ctrlcHandler(sig, frame):
        global running

        running = False


# ---------------------------------------------------------------
def runCmd(tracker, cmd):
        finalCmd = cmd.replace('${r}', tracker['res'])
        logThis(LOG_INFO, 'running "' + finalCmd + '"')
        os.system(finalCmd)


# ---------------------------------------------------------------
def showDashboard():
        global trackers
        global packetsTx
        global bytesTx
        global packetsRx
        global bytesRx
        global errorsRx
        global maxRunSecs

        clearScreen()
        printHeadline()

        if maxRunSecs > 0:
                print('%sLIMITED RUN TIME: %d SECONDS REMAINING%s' % (colorRed(), maxRunSecs, colorNone()))

        print('')
        print('ID: ' + configuration['id'])
        print('TX: %d packets (%d bytes)' % (packetsTx, bytesTx))
        print('RX: %d packets (%d bytes), [%d errors]' % (packetsRx, bytesRx, errorsRx))
        print('')

        print('%-20s %-18s %-20s' % ('Resource', 'Local State', 'Owner'))
        print('-------------------- ------------------ ------------------------------------')

        for res in trackers:
                tracker = trackers[res]
                if tracker['state'] == ST_IDLE:
                        clr = colorNone()
                elif tracker['state'] == ST_GOING_ACTIVE:
                        clr = colorYellow()
                elif tracker['state'] == ST_ACTIVE:
                        clr = colorGreen()

                if tracker['state'] == ST_IDLE:
                        print('%s%-20s %-18s %-36s%s' % (clr, res, stateDesc(tracker['state']), tracker['owner'], colorNone()))
                else:
                        print('%s%-20s %-18s %-36s%s' % (clr, res, stateDesc(tracker['state']), '(self)', colorNone()))


# ---------------------------------------------------------------
def printHeadline():
        print('-----------------------------------------------------------------------------------')
        print('nsm v0.1')
        print('Copyright (c) 2022 Rally Tactical Systems, Inc.')
        print('-----------------------------------------------------------------------------------')


# ---------------------------------------------------------------
if __name__ == "__main__":
        printHeadline()

        if args.config_file == None:
                loadConfiguration('nsm_conf.json')
        else:
                loadConfiguration(args.config_file)

        if args.id != None:
                configuration['id'] = args.id

        if args.fixed_token != None and args.fixed_token >= 0:
                fixedToken = args.fixed_token
        else:
                fixedToken = -1

        if args.log_level != None and args.log_level >= 0:
                configuration['logging']['level'] = args.log_level

        checkConfiguration()

        signal.signal(signal.SIGINT, ctrlcHandler)        

        logThis(LOG_INFO, 'starting for id: ' + configuration['id'] + ' via ' + configuration['networking']['interfaceAddress'] + ' on ' + configuration['networking']['address'] + '/' + str(configuration['networking']['port'] ))

        running = True
        processRx(None)

        startRxTx()

        if args.max_run_secs != None:
                maxRunSecs = args.max_run_secs
        else:
                maxRunSecs = 0

        while running:
                if configuration['logging']['dashboard']:
                        showDashboard()

                time.sleep(1)

                if maxRunSecs > 0:
                        maxRunSecs = maxRunSecs - 1
                        if maxRunSecs <= 0:
                                running = False
                                break
                

        logThis(LOG_INFO, 'stopping...')
        stopRxTx()

        for res in trackers:
                stateChange(trackers[res], ST_IDLE)

        print('done!')

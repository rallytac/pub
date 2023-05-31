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
import base64
import hashlib
import subprocess
import ipaddress

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
global startTs
global cryptoInstance
global netError
global dashboardToken

try:
        from cryptography.fernet import Fernet
        haveCrypto = True
except ImportError as e:
        haveCrypto = False
        pass
        
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
cryptoInstance = None
startTs = datetime.now()
netError = False
dashboardToken = False

parser = argparse.ArgumentParser(description='')
parser.add_argument('--config-file', type=str, help='Override the configuration file to use (default is nsm_conf.json)')
parser.add_argument('--id', type=str, help='Override the ID for this instance')
parser.add_argument('--log-level', type=int, help='Override the logging level in the configuration file')
parser.add_argument('--fixed-token', type=int, help='The global voting token to use (default is random per resource)')
parser.add_argument('--max-run-secs', type=int, help='Maximum number of seconds to run (generally only useful for testing')
parser.add_argument('--log-cmds', type=bool, help='Log command output(s)')
parser.add_argument('--dashboard-token', action=argparse.BooleanOptionalAction, help='Show the resource token in the dashboard')
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
def acquireMutex(msg):
        global mutex
        mutex.acquire()


# ---------------------------------------------------------------
def releaseMutex(msg):
        global mutex
        mutex.release()


# ---------------------------------------------------------------
def tokenPriority(token):
        return ((token & 0xff000000) >> 24)


# ---------------------------------------------------------------
def tokenRnd(token):
        return (token & 0x00ffffff)


# ---------------------------------------------------------------
def generateToken(tokenStart, tokenEnd):
        global fixedToken

        if fixedToken >= 0:
                return fixedToken
        else:
                return (configuration['priority'] << 24) + (random.randint(tokenStart, tokenEnd))


# ---------------------------------------------------------------
def logThis(lvl, msg):
        if not configuration['logging']['dashboard']:
                if lvl <= configuration['logging']['level']:
                        if lvl == LOG_FATAL:
                                clr = colorRed()
                                t = 'F'
                        elif lvl == LOG_ERROR:
                                clr = colorRed()
                                t = 'E'
                        elif lvl == LOG_WARN:
                                clr = colorYellow()
                                t = 'W'
                        elif lvl == LOG_INFO:
                                clr = colorGreen()
                                t = 'I'
                        elif lvl == LOG_DEBUG:
                                clr = colorNone()
                                t = 'D'

                        print('%s%s %s : %s%s' % (clr, str(datetime.now()), t, msg, colorNone()))



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
        setDefaultConfigurationValue('defaultPriority', 0)
        
        setDefaultConfigurationValue('networking', {})
        setDefaultConfigurationValue('networking', {}, 'interfaceName', '')
        setDefaultConfigurationValue('networking', {}, 'address', '')
        setDefaultConfigurationValue('networking', {}, 'port', 0)
        setDefaultConfigurationValue('networking', {}, 'ttl', 0)
        setDefaultConfigurationValue('networking', {}, 'tos', 0)
        setDefaultConfigurationValue('networking', {}, 'txOversend', 0)
        setDefaultConfigurationValue('networking', {}, 'rxLossPercentage', 0)
        setDefaultConfigurationValue('networking', {}, 'rxErrorPercentage', 0)
        setDefaultConfigurationValue('networking', {}, 'txLossPercentage', 0)
        setDefaultConfigurationValue('networking', {}, 'txErrorPercentage', 0)
        setDefaultConfigurationValue('networking', {}, 'cryptoPassword', '')

        setDefaultConfigurationValue('resources', [])

        setDefaultConfigurationValue('run', {})
        setDefaultConfigurationValue('run', {}, 'onIdle', '')
        setDefaultConfigurationValue('run', {}, 'beforeGoingActive', '')
        setDefaultConfigurationValue('run', {}, 'onGoingActive', '')
        setDefaultConfigurationValue('run', {}, 'beforeActive', '')
        setDefaultConfigurationValue('run', {}, 'onActive', '')

        setDefaultConfigurationValue('timing', {})
        setDefaultConfigurationValue('timing', {}, 'internalMultiplier', 1)
        setDefaultConfigurationValue('timing', {}, 'txIntervalSecs', 1)
        setDefaultConfigurationValue('timing', {}, 'transitionWaitSecs', 5)

        setDefaultConfigurationValue('electionToken', {})
        setDefaultConfigurationValue('electionToken', {}, 'start', 1000000)
        setDefaultConfigurationValue('electionToken', {}, 'end', 2000000)        

        setDefaultConfigurationValue('logging', {})
        setDefaultConfigurationValue('logging', {}, 'level', 3)
        setDefaultConfigurationValue('logging', {}, 'dashboard', True)
        setDefaultConfigurationValue('logging', {}, 'logCommandOutput', False)        

        for item in configuration['resources']:
                key = item['id']

                try:
                        priority = item['priority']
                except:
                        priority = configuration['defaultPriority']

                trackers[key] = {}
                trackers[key]['res'] = key
                trackers[key]['priority'] = priority
                trackers[key]['state'] = ST_NONE
                trackers[key]['token'] = 0
                trackers[key]['goActiveTime'] = (datetime.now() + timedelta(seconds=configuration['timing']['transitionWaitSecs']))
                trackers[key]['owner'] = ''
                trackers[key]['stunSecs'] = 0


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
        global RX_INTERVAL_SECS
        global configuration
        
        if configuration['id'] == '':
                configuration['id'] = str(uuid.uuid1())

        if configuration['networking']['interfaceName'] == '':
                printErrorAndExit('no networking.interfaceName defined')

        if configuration['networking']['address'] == '':
                printErrorAndExit('no networking.address defined')

        if isIpMulticast(configuration['networking']['address']):
                configuration['networking']['type'] = 'mc'
        elif isIpBroadcast(configuration['networking']['address']):
                configuration['networking']['type'] = 'bc'
        else:
                printErrorAndExit('networking.address specifies an unsupported address type')

        if configuration['networking']['port'] <= 0:
                printErrorAndExit('invalid networking.port')

        if configuration['networking']['ttl'] <= 0:
                printErrorAndExit('invalid networking.ttl')

        if configuration['networking']['tos'] < 0:
                printErrorAndExit('invalid networking.tos')

        if len(configuration['resources']) == 0:
                printErrorAndExit('no resources defined')

        if configuration['run']['onIdle'] == '':
                printErrorAndExit('no run.onIdle defined')

        if configuration['run']['onGoingActive'] == '':
                printErrorAndExit('no run.onGoingActive defined')

        if configuration['run']['onActive'] == '':
                printErrorAndExit('no run.onActive defined')

        if configuration['timing']['internalMultiplier'] <= 0:
                printErrorAndExit('invalid timing.internalMultiplier')                

        if configuration['timing']['txIntervalSecs'] <= 0:
                printErrorAndExit('invalid timing.txIntervalSecs')        

        if configuration['timing']['transitionWaitSecs'] <= 0:
                printErrorAndExit('invalid timing.transitionWaitSecs')        

        if configuration['timing']['transitionWaitSecs'] < (RX_INTERVAL_SECS * 3):
                printErrorAndExit('timing.transitionWaitSecs cannot be less than ' + str(RX_INTERVAL_SECS * 3))

        configuration['timing']['txIntervalSecs'] = configuration['timing']['txIntervalSecs'] * configuration['timing']['internalMultiplier']
        configuration['timing']['transitionWaitSecs'] = configuration['timing']['transitionWaitSecs'] * configuration['timing']['internalMultiplier']

        if configuration['electionToken']['start'] <= 0:
                printErrorAndExit('electionToken.start cannot be 0 or negative')

        if (configuration['electionToken']['end'] <= configuration['electionToken']['start']):
                printErrorAndExit('electionToken.end must be greater than electionToken.start')

        if (configuration['electionToken']['end'] - configuration['electionToken']['start']) < 1000:
                printErrorAndExit('electionToken.start and electionToken.end must have a range of 1000 or more')

        if configuration['logging']['level'] < 0 or configuration['logging']['level'] > 4:
                printErrorAndExit('invalid logging.level')


# ---------------------------------------------------------------
def initCrypto():
        global cryptoInstance

        if haveCrypto:
                if configuration['networking']['cryptoPassword'] != None and configuration['networking']['cryptoPassword'] != '':
                        kdfBytes = hashlib.pbkdf2_hmac('sha256', str.encode(configuration['networking']['cryptoPassword']), b'04DBAA5900D5421D9CCB25A54ED4FA56', 10000, 32)
                        key = base64.urlsafe_b64encode(kdfBytes)
                        cryptoInstance = Fernet(key)
                

# ---------------------------------------------------------------
def wireMsgFromString(s):
        global cryptoInstance

        if cryptoInstance != None:
                return cryptoInstance.encrypt(str.encode(s))
        else:
                return s.encode()


# ---------------------------------------------------------------
def stringFromWireMsg(b):
        global cryptoInstance

        if cryptoInstance != None:
                return cryptoInstance.decrypt(b).decode()
        else:
                return b.decode()


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
def stateChange(tracker, newState, tokenRange=None):
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
                        tracker['token'] = generateToken(tokenRange[0], tokenRange[1])

                elif newState == ST_ACTIVE:
                        logThis(LOG_DEBUG, 'active ' + goActiveDesc(tracker['goActiveTime']))
                        cmdToRun = configuration['run']['onActive']

                else:
                        logThis(LOG_FATAL, '***STATE ERROR')
                        tracker['token'] = 0

                tracker['state'] = newState

        if cmdToRun != '':
                outputString = runCmd(tracker, cmdToRun)
                if configuration['logging']['logCommandOutput']:
                        print(outputString)


# ---------------------------------------------------------------
def getState(res):
        global trackers

        acquireMutex('getState')
        rc = trackers[res]['state']
        releaseMutex('getState')
        return rc


# ---------------------------------------------------------------
def getExternalTokenRange(tracker):
        global DEFAULT_TOKEN_RANGE
        global configuration

        if configuration['run']['beforeGoingActive'] != '':
                try:
                        s = runCmd(tracker, configuration['run']['beforeGoingActive'])
                        s = s.replace('\n', '')
                        ars = s.split('-')
                        if len(ars) != 2:
                                raise Exception()
                        
                        if int(ars[0]) <= 0 or int(ars[1]) <= 0:
                                logThis(LOG_ERROR, configuration['run']['beforeGoingActive'] + ' returned invalid token range values (zero or negative)')
                                raise Exception()

                        if int(ars[0]) >= int(ars[1]):
                                logThis(LOG_ERROR, configuration['run']['beforeGoingActive'] + ' returned invalid token range values (start and end reversed)')
                                raise Exception()

                        if int(ars[1]) - int(ars[0]) < 1000:
                                logThis(LOG_ERROR, configuration['run']['beforeGoingActive'] + ' returned an invalid token range')
                                raise Exception()

                        rc = [int(ars[0]), int(ars[1])]
                except:
                        rc = None
        else:
                rc = [configuration['electionToken']['start'], configuration['electionToken']['end']]
        
        return rc


# ---------------------------------------------------------------
def getExternalConfirmation(tracker):
        global configuration

        if configuration['run']['beforeActive'] != '':
                try:
                        s = runCmd(tracker, configuration['run']['beforeActive'])
                        s = s.replace('\n', '')
                        rc = (int(s) == 1)
                except:
                        rc = False
        else:
                rc = True
        
        return rc


# ---------------------------------------------------------------
def rxThread():
        global configuration
        global packetsRx
        global bytesRx
        global errorsRx
        global netError

        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)

        if configuration['networking']['type'] == 'mc':
                sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_LOOP, 1)
        elif configuration['networking']['type'] == 'bc':
                sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

        sock.setblocking(False)

        #sock.bind((configuration['networking']['interfaceAddress'], configuration['networking']['port']))
        sock.bind(('', configuration['networking']['port']))

        if configuration['networking']['type'] == 'mc':
                sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton(configuration['networking']['interfaceAddress']))
                mreq = struct.pack('=4s4s', socket.inet_aton(configuration['networking']['address']), socket.inet_aton(configuration['networking']['interfaceAddress']))
                sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

        while running and not netError:
                ready = select.select([sock], [], [], (RX_INTERVAL_SECS * configuration['timing']['internalMultiplier']))

                if ready[0]:
                        wireMsg = sock.recv(10240)
                        try:
                                toRecv = stringFromWireMsg(wireMsg)
                                obj = json.loads(toRecv)

                                if obj['i'] != configuration['id']:
                                        packetsRx = packetsRx + 1
                                        bytesRx = bytesRx + len(wireMsg)

                                if (configuration['networking']['rxErrorPercentage'] == 0 or configuration['networking']['rxErrorPercentage'] <= random.randint(1, 100)):
                                        if (configuration['networking']['rxLossPercentage'] == 0 or configuration['networking']['rxLossPercentage'] <= random.randint(1, 100)):
                                                processRx(obj)
                                        else:
                                                processRx(None)
                                else:
                                        raise Exception('forced rxError!')
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
        global netError

        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

        if configuration['networking']['type'] == 'mc':
                sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, configuration['networking']['ttl'])
        elif configuration['networking']['type'] == 'bc':
                sock.setsockopt(socket.IPPROTO_IP, socket.IP_TTL, configuration['networking']['ttl'])
                sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

        sock.setsockopt(socket.IPPROTO_IP, socket.IP_TOS, configuration['networking']['tos'])
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
        sock.bind((configuration['networking']['interfaceAddress'], configuration['networking']['port']))

        while running and not netError:
                try:
                        time.sleep(configuration['timing']['txIntervalSecs'])

                        sendTheMsg = False

                        msg = '{'
                        msg += '"i":"' + configuration['id'] + '"'
                        msg += ',"r":['

                        cnt = 0

                        acquireMutex('txThread')

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

                        releaseMutex('txThread')

                        msg += ']}'

                        if sendTheMsg and (configuration['networking']['txLossPercentage'] == 0 or configuration['networking']['txLossPercentage'] <= random.randint(1, 100)):
                                for x in range(0, (configuration['networking']['txOversend'] + 1)):
                                        logThis(LOG_DEBUG, 'sending (' + str(x + 1) + ') ' + msg)
                                        wireMsg = wireMsgFromString(msg)
                                        sock.sendto(wireMsg, (configuration['networking']['address'], configuration['networking']['port']))
                                        packetsTx = packetsTx + 1
                                        bytesTx = bytesTx + len(wireMsg)
                except:
                        netError = True


# ---------------------------------------------------------------
def processRx(obj):
        global trackers

        acquireMutex('processRx')

        if obj != None:
                remoteId = obj['i']

                if remoteId != configuration['id']:
                        remoteTrackers = obj['r']

                        for res in trackers:
                                tracker = trackers[res]
                                remoteTracker = None
                                remoteFound = False
                                for remoteTracker in remoteTrackers:
                                        if remoteTracker['r'] == res:
                                                remoteFound = True
                                                break

                                if not remoteFound:
                                        remoteTracker = None

                                if remoteTracker != None:
                                        if remoteTracker['s'] == ST_ACTIVE:
                                                if configuration['priority'] > tokenPriority(remoteTracker['t']):
                                                        logThis(LOG_DEBUG, res + ' @ ' + remoteId + ' is active with a lower priority token, i will ignore it and accelerate my interval')
                                                        tracker['goActiveTime'] = datetime.now()
                                                else:
                                                        tracker['goActiveTime'] = datetime.max
                                                        tracker['owner'] = remoteId
                                                        logThis(LOG_DEBUG, res + ' @ ' + remoteId + ' is active, i will go or stay idle')
                                                        stateChange(tracker, ST_IDLE)

                                        elif remoteTracker['s'] == ST_GOING_ACTIVE:
                                                if configuration['priority'] > tokenPriority(remoteTracker['t']):
                                                        logThis(LOG_DEBUG, res + ' @ ' + remoteId + ' is going active with a lower priority token, i will ignore it and accelerate my interval')
                                                        tracker['goActiveTime'] = datetime.now()
                                                else:
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
                                                tokenRange = getExternalTokenRange(tracker,)
                                                if tokenRange != None:
                                                        tracker['goActiveTime'] = datetime.now() + timedelta(seconds=(configuration['timing']['transitionWaitSecs']))                                                        
                                                        logThis(LOG_DEBUG, res + ' is going active with a token range of ' + str(tokenRange[0]) + '-' + str(tokenRange[1]))
                                                        stateChange(tracker, ST_GOING_ACTIVE, tokenRange)
                                                else:
                                                        logThis(LOG_DEBUG, res + ' is denied going active - remaining idle')

                                        elif tracker['state'] == ST_GOING_ACTIVE:
                                                if getExternalConfirmation(tracker) == True:
                                                        tracker['goActiveTime'] = datetime.max
                                                        tracker['stunSecs'] = 0
                                                        stateChange(tracker, ST_ACTIVE)
                                                else:
                                                        logThis(LOG_WARN, res + ' is denied active - going idle and stunning for ' + str(configuration['timing']['transitionWaitSecs'] * 5) + ' seconds')
                                                        tracker['goActiveTime'] = datetime.max
                                                        tracker['stunSecs'] = (configuration['timing']['transitionWaitSecs'] * 5)
                                                        stateChange(tracker, ST_IDLE)

                        else:
                                if tracker['state'] == ST_IDLE:
                                        tracker['goActiveTime'] = (datetime.now() + timedelta(seconds=(configuration['timing']['transitionWaitSecs'] + tracker['stunSecs'])))
                                        tracker['stunSecs'] = 0

        releaseMutex('processRx')


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
def sigHandler(sig, frame):
        global running

        running = False


# ---------------------------------------------------------------
def runCmd(tracker, cmd):
        logThis(LOG_INFO, 'running "' + cmd + '" for ' + tracker['res'])
        res = subprocess.run([cmd, tracker['res']], stdout=subprocess.PIPE)
        return res.stdout.decode('utf-8')


# ---------------------------------------------------------------
def showDashboard():
        global trackers
        global packetsTx
        global bytesTx
        global packetsRx
        global bytesRx
        global errorsRx
        global maxRunSecs
        global cryptoInstance

        clearScreen()
        printHeadline()

        if maxRunSecs > 0:
                print('%sLIMITED RUN TIME: %d SECONDS REMAINING%s' % (colorRed(), maxRunSecs, colorNone()))

        uptime = (datetime.now() - startTs).seconds
        if uptime > 0:
                txKbps = (((bytesTx * 8) / uptime) / 1000)
                rxKbps = (((bytesRx * 8) / uptime) / 1000)
        else:
                txKbps = 0
                rxKbps = 0

        print('')
        print('ID  : ' + configuration['id'])
        print('PRI : ' + str(configuration['priority']))

        print('UP  : ' + str(uptime) + ' seconds')

        print('')        
        print('NET: %s:%d %s' % (configuration['networking']['address'], configuration['networking']['port'], ('*ENCRYPTED*' if cryptoInstance != None else '*CLEAR*')))
        
        if configuration['networking']['txLossPercentage'] > 0:
                extraInfo = ' [' + str(configuration['networking']['txLossPercentage']) + '% forced TX loss]'
        else:
                extraInfo = ''

        if configuration['networking']['txOversend'] > 0:
                extraInfo = extraInfo + ' [' + str(configuration['networking']['txOversend']) + 'X oversend]'

        print('TX : %d packets, %d bytes, %.2f Kbps%s' % (packetsTx, bytesTx, txKbps, extraInfo))
                
        if configuration['networking']['rxLossPercentage'] > 0:
                extraInfo = ' [' + str(configuration['networking']['rxLossPercentage']) + '% forced RX loss]'
        else:
                extraInfo = ''

        print('RX : %d packets, %d bytes, %.2f Kbps, [%d errors]%s' % (packetsRx, bytesRx, rxKbps, errorsRx, extraInfo))

        print('')
        if dashboardToken:
                print('%-20s %-18s %-36s %-12s' % ('Resource', 'Local State', 'Owner', 'Token'))
                print('-------------------- ------------------ ------------------------------------ ------------')
        else:
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
                elif tracker['state'] == ST_NONE:
                        clr = colorYellow()
                else:
                        clr = colorNone()

                if dashboardToken:
                        token = ' ' + format(tracker['token'], "08x")
                else:
                        token = ''

                if tracker['state'] == ST_NONE:
                        print('%s%-20s %-18s %-36s%s' % (clr, res, stateDesc(tracker['state']), '?', colorNone()))
                elif tracker['state'] == ST_IDLE:
                        print('%s%-20s %-18s %-36s%s%s' % (clr, res, stateDesc(tracker['state']), tracker['owner'], token, colorNone()))
                else:
                        print('%s%-20s %-18s %-36s%s%s' % (clr, res, stateDesc(tracker['state']), '(self)', token, colorNone()))


# ---------------------------------------------------------------
def printHeadline():
        print('-----------------------------------------------------------------------------------')
        print('nsm v0.4')
        print('Copyright (c) 2022 Rally Tactical Systems, Inc.')
        print('-----------------------------------------------------------------------------------')


# ---------------------------------------------------------------
def determineIpAddressOfInterface():
        global configuration

        configuration['networking']['interfaceAddress'] = getIpAddressForInterface(configuration['networking']['interfaceName'])
        if configuration['networking']['interfaceAddress'] == '':
                return False
        else:
                return True


# ---------------------------------------------------------------
def isIpMulticast(ip):
        try:
                return ipaddress.ip_address(ip).is_multicast
        except:
                return False


# ---------------------------------------------------------------
def isIpBroadcast(ip):
        return ip == '255.255.255.255'


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
                

        if args.dashboard_token:
                dashboardToken = True

        checkConfiguration()
        initCrypto()

        running = True
        signal.signal(signal.SIGINT, sigHandler)
        signal.signal(signal.SIGTERM, sigHandler)
        signal.signal(signal.SIGHUP, sigHandler)

        while running:
                if configuration['logging']['dashboard']:
                        showDashboard()

                netError = False

                if not determineIpAddressOfInterface():
                        logThis(LOG_INFO, 'waiting to determine IP address for interface ' + configuration['networking']['interfaceName'])
                        time.sleep(1)
                        continue                        

                logThis(LOG_INFO, 'starting for id: ' + configuration['id'] + ' via ' + configuration['networking']['interfaceAddress'] + ' on ' + configuration['networking']['address'] + '/' + str(configuration['networking']['port'] ))

                processRx(None)

                startRxTx()

                if args.max_run_secs != None:
                        maxRunSecs = args.max_run_secs
                else:
                        maxRunSecs = 0

                while running and not netError:
                        if configuration['logging']['dashboard']:
                                showDashboard()
                                time.sleep(1 * configuration['timing']['internalMultiplier'])
                        else:
                                time.sleep(1)

                        if maxRunSecs > 0:
                                maxRunSecs = maxRunSecs - (1 * configuration['timing']['internalMultiplier'])
                                if maxRunSecs <= 0:
                                        running = False
                                        break                        

                logThis(LOG_INFO, 'stopping...')
                stopRxTx()

                for res in trackers:
                        stateChange(trackers[res], ST_IDLE)

        print('done!')

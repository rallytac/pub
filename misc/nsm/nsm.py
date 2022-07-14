#
# RTS Network State Machine (nsm)
# Copyright (c) 2022 Rally Tactical Systems, Inc.
#

from distutils.command.config import config
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
global state
global myToken
global mutex

global configuration


# ---------------------------------------------------------------
def logThis(lvl, msg):
        if lvl <= configuration['logging']['level']:
                if lvl == LOG_FATAL:
                        t = 'F'
                print(str(datetime.now()) + ' : ' + str(lvl) + ' : ' + msg)


# ---------------------------------------------------------------
def printErrorAndExit(msg):
        print('ERROR: ' + msg)
        sys.exit(1)


# ---------------------------------------------------------------
def loadConfiguration(path):
        global configuration
        
        with open(path) as f:
                configuration = json.load(f)


# ---------------------------------------------------------------
def checkConfiguration():
        global configuration
        
        if configuration['id'] == '':
                printErrorAndExit('no id defined')

        if configuration['networking']['interfaceAddress'] == '':
                printErrorAndExit('no networking.interfaceAddress defined')

        if configuration['networking']['address'] == '':
                printErrorAndExit('no networking.address defined')

        if configuration['networking']['port'] <= 0:
                printErrorAndExit('invalid networking.port')

        if configuration['networking']['ttl'] <= 0:
                printErrorAndExit('invalid networking.ttl')

        if configuration['run']['onIdle'] == '':
                printErrorAndExit('no run.onIdle defined')

        if configuration['run']['onGoingActive'] == '':
                printErrorAndExit('no run.onGoingActive defined')

        if configuration['run']['onActive'] == '':
                printErrorAndExit('no run.onActive defined')

        if configuration['timing']['rxIntervalSecs'] <= 0:
                printErrorAndExit('invalid timing.rxIntervalSecs')

        if configuration['timing']['txIntervalSecs'] <= 0:
                printErrorAndExit('invalid timing.txIntervalSecs')

        if configuration['timing']['transitionWaitSecs'] <= 0:
                printErrorAndExit('invalid timing.transitionWaitSecs')

        if configuration['timing']['transitionWaitSecs'] <= (configuration['timing']['rxIntervalSecs'] * 3):
                printErrorAndExit('timing.transitionWaitSecs cannot be less than timing.rxIntervalSecs * 3')                

        if configuration['logging']['level'] < 0 or configuration['logging']['level'] > 4:
                printErrorAndExit('invalid logging.level')


# ---------------------------------------------------------------
def stateChange(newState):
        global configuration
        global state
        global myToken        

        cmdToRun = ''

        mutex.acquire()

        if newState != state:
                if newState == ST_IDLE:
                        logThis(LOG_DEBUG, '--->ST_IDLE')
                        cmdToRun = configuration['run']['onIdle']
                        myToken = 0

                elif newState == ST_GOING_ACTIVE:
                        logThis(LOG_DEBUG, '--->ST_GOING_ACTIVE')
                        cmdToRun = configuration['run']['onGoingActive']
                        myToken = random.randint(1, 100000)

                elif newState == ST_ACTIVE:
                        logThis(LOG_DEBUG, '--->ST_ACTIVE')
                        cmdToRun = configuration['run']['onActive']

                else:
                        logThis(LOG_FATAL, '***STATE ERROR')
                        myToken = 0

                state = newState

        mutex.release()

        if cmdToRun != '':
                runCmd(cmdToRun)


# ---------------------------------------------------------------
def getState():
        global state

        mutex.acquire()
        rc = state
        mutex.release()
        return rc


# ---------------------------------------------------------------
def rxThread():
        global configuration

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

        goActiveTime = (datetime.now() + timedelta(seconds=configuration['timing']['transitionWaitSecs']))

        while running:
                ready = select.select([sock], [], [], configuration['timing']['rxIntervalSecs'])

                if ready[0]:
                        data = sock.recv(10240)
                        obj = json.loads(data)

                        remoteId = obj['i']
                        remoteToken = obj['t']
                        remoteState = obj['s']

                        if remoteId != configuration['id']:
                                if remoteState == ST_ACTIVE:
                                        goActiveTime = datetime.max
                                        stateChange(ST_IDLE)
                                        logThis(LOG_DEBUG, remoteId + ' is active, going or staying idle')

                                elif remoteState == ST_GOING_ACTIVE:
                                        if remoteToken > myToken:
                                                goActiveTime = datetime.max
                                                stateChange(ST_IDLE)
                                                logThis(LOG_DEBUG, remoteId + ' is going active with a higher token, going or staying idle')

                else:
                        if goActiveTime == datetime.max:
                                if getState() == ST_IDLE:
                                        goActiveTime = (datetime.now() + timedelta(seconds=configuration['timing']['transitionWaitSecs']))

                if goActiveTime != datetime.max:
                        if datetime.now() >= goActiveTime:
                                if getState() == ST_IDLE:
                                        goActiveTime = (datetime.now() + timedelta(seconds=configuration['timing']['transitionWaitSecs']))
                                        stateChange(ST_GOING_ACTIVE)

                                elif getState() == ST_GOING_ACTIVE:
                                        goActiveTime = datetime.max
                                        stateChange(ST_ACTIVE)


# ---------------------------------------------------------------
def txThread():
        global configuration

        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, configuration['networking']['ttl'])
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
        sock.bind((configuration['networking']['interfaceAddress'], configuration['networking']['port']))

        while running:
                time.sleep(configuration['timing']['txIntervalSecs'])

                sendTheMsg = False
                st = getState()

                if st == ST_GOING_ACTIVE or st == ST_ACTIVE:
                        sendTheMsg = True
                        msg = '{'
                        msg += '"i":"' + configuration['id'] + '"'
                        msg += ',"t":' + str(myToken)
                        msg += ',"s":' + str(st)
                        msg += '}'

                if sendTheMsg:
                        logThis(LOG_DEBUG, 'sending ' + msg)
                        sock.sendto(msg.encode(), (configuration['networking']['address'], configuration['networking']['port']))


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
def runCmd(cmd):
        logThis(LOG_INFO, 'running "' + cmd + '"')
        os.system(cmd)


# ---------------------------------------------------------------
if __name__ == "__main__":
        print('-----------------------------------------------------------------------------------')
        print('nsm v0.1')
        print('Copyright (c) 2022 Rally Tactical Systems, Inc.')
        print('-----------------------------------------------------------------------------------')

        loadConfiguration('nsm_conf.json')
        checkConfiguration()

        signal.signal(signal.SIGINT, ctrlcHandler)
        mutex = Lock()

        logThis(LOG_INFO, 'starting for id: ' + configuration['id'] + ' via ' + configuration['networking']['interfaceAddress'] + ' on ' + configuration['networking']['address'] + '/' + str(configuration['networking']['port'] ))

        state = ST_NONE
        stateChange(ST_IDLE)

        running = True
        myToken = 0

        startRxTx()

        while running:
                time.sleep(1)

        logThis(LOG_INFO, 'stopping...')
        stopRxTx()

        stateChange(ST_IDLE)

        print('done!')

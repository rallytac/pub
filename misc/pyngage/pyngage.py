#
# pyngage
# Copyright (c) 2022 Rally Tactical Systems, Inc.
#

from copy import deepcopy
import pyaudio
from rtp import RTP, Extension, PayloadType
import socket
import numpy as np
import time
import threading
import queue
from threading import Timer
import argparse

# Constants
MAX_UDP_PACKET_SIZE = 4096                          # The maximum size of a UDP packet
SAMPLING_RATE = 8000                                # The microphone will be configured to sample at this rate ...
SAMPLING_CHANNELS = 1                               # ... in mono mode
FRAMING_MS = 60                                     # We'll pack this many milliseconds of audio into an RTP packet
SEND_INTERVAL = (FRAMING_MS / 1000)                 # Our sender thread needs to stream packets out at this rate
MIC_SAMPLES_PER_READ = (8 * FRAMING_MS)             # We'll need to capture this many samples from the microphone at a time
ENGAGE_PCM_CODEC_RTP_PT = PayloadType.DYNAMIC_99    # We're using RTP payload type 99 for Engage's linear PCM coder

# Globals
g_running = True
g_txQueue = queue.Queue()
g_txSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
g_rxSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
g_txPackets = 0
g_rxPackets = 0

parser = argparse.ArgumentParser(description='')

meg = parser.add_mutually_exclusive_group(required=True)
meg.add_argument('-tx', action='store_true', help='Transmit')
meg.add_argument('-rx', action='store_true', help='Receive')

parser.add_argument('-addr', required=True, type=str, help='Address for TX or RX')
parser.add_argument('-port', required=True, type=int, help='Port for TX or RX')
args = parser.parse_args()

g_myPyAudio = pyaudio.PyAudio()


def rxThread():
    global g_running
    global g_rxSocket
    global g_myPyAudio
    global g_rxPackets

    # Open the speaker
    spkStream = g_myPyAudio.open(format=pyaudio.paInt16,
                    channels=SAMPLING_CHANNELS,
                    rate=SAMPLING_RATE,
                    output=True)

    # Make ourselves a RTP packet we can receive into
    rtpPacket = RTP()

    while g_running:
        # Read the incoming packet data
        packetData, srcAddress = g_rxSocket.recvfrom(MAX_UDP_PACKET_SIZE)

        # We may receive garbage so protect this a little
        try:
            # Parse the RTP packet
            rtpPacket.fromBytes(packetData)

            # Do a basic RTP packet check
            if rtpPacket.version == 2 and rtpPacket.payloadType == ENGAGE_PCM_CODEC_RTP_PT:
                # We need a payload
                if(len(rtpPacket.payload) >= 2):
                    # Make a byte array from it...
                    ba = np.frombuffer(rtpPacket.payload, dtype=np.byte)

                    # NOTE: Normally the decoded audio - along with it's
                    # RTP packet information - would go into a jitter buffer
                    # at this point.  A seperate thread handling the speaker
                    # would pull the audio from that jitter buffer.  For
                    # simplicity's sake, though, we're not doing that here but
                    # instead simply taking the audio payload and sending it
                    # directly to the speaker

                    # ... and write it to the speaker
                    spkStream.write(ba)

                    # Some stats
                    g_rxPackets = g_rxPackets + 1
                    if g_rxPackets % 10 == 0:
                        print('rx: %s packets' % (g_rxPackets))

        except:
            pass


# A little class to make a repeating timer
class RepeatTimer(Timer):  
    def run(self):  
        while not self.finished.wait(self.interval):  
            self.function(*self.args,**self.kwargs) 


# Called to send the next RTP packet on the output queue (if any)
def sendNextPacketBuffer():
    global g_txSocket
    global g_txPackets
    global args

    if not g_txQueue.empty():
        ba = g_txQueue.get()
        g_txSocket.sendto(ba, (args.addr, args.port))

        # Some stats
        g_txPackets = g_txPackets + 1
        if g_txPackets % 10 == 0:
            print('tx: %s packets' % (g_txPackets))



# This thread captures samples from the microphone, constructs an RTP packet, and 
# appends it to the TX queue
def micThread():
    global g_txQueue
    global g_myPyAudio

    # Open the microphone
    micStream = g_myPyAudio.open(format=pyaudio.paInt16,
                    channels=SAMPLING_CHANNELS,
                    rate=SAMPLING_RATE,
                    input=True)

    # Setup the RTP packet and its first values
    rtpPacket = RTP(marker=True,
        sequenceNumber=1,
        timestamp=1,
        payloadType=ENGAGE_PCM_CODEC_RTP_PT
    )

    while g_running:
        # Read the PCM samples from the microphone - this is captured as 16-bit integers but returned
        # to us as an array of bytes
        micBytes = micStream.read(MIC_SAMPLES_PER_READ)

        # Set the RTP packet's payload (the PCM audio)
        rtpPacket.payload = bytearray(micBytes)        

        # Get the array of bytes that represent the entire RTP packet
        ba = rtpPacket.toBytearray()
        
        # Put it onto the queue but make sure we make a deep copy because
        # another thread is going to pulling it off the queue.
        g_txQueue.put(deepcopy(ba))

        # Prepare the next RTP packet - increment the sequence by 1, clear the marker bit, and bump
        # the RTP timestamp by the number of audio samples we just read from the microphone above
        rtpPacket.sequenceNumber += 1
        rtpPacket.marker = False
        rtpPacket.timestamp += int(MIC_SAMPLES_PER_READ / 2)


if __name__ == "__main__":
    g_running = True

    if args.tx:
        # Fire up a timer to send RTP packets
        txTimer = RepeatTimer(SEND_INTERVAL, sendNextPacketBuffer)  
        txTimer.start()

        # Fire up the microphone thread
        mt = threading.Thread(target=micThread)
        mt.start()

        while g_running:
            time.sleep(1)

        txTimer.cancel()
        mt.join()
        
    elif args.rx:
        # Bind our socket if we're receiving
        g_rxSocket.bind((args.addr, args.port))

        # Fire up the receiver thread
        rt = threading.Thread(target=rxThread)
        rt.start()

        while g_running:
            time.sleep(1)

        rt.join()

    
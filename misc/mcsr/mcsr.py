#!/usr/bin/env python3
#
# Engage Provisioning Tool
# Copyright (c) 2025 Rally Tactical Systems, Inc.
#

"""
Multicast UDP Packet Sender/Receiver

This script can send or receive multicast UDP packets with configurable parameters:
- Network interface
- Multicast address
- Port
- Packet size
- Packet count (for sending)
- Send interval (for sending)

Usage:
    python mcsr.py send --interface eth0 --address 224.1.1.1 --port 5000 --size 1024 --count 10 --interval 1.0
    python mcsr.py receive --interface eth0 --address 224.1.1.1 --port 5000
"""

import argparse
import socket
import struct
import time
import sys
import threading
import signal


class MulticastUDPSender:
    def __init__(self, interface, address, port, packet_size, count, interval):
        self.interface = interface
        self.address = address
        self.port = port
        self.packet_size = packet_size
        self.count = count
        self.interval = interval
        self.sock = None
        self.running = True

    def create_socket(self):
        """Create and configure the multicast socket for sending"""
        try:
            # Create UDP socket
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            
            # Set socket options for multicast
            self.sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 32)
            
            # Bind to specific interface if specified
            if self.interface:
                # Get interface IP address
                interface_ip = self._get_interface_ip(self.interface)
                if interface_ip:
                    self.sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, 
                                       socket.inet_aton(interface_ip))
                    print(f"Using interface {self.interface} with IP {interface_ip}")
                else:
                    print(f"Warning: Could not get IP for interface {self.interface}")
            
            print(f"Socket created for sending to {self.address}:{self.port}")
            
        except Exception as e:
            print(f"Error creating socket: {e}")
            sys.exit(1)

    def _get_interface_ip(self, interface):
        """Get the IP address of a network interface"""
        try:
            import netifaces
            addrs = netifaces.ifaddresses(interface)
            if netifaces.AF_INET in addrs:
                return addrs[netifaces.AF_INET][0]['addr']
        except ImportError:
            # Fallback method using socket
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                s.connect(("8.8.8.8", 80))
                local_ip = s.getsockname()[0]
                s.close()
                return local_ip
            except:
                pass
        except:
            pass
        return None

    def send_packets(self):
        """Send multicast packets"""
        self.create_socket()
        
        print(f"Sending {self.count} packets of {self.packet_size} bytes to {self.address}:{self.port}")
        print(f"Send interval: {self.interval} milliseconds")
        print("Press Ctrl+C to stop early\n")
        
        try:
            for i in range(self.count):
                if not self.running:
                    break
                    
                # Create packet data
                packet_data = self._create_packet_data(i)
                
                # Send packet
                self.sock.sendto(packet_data, (self.address, self.port))
                
                print(f"Sent packet {i+1}/{self.count} ({len(packet_data)} bytes)")
                
                # Wait for interval (except for last packet)
                if i < self.count - 1 and self.interval > 0:
                    time.sleep(self.interval / 1000.0)
                    
        except KeyboardInterrupt:
            print("\nSending interrupted by user")
        except Exception as e:
            print(f"Error sending packets: {e}")
        finally:
            if self.sock:
                self.sock.close()
            print("Sender finished")

    def _create_packet_data(self, packet_num):
        """Create packet data with sequence number and timestamp"""
        # Create packet with sequence number, timestamp, and padding
        timestamp = int(time.time() * 1000000)  # microseconds
        seq_data = struct.pack('!QQ', packet_num, timestamp)  # 16 bytes
        
        # Add padding to reach desired packet size
        padding_size = max(0, self.packet_size - len(seq_data))
        padding = b'X' * padding_size
        
        return seq_data + padding

    def stop(self):
        """Stop the sender"""
        self.running = False


class MulticastUDPReceiver:
    def __init__(self, interface, address, port):
        self.interface = interface
        self.address = address
        self.port = port
        self.sock = None
        self.running = True

    def create_socket(self):
        """Create and configure the multicast socket for receiving"""
        try:
            # Create UDP socket
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            
            # Allow address reuse
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            
            # Bind to the multicast port
            self.sock.bind(('', self.port))
            
            # Join multicast group
            mreq = struct.pack("4sl", socket.inet_aton(self.address), socket.INADDR_ANY)
            self.sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
            
            # Set interface if specified
            if self.interface:
                interface_ip = self._get_interface_ip(self.interface)
                if interface_ip:
                    self.sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, 
                                       socket.inet_aton(interface_ip))
                    print(f"Using interface {self.interface} with IP {interface_ip}")
                else:
                    print(f"Warning: Could not get IP for interface {self.interface}")
            
            print(f"Socket created for receiving from {self.address}:{self.port}")
            
        except Exception as e:
            print(f"Error creating socket: {e}")
            sys.exit(1)

    def _get_interface_ip(self, interface):
        """Get the IP address of a network interface"""
        try:
            import netifaces
            addrs = netifaces.ifaddresses(interface)
            if netifaces.AF_INET in addrs:
                return addrs[netifaces.AF_INET][0]['addr']
        except ImportError:
            # Fallback method using socket
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                s.connect(("8.8.8.8", 80))
                local_ip = s.getsockname()[0]
                s.close()
                return local_ip
            except:
                pass
        except:
            pass
        return None

    def receive_packets(self):
        """Receive multicast packets"""
        self.create_socket()
        
        print(f"Listening for packets on {self.address}:{self.port}")
        print("Press Ctrl+C to stop\n")
        
        packet_count = 0
        total_bytes = 0
        
        try:
            while self.running:
                try:
                    # Set timeout to allow checking self.running
                    self.sock.settimeout(1.0)
                    data, addr = self.sock.recvfrom(65536)
                    
                    packet_count += 1
                    total_bytes += len(data)
                    
                    # Parse packet data if it contains sequence info
                    if len(data) >= 16:
                        try:
                            seq_num, timestamp = struct.unpack('!QQ', data[:16])
                            current_time = int(time.time() * 1000000)
                            latency = current_time - timestamp
                            print(f"Packet {packet_count}: {len(data)} bytes from {addr[0]}:{addr[1]}, "
                                  f"seq={seq_num}, latency={latency}μs")
                        except:
                            print(f"Packet {packet_count}: {len(data)} bytes from {addr[0]}:{addr[1]}")
                    else:
                        print(f"Packet {packet_count}: {len(data)} bytes from {addr[0]}:{addr[1]}")
                        
                except socket.timeout:
                    continue
                except Exception as e:
                    if self.running:
                        print(f"Error receiving packet: {e}")
                    
        except KeyboardInterrupt:
            print("\nReceiving interrupted by user")
        finally:
            if self.sock:
                # Leave multicast group
                try:
                    mreq = struct.pack("4sl", socket.inet_aton(self.address), socket.INADDR_ANY)
                    self.sock.setsockopt(socket.IPPROTO_IP, socket.IP_DROP_MEMBERSHIP, mreq)
                except:
                    pass
                self.sock.close()
            print(f"\nReceiver finished. Total: {packet_count} packets, {total_bytes} bytes")

    def stop(self):
        """Stop the receiver"""
        self.running = False


def signal_handler(signum, frame):
    """Handle Ctrl+C gracefully"""
    print("\nShutting down...")
    sys.exit(0)


def main():
    parser = argparse.ArgumentParser(description='Multicast UDP Packet Sender/Receiver')
    subparsers = parser.add_subparsers(dest='mode', help='Operation mode')
    
    # Send command
    send_parser = subparsers.add_parser('send', help='Send multicast packets')
    send_parser.add_argument('--interface', '-i', help='Network interface (e.g., eth0, en0)')
    send_parser.add_argument('--address', '-a', required=True, help='Multicast address (e.g., 224.1.1.1)')
    send_parser.add_argument('--port', '-p', type=int, required=True, help='Multicast port')
    send_parser.add_argument('--size', '-s', type=int, default=1024, help='Packet size in bytes (default: 1024)')
    send_parser.add_argument('--count', '-c', type=int, default=10, help='Number of packets to send (default: 10)')
    send_parser.add_argument('--interval', '-t', type=float, default=1000.0, help='Send interval in milliseconds (default: 1000)')
    
    # Receive command
    receive_parser = subparsers.add_parser('receive', help='Receive multicast packets')
    receive_parser.add_argument('--interface', '-i', help='Network interface (e.g., eth0, en0)')
    receive_parser.add_argument('--address', '-a', required=True, help='Multicast address (e.g., 224.1.1.1)')
    receive_parser.add_argument('--port', '-p', type=int, required=True, help='Multicast port')
    
    args = parser.parse_args()
    
    if not args.mode:
        parser.print_help()
        sys.exit(1)
    
    # Validate multicast address
    try:
        socket.inet_aton(args.address)
        # Check if it's a valid multicast address (224.0.0.0 to 239.255.255.255)
        addr_parts = [int(x) for x in args.address.split('.')]
        if not (224 <= addr_parts[0] <= 239):
            print(f"Warning: {args.address} is not in the multicast range (224.0.0.0-239.255.255.255)")
    except socket.error:
        print(f"Error: Invalid IP address {args.address}")
        sys.exit(1)
    
    # Validate port
    if not (1 <= args.port <= 65535):
        print(f"Error: Port must be between 1 and 65535")
        sys.exit(1)
    
    # Set up signal handler
    signal.signal(signal.SIGINT, signal_handler)
    
    if args.mode == 'send':
        # Validate send parameters
        if args.size < 16:
            print("Warning: Packet size less than 16 bytes may not include sequence information")
        if args.count < 1:
            print("Error: Count must be at least 1")
            sys.exit(1)
        if args.interval < 0:
            print("Error: Interval must be non-negative")
            sys.exit(1)
            
        sender = MulticastUDPSender(args.interface, args.address, args.port, 
                                   args.size, args.count, args.interval)
        sender.send_packets()
        
    elif args.mode == 'receive':
        receiver = MulticastUDPReceiver(args.interface, args.address, args.port)
        receiver.receive_packets()


if __name__ == '__main__':
    main()

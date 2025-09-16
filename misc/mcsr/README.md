# Multicast UDP Packet Sender/Receiver (MCSR)

A versatile Python tool for sending and receiving multicast UDP packets with configurable parameters. Perfect for network testing, multicast validation, and tactical communication testing.

## What is MCSR?

MCSR (Multicast UDP Packet Sender/Receiver) is a Python utility that provides:
- **Multicast packet sending** with configurable timing and size
- **Multicast packet receiving** with latency measurement
- **Network interface binding** for specific network interfaces
- **Sequence tracking** and timestamp analysis
- **Real-time statistics** and monitoring

## Prerequisites

- Python 3.6 or higher
- Optional: `netifaces` library for enhanced interface detection

### Installation

```bash
# Optional: Install netifaces for better interface detection
pip install netifaces
```

## Usage

### Basic Syntax

```bash
python3 mcsr.py <mode> [options]
```

### Sending Multicast Packets

**Basic sending:**
```bash
python3 mcsr.py send --address 224.1.1.1 --port 5000
```

**Advanced sending with custom parameters:**
```bash
python3 mcsr.py send --interface eth0 --address 224.1.1.1 --port 5000 --size 1024 --count 100 --interval 100
```

**High-frequency sending:**
```bash
python3 mcsr.py send --address 224.1.1.1 --port 5000 --size 512 --count 1000 --interval 10
```

### Receiving Multicast Packets

**Basic receiving:**
```bash
python3 mcsr.py receive --address 224.1.1.1 --port 5000
```

**Receiving on specific interface:**
```bash
python3 mcsr.py receive --interface eth0 --address 224.1.1.1 --port 5000
```

## Command Line Options

### Send Mode Options
- **`--interface, -i`**: Network interface (e.g., eth0, en0)
- **`--address, -a`**: Multicast address (required, e.g., 224.1.1.1)
- **`--port, -p`**: Multicast port (required, 1-65535)
- **`--size, -s`**: Packet size in bytes (default: 1024)
- **`--count, -c`**: Number of packets to send (default: 10)
- **`--interval, -t`**: Send interval in milliseconds (default: 1000)

### Receive Mode Options
- **`--interface, -i`**: Network interface (e.g., eth0, en0)
- **`--address, -a`**: Multicast address (required, e.g., 224.1.1.1)
- **`--port, -p`**: Multicast port (required, 1-65535)

## Packet Format

### Sent Packets
Each packet contains:
- **Sequence number** (8 bytes, big-endian)
- **Timestamp** (8 bytes, microseconds since epoch)
- **Padding** (X characters to reach desired size)

### Received Packet Analysis
The receiver automatically:
- Parses sequence numbers and timestamps
- Calculates latency (receive time - send time)
- Displays packet statistics
- Tracks total bytes received

## Examples

### Network Testing

**Test multicast connectivity:**
```bash
# Terminal 1: Start receiver
python3 mcsr.py receive --address 224.1.1.1 --port 5000

# Terminal 2: Send test packets
python3 mcsr.py send --address 224.1.1.1 --port 5000 --count 10
```

**Load testing:**
```bash
# Send 1000 packets at 10ms intervals
python3 mcsr.py send --address 224.1.1.1 --port 5000 --count 1000 --interval 10
```

**Interface-specific testing:**
```bash
# Send on specific interface
python3 mcsr.py send --interface eth0 --address 224.1.1.1 --port 5000

# Receive on specific interface
python3 mcsr.py receive --interface eth0 --address 224.1.1.1 --port 5000
```

### Tactical Communication Testing

**Simulate tactical radio traffic:**
```bash
# Send tactical-sized packets
python3 mcsr.py send --address 224.1.1.1 --port 8513 --size 256 --count 50 --interval 500
```

**Test multicast reliability:**
```bash
# Send burst traffic
python3 mcsr.py send --address 224.1.1.1 --port 5000 --count 100 --interval 1
```

## Output Examples

### Sender Output
```
Using interface eth0 with IP 192.168.1.100
Socket created for sending to 224.1.1.1:5000
Sending 10 packets of 1024 bytes to 224.1.1.1:5000
Send interval: 1000 milliseconds
Press Ctrl+C to stop early

Sent packet 1/10 (1024 bytes)
Sent packet 2/10 (1024 bytes)
...
Sender finished
```

### Receiver Output
```
Using interface eth0 with IP 192.168.1.100
Socket created for receiving from 224.1.1.1:5000
Listening for packets on 224.1.1.1:5000
Press Ctrl+C to stop

Packet 1: 1024 bytes from 192.168.1.100:5000, seq=0, latency=1250μs
Packet 2: 1024 bytes from 192.168.1.100:5000, seq=1, latency=1180μs
...

Receiver finished. Total: 10 packets, 10240 bytes
```

## Use Cases

### Network Testing
- **Multicast connectivity validation**
- **Network performance testing**
- **Latency measurement**
- **Packet loss detection**

### Development and Debugging
- **Protocol testing**
- **Network stack validation**
- **Application testing**
- **Load testing**

### Tactical Systems
- **Tactical communication simulation**
- **Radio network testing**
- **Mission-critical system validation**
- **Network resilience testing**

## Performance Considerations

### Sending Performance
- **High-frequency sending**: Use small intervals (1ms) for burst testing
- **Large packets**: Increase packet size for bandwidth testing
- **Interface binding**: Specify interface for consistent performance

### Receiving Performance
- **Timeout handling**: 1-second timeout allows graceful shutdown
- **Memory usage**: Processes packets one at a time
- **Latency accuracy**: Microsecond precision for timing analysis

## Troubleshooting

### Common Issues

**"Could not get IP for interface"**
- Verify interface exists: `ip link show` or `ifconfig`
- Install netifaces: `pip install netifaces`
- Check interface is up and has IP address

**"Invalid IP address"**
- Ensure multicast address is in range 224.0.0.0-239.255.255.255
- Use valid IP format (e.g., 224.1.1.1)

**"Permission denied"**
- May need sudo for raw socket access on some systems
- Check firewall settings for multicast traffic

**No packets received**
- Verify multicast address and port match
- Check network connectivity
- Ensure firewall allows multicast traffic
- Verify interface is correct

### Debug Tips

**Test with simple multicast:**
```bash
# Use standard multicast address
python3 mcsr.py send --address 224.0.0.1 --port 5000 --count 1
```

**Check network interfaces:**
```bash
# List available interfaces
ip link show
# or
ifconfig
```

**Monitor network traffic:**
```bash
# Use tcpdump to monitor multicast traffic
sudo tcpdump -i eth0 -n host 224.1.1.1
```

**Test local loopback:**
```bash
# Send and receive on same machine
python3 mcsr.py send --address 224.1.1.1 --port 5000 --count 5
python3 mcsr.py receive --address 224.1.1.1 --port 5000
```

## Advanced Usage

### Scripting and Automation

**Bash script for automated testing:**
```bash
#!/bin/bash
# Start receiver in background
python3 mcsr.py receive --address 224.1.1.1 --port 5000 &
RECEIVER_PID=$!

# Wait a moment, then send packets
sleep 2
python3 mcsr.py send --address 224.1.1.1 --port 5000 --count 100

# Stop receiver
kill $RECEIVER_PID
```

**Python integration:**
```python
import subprocess
import time

# Start receiver
receiver = subprocess.Popen([
    'python3', 'mcsr.py', 'receive',
    '--address', '224.1.1.1', '--port', '5000'
])

# Send packets
subprocess.run([
    'python3', 'mcsr.py', 'send',
    '--address', '224.1.1.1', '--port', '5000',
    '--count', '10'
])

# Clean up
receiver.terminate()
```

### Integration with Other Tools

**With NSM testing:**
```bash
# Test NSM multicast communication
python3 mcsr.py send --address 255.255.255.255 --port 8513 --size 256 --count 50
```

**With network monitoring:**
```bash
# Send packets while monitoring with tcpdump
sudo tcpdump -i eth0 -n host 224.1.1.1 &
python3 mcsr.py send --address 224.1.1.1 --port 5000 --count 100
```

## Security Considerations

### Network Security
- **Multicast addresses**: Use appropriate multicast ranges
- **Firewall rules**: Configure firewall for multicast traffic
- **Interface binding**: Limit traffic to specific interfaces
- **Access control**: Ensure proper network segmentation

### Testing Security
- **Isolated networks**: Use test networks for testing
- **Controlled environments**: Avoid production networks
- **Traffic monitoring**: Monitor for unexpected traffic
- **Resource limits**: Set appropriate packet counts and intervals

## License

Copyright (c) 2025 Rally Tactical Systems, Inc.

## Support

For questions, issues, or contributions, please refer to the main project documentation or contact the development team.

Happy testing! 🚀

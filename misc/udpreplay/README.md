# UDP Replay Tool

A high-performance UDP packet replay tool based on Erik Rigtorp's original work, enhanced by Rally Tactical Systems with additional features for tactical network testing and simulation.

## What is UDP Replay?

UDP Replay is a command-line tool that reads UDP packets from PCAP files and replays them over the network with precise timing control. It's particularly useful for:

- **Network testing**: Replay captured traffic to test system behavior
- **Load testing**: Generate controlled network load
- **Protocol testing**: Test how systems handle specific packet sequences
- **Tactical simulation**: Replay tactical communication patterns

## Building

### Prerequisites

- C++ compiler with C++11 support
- CMake
- libpcap development libraries

### Build Instructions

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

```bash
udpreplay [options] pcap_file
```

### Basic Examples

**Replay a PCAP file at original speed:**
```bash
./udpreplay capture.pcap
```

**Replay at 2x speed:**
```bash
./udpreplay -s 2.0 capture.pcap
```

**Replay with 10ms intervals between packets:**
```bash
./udpreplay -c 10 capture.pcap
```

**Replay on specific interface:**
```bash
./udpreplay -i eth0 capture.pcap
```

**Redirect all packets to a different address:**
```bash
./udpreplay -a 192.168.1.100 capture.pcap
```

**Redirect all packets to a different port:**
```bash
./udpreplay -o 8080 capture.pcap
```

## Command Line Options

### Network Configuration
- **`-i iface`**: Interface to send packets through (e.g., `eth0`, `en0`)
- **`-l`**: Enable loopback (packets sent to local interface)
- **`-b`**: Enable broadcast (SO_BROADCAST socket option)

### Timing Control
- **`-s speed`**: Replay speed relative to PCAP timestamps (default: 1.0)
  - `0.5` = half speed
  - `2.0` = double speed
  - `0.1` = 10x slower
- **`-c millisec`**: Constant milliseconds between packets (overrides PCAP timing)
- **`-r repeat`**: Number of times to loop the data
  - `1` = play once (default)
  - `5` = play 5 times
  - `-1` = infinite loop

### Packet Filtering
- **`-f port`**: Filter by source port (only replay packets from this port)
- **`-g port`**: Filter by destination port (only replay packets to this port)

### Packet Modification
- **`-a addr`**: Override destination address (redirect all packets to this IP)
- **`-o port`**: Override destination port (redirect all packets to this port)
- **`-t ttl`**: Set packet TTL (Time To Live)

## Advanced Usage

### Testing Network Services

**Test a web server by replaying HTTP traffic:**
```bash
./udpreplay -a 192.168.1.10 -o 80 http_traffic.pcap
```

**Test with different timing patterns:**
```bash
# Burst traffic (10ms intervals)
./udpreplay -c 10 burst_traffic.pcap

# Slow, steady traffic (1 second intervals)
./udpreplay -c 1000 steady_traffic.pcap
```

### Load Testing

**Generate high load:**
```bash
./udpreplay -s 10.0 -r -1 high_load.pcap
```

**Test with port filtering:**
```bash
# Only replay DNS traffic
./udpreplay -g 53 dns_traffic.pcap

# Only replay traffic from port 12345
./udpreplay -f 12345 app_traffic.pcap
```

### Tactical Network Simulation

**Simulate tactical radio traffic:**
```bash
./udpreplay -i radio0 -t 16 -b tactical_comm.pcap
```

**Test multicast scenarios:**
```bash
./udpreplay -i eth0 -l multicast_traffic.pcap
```

## Output and Monitoring

The tool provides real-time feedback:

- **`.`** - Successfully sent packet
- **`h`** - Packet truncated (header.len != header.caplen)
- **`e`** - Non-IP packet (skipped)
- **`i`** - Non-IPv4 packet (skipped)
- **`u`** - Non-UDP packet (skipped)
- **`s`** - Source port filtered out
- **`d`** - Destination port filtered out

**Final statistics:**
```
sent 1250 packets (156250 bytes), filtered 45 packets (5625 bytes)
```

## Performance Considerations

- **High-speed replay**: Use `-s` with values > 1.0 for faster than real-time replay
- **Low-latency**: The tool uses high-precision timing for accurate packet spacing
- **Memory efficient**: Processes packets one at a time without loading entire PCAP into memory
- **CPU usage**: Minimal CPU overhead, suitable for continuous operation

## Troubleshooting

### Common Issues

**"if_nametoindex: No such device"**
- Check interface name with `ip link show` or `ifconfig`
- Ensure interface exists and is up

**"socket: Permission denied"**
- Run with appropriate permissions
- On Linux, may need `sudo` for raw socket access

**"pcap_open: No such file"**
- Verify PCAP file exists and is readable
- Check file path and permissions

**Packets not appearing on network**
- Verify interface is correct (`-i` option)
- Check if loopback is needed (`-l` option)
- Ensure destination address/port are reachable

### Debug Tips

**Enable verbose output:**
```bash
# Monitor with tcpdump on another terminal
sudo tcpdump -i eth0 -n

# Then run udpreplay
./udpreplay -i eth0 test.pcap
```

**Test with small files first:**
```bash
# Create a small test PCAP
tcpdump -c 10 -w test.pcap

# Replay it
./udpreplay test.pcap
```

## Use Cases

### Network Testing
- Replay captured traffic to test system resilience
- Generate predictable load patterns
- Test protocol implementations

### Development
- Test applications with realistic network conditions
- Debug network protocol issues
- Validate system behavior under load

### Tactical Systems
- Simulate tactical communication patterns
- Test radio network protocols
- Validate system performance under various conditions

## License

This software is based on Erik Rigtorp's original work (© 2020 Erik Rigtorp <erik@rigtorp.se>, MIT License) with enhancements by Rally Tactical Systems, Inc.

## Support

For questions, issues, or contributions, please refer to the main project documentation or contact the development team.

Happy testing! 🚀

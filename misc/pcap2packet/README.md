# PCAP to Packet Converter

A utility tool for extracting UDP packet payloads from PCAP files and converting them to a custom packet format. This tool is part of the Rally Tactical Systems toolkit for network analysis and packet processing.

## What is PCAP to Packet?

PCAP to Packet is a command-line tool that:
- Reads UDP packets from PCAP files
- Extracts the UDP payload data
- Writes the packet data to a custom packet file format
- Filters packets based on various criteria
- Provides statistics on processed packets

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
pcap2packet pcap_file packet_file
```

### Basic Examples

**Extract all UDP packets from a PCAP file:**
```bash
./pcap2packet capture.pcap output.packet
```

**Process a large PCAP file:**
```bash
./pcap2packet large_capture.pcap processed.packet
```

## Input and Output

### Input: PCAP Files
- Standard PCAP format files
- Must contain UDP packets
- Supports both live capture and offline files

### Output: Packet Files
- Custom binary format containing UDP payloads
- Sequential packet data without headers
- Optimized for further processing

## Packet Processing

The tool processes packets in the following order:

1. **Ethernet Layer**: Skips VLAN tags, processes only IP packets
2. **IP Layer**: Filters for IPv4 packets only
3. **UDP Layer**: Extracts UDP payload data
4. **Payload**: Writes raw UDP data to output file

### Supported Packet Types
- IPv4 UDP packets
- Packets with VLAN tags (automatically skipped)
- Standard Ethernet frames

### Filtered Packets
The tool skips and reports:
- **`h`** - Truncated packets (header.len != header.caplen)
- **`e`** - Non-IP packets
- **`i`** - Non-IPv4 packets  
- **`u`** - Non-UDP packets

## Output Format

The output packet file contains:
- Raw UDP payload data
- Sequential packet data
- No packet headers or metadata
- Binary format for efficient processing

## Statistics

The tool provides processing statistics:
```
sent 1250 packets (156250 bytes), filtered 45 packets (5625 bytes)
```

Where:
- **sent**: Number of UDP packets successfully processed
- **bytes**: Total bytes of UDP payload data extracted
- **filtered**: Number of packets skipped due to filtering
- **bytes filtered**: Total bytes in filtered packets

## Use Cases

### Network Analysis
- Extract application data from network captures
- Prepare packet data for custom analysis tools
- Convert PCAP data for specialized processing

### Protocol Development
- Extract payload data for protocol testing
- Prepare test data for application development
- Convert network captures to application-specific formats

### Tactical Systems
- Process tactical communication captures
- Extract mission-critical data from network traffic
- Prepare data for tactical analysis tools

## Performance Considerations

- **Memory efficient**: Processes packets sequentially
- **Fast processing**: Minimal overhead for UDP extraction
- **Large file support**: Can handle large PCAP files
- **Error handling**: Gracefully handles malformed packets

## Troubleshooting

### Common Issues

**"pcap_open: No such file"**
- Verify PCAP file exists and is readable
- Check file path and permissions

**"No packets processed"**
- Ensure PCAP file contains UDP packets
- Check if packets are IPv4 (IPv6 not supported)
- Verify packets are not filtered out

**Empty output file**
- Check if PCAP contains valid UDP traffic
- Verify file permissions for output directory

### Debug Tips

**Check PCAP contents first:**
```bash
# Examine PCAP file
tcpdump -r capture.pcap -n

# Count UDP packets
tcpdump -r capture.pcap -n | grep UDP | wc -l
```

**Test with small files:**
```bash
# Create a small test PCAP
tcpdump -c 10 -w test.pcap

# Process it
./pcap2packet test.pcap test.packet
```

## Technical Details

### Packet Processing Flow
1. Open PCAP file with nanosecond timestamp precision
2. Iterate through each packet
3. Parse Ethernet header and skip VLAN tags
4. Validate IPv4 and UDP protocols
5. Extract UDP payload data
6. Write payload to output file
7. Generate processing statistics

### Supported Platforms
- Linux (GLIBC)
- macOS (Mach)
- Other Unix-like systems

### Dependencies
- libpcap for PCAP file handling
- Standard C++ libraries
- POSIX-compliant system calls

## Integration

This tool is designed to work with other Rally Tactical Systems utilities:
- **udpreplay**: Can replay the extracted packet data
- **NSM**: Can process tactical communication patterns
- **Custom tools**: Output format suitable for further processing

## License

This software is based on Erik Rigtorp's original work (© 2020 Erik Rigtorp <erik@rigtorp.se>, MIT License) with enhancements by Rally Tactical Systems, Inc.

## Support

For questions, issues, or contributions, please refer to the main project documentation or contact the development team.

Happy processing! 🚀

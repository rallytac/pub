# Network State Manager (NSM)

Welcome to the Network State Manager (NSM)! This is a distributed state management system designed to coordinate network resources across multiple instances using a leader election protocol. Think of it as a smart coordinator that ensures only one instance is "active" at any given time, with automatic failover when needed.

## What is NSM?

NSM is a Python-based system that helps multiple network instances work together by:
- **Electing leaders per resource** - Each resource can have a different active instance
- **Automatic failover** - If the active instance fails, another takes over
- **Resource coordination** - Manages multiple resources with different priorities
- **Network resilience** - Handles network failures gracefully

**Important**: Each resource is managed independently! If Instance A wins the election for Resource 1, it doesn't guarantee that Instance A will also win Resource 2. Different instances can be active for different resources simultaneously, allowing for distributed load balancing and specialized resource management.

Perfect for scenarios where you need high availability and can't afford to have multiple instances conflicting with each other.

## Quick Start

### 1. Prerequisites

Make sure you have Python 3 installed with these optional dependencies:
```bash
pip install cryptography colorama
```

### 2. Configuration

Start by copying and customizing the configuration file:
```bash
cp nsm_conf.json my_nsm_conf.json
```

Edit `my_nsm_conf.json` to match your environment:
- **Interface**: Set `interfaceName` to your network interface (e.g., "eth0", "en0")
- **Network**: Configure `address` and `port` for multicast communication
- **Resources**: Define your resources and their priorities
- **Scripts**: Point to your custom state transition scripts

### 3. Run NSM

**Basic usage:**
```bash
./nsm.sh
```

**With custom configuration:**
```bash
python3 _nsm.py --config-file my_nsm_conf.json
```

**With custom instance ID:**
```bash
python3 _nsm.py --id my-instance-01
```

## Configuration Guide

### Network Settings
```json
"networking": {
    "interfaceName": "en0",           // Your network interface
    "address": "255.255.255.255",     // Multicast address
    "port": 8513,                     // Port number
    "ttl": 64,                        // Time to live
    "cryptoPassword": "your-password" // Encryption key
}
```

### Resources
```json
"resources": [
    {
        "id": "res01",
        "priority": 100               // Higher = more important
    }
]
```

### State Transition Scripts
```json
"run": {
    "onIdle": "./onIdle.sh",
    "beforeGoingActive": "./beforeGoingActive.sh",
    "onGoingActive": "./onGoingActive.sh",
    "beforeActive": "./beforeActive.sh",
    "onActive": "./onActive.sh"
}
```

### Timing
```json
"timing": {
    "internalMultiplier": 1,          // Speed multiplier
    "txIntervalSecs": 1,              // Transmission interval
    "transitionWaitSecs": 3           // State transition delay
}
```

## State Machine

NSM operates with four states **per resource**:

1. **NONE** - Initial state
2. **IDLE** - Waiting, not active for this resource
3. **GOING_ACTIVE** - Transitioning to active (election phase) for this resource
4. **ACTIVE** - Currently leading/active for this resource

### State Transitions

- **IDLE → GOING_ACTIVE**: When no other instance is active for this specific resource
- **GOING_ACTIVE → ACTIVE**: After winning the election for this resource
- **ACTIVE → IDLE**: When another instance takes over this resource or fails
- **Any → IDLE**: On network errors or timeouts

**Key Point**: Each resource has its own independent state machine. Instance A might be ACTIVE for Resource 1 while Instance B is ACTIVE for Resource 2, and Instance C might be IDLE for both resources.

### Network Bandwidth Utilization

NSM is designed to minimize network traffic:
- **Transmitting instances**: Only instances that are GOING_ACTIVE or ACTIVE for one or more resources will transmit network traffic
- **Listening instances**: Instances that are IDLE for all resources only listen for traffic from other instances
- **Efficient communication**: This design reduces network congestion and bandwidth usage, especially in scenarios with many instances where only a few need to be active
- **Bandwidth usage**: For a single resource, network bandwidth is approximately 0.5 kbps. The more resources an instance is responsible for, the larger the data packets will be and, therefore, bandwidth usage will increase proportionally
- **Multiple active instances**: If multiple instances are active (for example, when multiple resources are spread across multiple instances), network bandwidth utilization will also increase as each active instance transmits its own traffic

## Customizing State Transitions

Each state transition can trigger custom scripts:

### onIdle.sh
Called when transitioning to idle state:
```bash
#!/bin/bash
echo "Instance is now idle"
# Your idle state logic here
```

### beforeGoingActive.sh
Determines the election token range (higher tokens win):
```bash
#!/bin/bash
# Default range
echo "1000000-2000000"

# Custom range for strategic advantage
# echo "3000000-3500000"
```

### onGoingActive.sh
Called when entering the election phase:
```bash
#!/bin/bash
echo "Instance is going active - participating in election"
# Your election logic here
```

### beforeActive.sh
Final confirmation before becoming active:
```bash
#!/bin/bash
# Return "1" to confirm, anything else to abort
echo "1"
```

### onActive.sh
Called when becoming the active instance:
```bash
#!/bin/bash
echo "Instance is now ACTIVE - taking leadership"
# Your active state logic here
```

## Command Line Options

```bash
python3 _nsm.py [options]

Options:
  --config-file FILE     Use custom config file (default: nsm_conf.json)
  --id ID               Set instance ID
  --log-level LEVEL     Set logging level (0-5)
  --fixed-token TOKEN   Use fixed election token
  --max-run-secs SECS   Run for limited time (testing)
  --dashboard-token     Show election tokens in dashboard
```

## Monitoring and Logging

### Dashboard
Enable the real-time dashboard in your config:
```json
"logging": {
    "level": 4,
    "dashboard": true
}
```

### Log Levels
- **0 (FATAL)**: Critical errors only
- **1 (ERROR)**: Error messages
- **2 (WARN)**: Warning messages
- **3 (INFO)**: General information
- **4 (DEBUG)**: Detailed debugging
- **5 (CHATTY)**: Verbose output

### Network Statistics
NSM tracks:
- Packets sent/received
- Bytes transmitted
- Network errors
- State transitions

## Testing and Development

### Test Mode
Run with limited time for testing:
```bash
python3 _nsm.py --max-run-secs 60
```

### Multiple Instances
Run multiple instances on different machines:
```bash
# Instance 1
python3 _nsm.py --id instance-01

# Instance 2  
python3 _nsm.py --id instance-02
```

**Resource Distribution**: Each instance will independently compete for each resource. You might end up with:
- Instance 1: ACTIVE for Resource A, IDLE for Resource B
- Instance 2: IDLE for Resource A, ACTIVE for Resource B
- Instance 3: IDLE for both resources (backup)

### Network Simulation
Configure network impairments in your config:
```json
"networking": {
    "rxLossPercentage": 5,      // 5% packet loss
    "txLossPercentage": 2,      // 2% transmission loss
    "rxErrorPercentage": 1      // 1% receive errors
}
```

## Troubleshooting

### Common Issues

**"waiting to determine IP address"**
- Check your `interfaceName` in the config
- Ensure the interface is up and has an IP address

**"Network error"**
- Verify multicast address and port
- Check firewall settings
- Ensure network connectivity

**"No state transitions"**
- Check your state transition scripts are executable
- Verify script paths in configuration
- Review log levels for more details

### Debug Mode
Run with maximum logging:
```bash
python3 _nsm.py --log-level 5
```

## Best Practices

1. **Unique Instance IDs**: Use descriptive, unique IDs for each instance
2. **Network Security**: Use strong encryption passwords
3. **Script Testing**: Test your state transition scripts independently
4. **Monitoring**: Enable dashboard for production monitoring
5. **Resource Priorities**: Set appropriate priorities for your resources
6. **Timing Tuning**: Adjust timing parameters based on your network latency

## Examples

### Basic High Availability Setup
```json
{
    "id": "web-server-01",
    "networking": {
        "interfaceName": "eth0",
        "address": "239.1.1.1",
        "port": 8513,
        "cryptoPassword": "secure-password-123"
    },
    "resources": [
        {"id": "web-service", "priority": 100}
    ],
    "run": {
        "onActive": "./start-web-service.sh",
        "onIdle": "./stop-web-service.sh"
    }
}
```

### Load Balancer Coordination
```json
{
    "id": "lb-primary",
    "resources": [
        {"id": "primary-lb", "priority": 200},
        {"id": "backup-lb", "priority": 100}
    ]
}
```

## Customization and Modification

NSM is designed to be flexible and adaptable to your specific needs. You are free to modify the software as needed to fit your use cases. This includes:

- **Custom state transition scripts** - Write your own logic for each state
- **Configuration modifications** - Adjust timing, networking, and resource parameters
- **Code modifications** - Modify `_nsm.py` to add new features or behaviors
- **Integration** - Integrate NSM with your existing systems and workflows

Feel free to fork, modify, and adapt NSM to work exactly how you need it to work!

## Support

For questions, issues, or contributions, please refer to the main project documentation or contact the development team.

Happy coordinating! 🚀

---

## Disclaimer

**IMPORTANT LEGAL NOTICE**

This software is provided "as is" without warranty of any kind, either express or implied, including but not limited to the implied warranties of merchantability and fitness for a particular purpose.

Rally Tactical Systems, Inc. makes no representations or warranties regarding the suitability of this software for any particular task, performance characteristics, reliability, or any other situation. The use or misuse of NSM may cause harm, damage, or other consequences for which Rally Tactical Systems, Inc. shall not be liable.

By using this software, you acknowledge and agree that:
- You use NSM at your own risk
- Rally Tactical Systems, Inc. disclaims all liability for any damages, losses, or consequences arising from the use of this software
- You are responsible for ensuring the software is suitable for your intended use
- You are responsible for testing and validating the software in your environment
- You are responsible for compliance with all applicable laws and regulations

This disclaimer applies to the fullest extent permitted by law.

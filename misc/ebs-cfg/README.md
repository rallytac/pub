#  Engage Bridging Service Configuration Tool v0.1

An interactive menu-driven tool to modify Engage Bridging Service (EBS) configuration files.

**Copyright (c) 2025 Rally Tactical Systems Inc**

## Overview

This tool provides an easy-to-use interactive interface for managing EBS configuration files:
- `engagebridged_conf.json` - Main EBS service configuration
- `bridges.json` - Bridging configuration (defines which groups are bridged together)

## Features

- **No external dependencies** - Uses only Python standard library
- **Interactive menus** - Simple navigation with numbered options
- **Guided workflows** - Step-by-step prompts when adding/editing configurations
- **Bridge wizards** - Automated wizards for common bridge creation tasks
- **First-time setup** - Automatic initialization of EBS ID and license from file
- **Edit existing items** - Modify groups and bridges without recreating them
- **Safe editing** - Validates input and confirms destructive operations
- **Automatic backups** - Creates temporary backups when changes are made
- **JSON validation** - Validates JSON structure before saving to prevent corruption
- **Context-aware displays** - Shows current values and lists automatically
- **Help system** - Type `?` for field-specific help on group types and audio encoders
- **Graceful interruption** - Handles Ctrl+C with options to revert changes or exit
- **Network interface detection** - Automatically detects and suggests available network interfaces

## Usage

### Running the Tool

The tool supports flexible command-line arguments:

**No arguments** - Uses default filenames in current directory:
```bash
./ebs_config_tool.py
# Looks for: engagebridged_conf.json and bridges.json in current directory
```

**One argument (directory path)** - Uses default filenames in specified directory:
```bash
./ebs_config_tool.py /path/to/config/directory
# Looks for: /path/to/config/directory/engagebridged_conf.json
#           /path/to/config/directory/bridges.json
```

**One argument (file path)** - Uses specified file as main config, looks for bridges.json in same directory:
```bash
./ebs_config_tool.py /path/to/engagebridged_conf.json
# Uses: /path/to/engagebridged_conf.json
#      /path/to/bridges.json
```

**Two arguments** - Specify both config files explicitly:
```bash
./ebs_config_tool.py path/to/engagebridged_conf.json path/to/bridges.json
```

**Help flag** - Display usage information:
```bash
./ebs_config_tool.py --help
# or
./ebs_config_tool.py -h
```

**Note:** If config files are not found, the tool will automatically display usage information to help you specify the correct paths.

### Main Application Menu

When you run the tool, you start directly in the main **Engage Bridging Service Configuration** menu, which provides access to all functionality:

- **Bridges management** - Create, edit, and remove bridges and groups
- **Bridge wizards** - Automated setup for common configurations
- **Service Configuration** - Access EBS service settings
- **Quit** - Exit the tool (press 'q' from any menu)

### Service Configuration Menu

When you select "Service Configuration" (option 7), you can configure:

1. **Update License from File**
   - Update license and featureset information from a JSON file
   - Displays current licensing information in formatted JSON before prompting for new file
   - Prompts for file path to license JSON file
   - Validates and merges license data into configuration
   - **Note:** Similar to first-time setup license initialization

2. **General Settings**
   - EBS ID
   - Mode
   - Configuration file check interval
   - Certificate store file name
   - **Note:** Shows current values before editing

3. **Status Report Settings**
   - Enable/disable status reporting
   - Configure report file location and interval
   - Set detail inclusion options
   - **Note:** Shows current values before editing

4. **Health Check Responder**
   - Configure TCP port for health checks
   - Set connection behavior
   - **Note:** Shows current values before editing

5. **Multicast TX Options**
   - Set priority and TTL
   - **Note:** Shows current values before editing

6. **TX Options**
   - Set priority and TTL
   - **Note:** Shows current values before editing

7. **Show Main Config** - Display the current main configuration file in a colorized JSON format

**Back** (or press 'b' or Esc)
**Quit** (or press 'q')

**Important:** Any attributes or objects starting with `_` (e.g., `_statusReport`, `_security`) are preserved in the JSON but are not editable by the tool. These are typically used for internal system state.

### Engage Bridging Service Configuration Menu

The main application menu displays:

- **Bridges Overview** - A table showing each bridge ID and its groups
  - Manually created bridges are marked with a `*` indicator (only shown if there are manually created bridges)
  - Wizard-created bridges have no indicator
  - If there are manually created bridges, a legend "* = Manually created" appears at the bottom

Then the menu options:
1. **Raw Bridge (Multicast to Rallypoint)** - Wizard to create a raw bridge connecting multicast and rallypoint groups
2. **Audio Bridge (Multicast to Rallypoint)** - Wizard to create an audio bridge with txAudio configuration
3. **TSM Bridge (TSM Radio to Rallypoint)** - Wizard to create a bridge with TSM payload transformations
4. **MPU Bridge (MPU Radio to Rallypoint)** - Wizard to create a bridge with MPU payload transformations
5. **Remove (enter number)** - Delete an existing bridge by its number
6. **Edit Bridges & Groups** - Access detailed editing menus for groups and bridges
7. **Service Configuration** - Access EBS service settings
8. **Quit** (or press 'q')

#### Edit Bridges & Groups Submenu

When you select "Edit Bridges & Groups" (option 6), you'll see a unified interface with numbered entries:

- **Bridges Table** - Shows all bridges with their groups, numbered starting from 1
- **Groups Table** - Shows all groups with detailed information, numbered continuing from where bridges end

**Example Display:**
```
Bridges:
#   ID              Groups          Info
--- --------------- --------------- -------------------------
1   raw-bridge-1    enterprise-1    MC: 239.1.1.1:7201, Enc: None
                    trunk-1         RP: 10.0.0.1:7443, Enc: None

Groups:
#   ID              Name            Type    Encoder         Multicast   Rallypoint  Used In
--- --------------- --------------- ------- --------------- ----------- ----------- -------
2   enterprise-1    Enterprise      1 (Audio) 25 (G.711)   Yes         No          raw-bridge-1
3   trunk-1         Trunk           1 (Audio) 25 (G.711)   No          Yes         raw-bridge-1
```

Then the menu options:
1. **Edit (enter number)** - Edit any bridge or group by its display number
2. **Add (Bridge or Group)** - Add a new bridge or group (prompts for type)
3. **Remove (enter number)** - Remove any bridge or group by its display number
4. **Show JSON** - Display the bridges.json file in a colorized JSON format
**Back** (or press 'b' or Esc) - Return to main menu

**Using the Unified Interface:**

- **To Edit:** Select option 1, then enter the number (e.g., `2` to edit enterprise-1 group)
- **To Add:** Select option 2, then choose Bridge or Group
- **To Remove:** Select option 3, then enter the number (e.g., `1` to remove raw-bridge-1)

The system automatically determines whether you're working with a bridge or group based on the number you select.

## Safety Features

### Automatic Backups

- Temporary backups are created automatically when you make your first change
- Backups are timestamped (e.g., `engagebridged_conf.backup_20251123_094314.json`)
- Backups are automatically deleted when you exit successfully
- If you exit with Ctrl+C and have unsaved changes, you can choose to revert from backup

### JSON Validation

- All JSON is validated before saving to prevent corrupted files
- If validation fails, the file is not modified and an error is displayed

### Ctrl+C Handling

When you press Ctrl+C:

- **If no changes made:** Prompts "Do you want to exit? [Y/n]:" (defaults to Yes)
- **If changes made:** Shows options:
  1. Revert all changes and exit
  2. Exit without reverting
  3. Continue editing

The default for exit prompts is **Yes**, so pressing Enter will exit.

## First-Time Setup

When you run the tool for the first time with a new or incomplete configuration, it will automatically:

1. **Welcome Screen** - Display a welcome message if EBS ID, license, or featureset is missing
2. **EBS ID Setup** - Prompt you to enter an EBS ID if it's missing
3. **License Initialization** - Prompt you to provide a license file (JSON format) containing:
   - `license` or `licensing` object with entitlement, key, and activation code
   - `featureset` object with signature and features
   
   You can:
   - Provide the path to a license file (e.g., `license.json`)
   - Type `skip` to continue without a license (you can add it later)

The license file will be automatically loaded and merged into the configuration. The featureset is read-only and cannot be edited through the tool interface.

## Bridge Wizards

The tool includes wizards to simplify common bridge creation tasks:

### Raw Bridge (Multicast to Rallypoint) Wizard

This wizard creates a complete bridge setup for connecting a multicast group (trunk side) to a rallypoint group (enterprise side) using Raw groups (type 3):

1. **Bridge Name** - Enter a descriptive name (e.g., "Enterprise Bridge 1")
   - The tool automatically generates a valid bridge ID from the name
   
2. **Stream ID** - Enter the stream ID for the enterprise/rallypoint side
   - The tool automatically creates:
     - Enterprise group: Uses the stream ID you provide
     - Trunk group: Uses `{stream_id}-trunk` as the ID
   
3. **Enterprise Side (Rallypoint)** - Configure:
   - Rallypoint host address and port
   - Network interface (optional, can be auto-detected)
   
4. **Trunk Side (Multicast)** - Configure:
   - Multicast RX address and port
   - Multicast TX address and port
   - TTL settings
   - Network interface (optional, can be auto-detected)

5. **Audio Settings** - Configure encoder and audio parameters for both groups

The wizard automatically:
- Creates both groups (Raw type)
- Creates the bridge connecting them
- Marks the bridge as wizard-created (no indicator shown)
- Validates all inputs before creating

### Audio Bridge (Multicast to Rallypoint) Wizard

This wizard creates a complete bridge setup for connecting a multicast group (trunk side) to a rallypoint group (enterprise side) using Audio groups (type 1) with txAudio configuration:

1. **Bridge Name** - Enter a descriptive name (e.g., "Audio Bridge 1")
   - The tool automatically generates a valid bridge ID from the name
   
2. **Stream ID** - Enter the stream ID for the enterprise/rallypoint side
   - The tool automatically creates:
     - Enterprise group: Uses the stream ID you provide
     - Trunk group: Uses `{stream_id}-trunk` as the ID
   
3. **Enterprise Side (Rallypoint)** - Configure:
   - Rallypoint host address and port
   - **Audio Configuration (Enterprise Group)** - Configure separately:
     - Audio encoder (default: 25 - Opus 16 kbit/s)
     - Full duplex setting
     - Framing (ms)
     - Max transmit seconds
     - Header extension options
   
4. **Trunk Side (Multicast)** - Configure:
   - Multicast RX address and port
   - Multicast TX address and port
   - TTL settings
   - Network interface (optional, can be auto-detected)
   - **Audio Configuration (Trunk Group)** - Configure separately:
     - Audio encoder (no default - must be specified)
     - Full duplex setting
     - Framing (ms)
     - Max transmit seconds
     - Header extension options (defaults to True for trunk side)

The wizard automatically:
- Creates both groups (Audio type with txAudio configuration)
- Creates the bridge connecting them
- Marks the bridge as wizard-created (no indicator shown)
- Validates all inputs before creating
- Prompts for txAudio settings separately for each group
- Sets `noHdrExt` to `True` by default for the trunk group

Wizard-created bridges can be edited or removed like any other bridge.

### TSM Bridge (TSM Radio to Rallypoint) Wizard

This wizard creates a complete bridge setup for connecting a TSM radio system (trunk side) to a rallypoint group (enterprise side) using Audio groups (type 1) with automatic TSM payload transformations:

1. **Bridge Name** - Enter a descriptive name (e.g., "TSM Bridge 1")
   - The tool automatically generates a valid bridge ID from the name
   
2. **Stream ID** - Enter the stream ID for the enterprise/rallypoint side
   - The tool automatically creates:
     - Enterprise group: Uses the stream ID you provide
     - Trunk group: Uses `{stream_id}-trunk` as the ID
   
3. **Enterprise Side (Rallypoint)** - Configure:
   - Rallypoint host address and port
   - **Audio Configuration (Enterprise Group)** - Configure separately:
     - Audio encoder (default: 25 - Opus 16 kbit/s)
     - Full duplex setting
     - Framing (ms)
     - Max transmit seconds
     - Header extension options
   
4. **Trunk Side (Multicast)** - Configure:
   - Multicast RX address and port
   - Multicast TX address and port
   - TTL settings
   - Network interface (optional, can be auto-detected)
   - **Audio Configuration (Trunk Group)** - Uses same encoder as enterprise group:
     - Full duplex setting
     - Framing (ms)
     - Max transmit seconds
     - Header extension options (defaults to True for trunk side)

The wizard automatically:
- Creates both groups (Audio type with txAudio configuration)
- Applies TSM payload transformations based on selected encoder
- Creates the bridge connecting them
- Marks the bridge as wizard-created (no indicator shown)
- Validates all inputs before creating
- Configures `customRtpPayloadType` and `inboundRtpPayloadTypeTranslations` for TSM compatibility

### MPU Bridge (MPU Radio to Rallypoint) Wizard

This wizard creates a complete bridge setup for connecting an MPU radio system (trunk side) to a rallypoint group (enterprise side) using Audio groups (type 1) with automatic MPU payload transformations:

1. **Bridge Name** - Enter a descriptive name (e.g., "MPU Bridge 1")
   - The tool automatically generates a valid bridge ID from the name
   
2. **Stream ID** - Enter the stream ID for the enterprise/rallypoint side
   - The tool automatically creates:
     - Enterprise group: Uses the stream ID you provide
     - Trunk group: Uses `{stream_id}-trunk` as the ID
   
3. **Enterprise Side (Rallypoint)** - Configure:
   - Rallypoint host address and port
   - **Audio Configuration (Enterprise Group)** - Configure separately:
     - Audio encoder (default: 25 - Opus 16 kbit/s)
     - Full duplex setting
     - Framing (ms)
     - Max transmit seconds
     - Header extension options
   
4. **Trunk Side (Multicast)** - Configure:
   - Multicast RX address and port
   - Multicast TX address and port
   - TTL settings
   - Network interface (optional, can be auto-detected)
   - **Audio Configuration (Trunk Group)** - Uses same encoder as enterprise group:
     - Full duplex setting
     - Framing (ms)
     - Max transmit seconds
     - Header extension options (defaults to True for trunk side)

The wizard automatically:
- Creates both groups (Audio type with txAudio configuration)
- Applies MPU payload transformations based on selected encoder
- Creates the bridge connecting them
- Marks the bridge as wizard-created (no indicator shown)
- Validates all inputs before creating
- Configures `customRtpPayloadType` and `inboundRtpPayloadTypeTranslations` for MPU compatibility

### Payload Transformations

When creating or editing Audio groups manually, you can configure payload transformations for interfacing with different radio systems:

1. **Automatic Configuration** - Select from available radio types:
   - **TSM**: Tactical Secure Messaging systems
   - **MPU**: Multi-Purpose Unit radios
   - **Engage**: Standard Engage systems (no transformations)

2. **Manual Configuration** - Enter custom payload types:
   - Custom RTP payload type (96-127)
   - Engage RTP payload type for translation

3. **Configuration Applied**:
   - `customRtpPayloadType` added to `txAudio` object
   - `inboundRtpPayloadTypeTranslations` array configured for payload mapping

**Example TSM Configuration (MELPe @ 2.4kbps):**
```json
"txAudio": {
    "encoder": 52,
    "customRtpPayloadType": 77
},
"inboundRtpPayloadTypeTranslations": [
    {
        "external": 77,
        "engage": 77
    }
]
```

**Example MPU Configuration (MELPe @ 2.4kbps):**
```json
"txAudio": {
    "encoder": 52,
    "customRtpPayloadType": 117
},
"inboundRtpPayloadTypeTranslations": [
    {
        "external": 117,
        "engage": 77
    }
]
```

## Examples

### Adding a Multicast Group

1. Select "Edit Bridges & Groups" from main menu (option 6)
2. Select "Add (Bridge or Group)" (option 2)
3. Select "Group"
4. Enter group ID: `my-group`
5. Enter group name: `My Group`
6. Enter group type: `1` (or see options with `?`)
7. Choose "Use multicast?" → Yes
8. Enter multicast address: `239.0.0.191`
9. Enter port: `1235`
10. Configure audio settings as prompted
11. Group is saved automatically

### Editing an Existing Group

1. Select "Edit Bridges & Groups" from main menu (option 6)
2. Select "Edit (enter number)" (option 1)
3. Enter the number of the group to edit
5. Modify any fields (all pre-populated with current values)
   - Group type shows current value: `Group type (0-3, ? for help) [1 - Audio]:`
   - Audio encoder shows current value: `Audio encoder (? for help) [25 - Opus 16 (kbit/s)]:`
6. Changes are saved automatically

### Creating a Bridge

**Option 1: Using a Wizard**
- **Raw Bridge Wizard**: Select "Raw Bridge (Multicast to Rallypoint)" (option 1)
- **Audio Bridge Wizard**: Select "Audio Bridge (Multicast to Rallypoint)" (option 2)
- **TSM Bridge Wizard**: Select "TSM Bridge (TSM Radio to Rallypoint)" (option 3)
- **MPU Bridge Wizard**: Select "MPU Bridge (MPU Radio to Rallypoint)" (option 4)
- Follow the wizard prompts

**Option 2: Manual Creation**
1. Select "Edit Bridges & Groups" from main menu (option 6)
2. Select "Add (Bridge or Group)" (option 2)
3. Select "Bridge"
4. Enter bridge ID: `Bridge1`
5. View available groups and select numbers (e.g., `1,2`)
6. Bridge is created automatically

### Editing an Existing Bridge

1. Select "Edit Bridges & Groups" from main menu (option 6)
2. Select "Edit (enter number)" (option 1)
3. Enter the number of the bridge to edit
5. Modify bridge ID or select different groups
   - Group selection prompt is pre-filled: `Groups [1,2]: 1,2`
   - Edit the values or press Enter to keep current groups
6. Changes are saved automatically

### Enabling Status Reporting

1. Select "Service Configuration" from main menu (option 7)
2. Select "Status Report Settings" (option 3)
3. Current values are displayed first
4. Choose to edit → Yes
5. Choose "Enable status report" → Yes
6. Configure file name, interval, and detail options
7. Settings are saved automatically

### Updating License Information

1. Select "Service Configuration" from main menu (option 7)
2. Select "Update License from File" (option 1)
3. Current licensing information is displayed in formatted JSON
4. Enter the path to your license JSON file
5. License and featureset are validated and merged into configuration
6. Changes are saved automatically

### Using the Raw Bridge Wizard

1. Select "Raw Bridge (Multicast to Rallypoint)" from main menu (option 1)
3. Enter a bridge name (e.g., "Enterprise Trunk 1")
4. Enter stream ID for enterprise side
5. Configure rallypoint host (address and port)
6. Configure multicast settings (RX/TX addresses and ports)
7. Bridge, groups, and connections are created automatically

### Using the Audio Bridge Wizard

1. Select "Audio Bridge (Multicast to Rallypoint)" from main menu (option 2)
3. Enter a bridge name (e.g., "Audio Trunk 1")
4. Enter stream ID for enterprise side
5. Configure rallypoint host (address and port)
6. Configure audio settings for enterprise group (encoder, full duplex, framing, etc.)
7. Configure multicast settings (RX/TX addresses and ports)
8. Configure audio settings for trunk group (encoder, full duplex, framing, etc.)
9. Bridge, groups, and connections are created automatically with txAudio on both groups

## Menu Structure

```
Engage Bridging Service Configuration (Main Menu)
├── [Bridges Overview - shows all bridges, * indicates manually created]
│
├── 1. Raw Bridge (Multicast to Rallypoint) - Wizard
├── 2. Audio Bridge (Multicast to Rallypoint) - Wizard
├── 3. TSM Bridge (TSM Radio to Rallypoint) - Wizard
├── 4. MPU Bridge (MPU Radio to Rallypoint) - Wizard
├── 5. Remove (enter number) - Delete bridge by number
├── 6. Edit Bridges & Groups (Unified Interface)
│   ├── [Numbered Bridges Table - bridges numbered 1, 2, 3...]
│   ├── [Numbered Groups Table - groups continue numbering 4, 5, 6...]
│   │
│   ├── 1. Edit (enter number) - Edit any bridge or group by number
│   ├── 2. Add (Bridge or Group) - Choose type, then add
│   ├── 3. Remove (enter number) - Remove any bridge or group by number
│   ├── 4. Show JSON - Display bridges.json in colorized format
│   └── b. Back (Esc)
│
├── 7. Service Configuration
│   ├── 1. Update License from File
│   ├── 2. General Settings (shows current values first)
│   ├── 3. Status Report Settings (shows current values first)
│   ├── 4. Health Check Responder (shows current values first)
│   ├── 5. Multicast TX Options (shows current values first)
│   ├── 6. TX Options (shows current values first)
│   ├── 7. Show Main Config (colorized JSON)
│   ├── b. Back (Esc)
│   └── q. Quit
│
└── q. Quit
```

## Help and Usage

### Getting Help

- Run with `--help` or `-h` flag to see usage information
- Type `?` when prompted for Group Type to see valid values (0-3) with descriptions
- Type `?` when prompted for Audio Encoder to see all valid encoder codes and descriptions
- If config files are not found, usage information is automatically displayed

### Group Type Values

- `0` - Unknown
- `1` - Audio
- `2` - Presence
- `3` - Raw

The prompt shows the current value: `Group type (0-3, ? for help) [1 - Audio]:`

### Audio Encoder Values

The tool supports many audio encoders including:
- G.711 (U-Law, A-Law)
- GSM, G.729, PCM
- AMR Narrowband (multiple bitrates)
- Opus (6-24 kbit/s)
- Speex (2.15-24.6 kbit/s)
- Codec2 (0.45-3.2 kbit/s)
- MELPe (0.60-2.4 kbit/s)
- CVSD

The prompt shows the current value: `Audio encoder (? for help) [25 - Opus 16 (kbit/s)]:`

Type `?` when prompted for the encoder to see the complete list with descriptions.

## Technical Details

### File Handling

- All changes are saved immediately after configuration
- JSON files are formatted with proper indentation (3 spaces for main config, 4 for bridges)
- The tool preserves JSON structure and formatting
- Attributes starting with `_` are preserved but not editable

### Input Pre-filling

- On Unix/macOS systems, the tool uses `readline` to pre-fill input fields
- When editing bridges, group selection is pre-filled with current groups
- You can edit the pre-filled values or press Enter to keep them

### Display Features

- Groups are displayed with blank lines between each group block for readability
- Group IDs are fully displayed; names are truncated if too long
- Current values are shown in brackets in prompts for group type and audio encoder
- Lists are automatically displayed when entering relevant menus

## Notes

- All changes are saved immediately after configuration
- The tool validates input and provides helpful error messages
- Destructive operations (removing groups/bridges) require confirmation
- When editing, all fields are pre-populated with current values
- When listing groups, all attributes are shown with actual JSON attribute names
- Values with help (group type, audio encoder) display both the value and description
- The tool preserves JSON formatting and structure
- Works on Linux, macOS, and Windows (uses standard library only)
- Perfect for airgapped systems - no internet connection required
- Temporary backup files are automatically cleaned up on successful exit

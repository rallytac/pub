#!/usr/bin/env python3
"""
EBS Configuration Tool - Interactive Menu
A simple interactive tool to modify Engage Bridging Service (EBS) configuration files.

Copyright (c) 2025 Rally Tactical Systems Inc
All rights reserved.
"""

import json
import sys
import os
import shutil
import re
import socket
import subprocess
import select
import io
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, List, Optional
from types import MethodType

# Version information
VERSION = "0.1"


class AbortOperationException(Exception):
    """Raised when a user aborts out of an interactive workflow."""
    pass


class Colors:
    """ANSI color codes for terminal output."""
    # Reset
    RESET = '\033[0m'
    
    # Regular colors
    RED = '\033[31m'
    GREEN = '\033[32m'
    YELLOW = '\033[33m'
    BLUE = '\033[34m'
    MAGENTA = '\033[35m'
    CYAN = '\033[36m'
    WHITE = '\033[37m'
    
    # Bright colors
    BRIGHT_RED = '\033[91m'
    BRIGHT_GREEN = '\033[92m'
    BRIGHT_YELLOW = '\033[93m'
    BRIGHT_BLUE = '\033[94m'
    BRIGHT_MAGENTA = '\033[95m'
    BRIGHT_CYAN = '\033[96m'
    BRIGHT_WHITE = '\033[97m'
    
    # Styles
    BOLD = '\033[1m'
    DIM = '\033[2m'
    UNDERLINE = '\033[4m'
    
    @classmethod
    def disable(cls):
        """Disable colors (for non-terminal environments)."""
        for attr in dir(cls):
            if not attr.startswith('_') and attr != 'disable' and attr != 'RESET':
                setattr(cls, attr, '')


# Try to import readline for input pre-filling (Unix/macOS only)
try:
    import readline
    READLINE_AVAILABLE = True
except ImportError:
    readline = None
    READLINE_AVAILABLE = False

# Try to import termios and tty for Esc key detection (Unix/macOS only)
try:
    import termios
    import tty
    HAS_TERMIOS = True
except ImportError:
    HAS_TERMIOS = False

# Static codec mappings for RTP payload transformations
CODEC_MAPPINGS = [
    {"name": "G.711 ulaw @ 64kbps", "engageEncoder": 1, "rtpPayloadTypes":[{"engage": 0}, {"tsm": 0}, {"mpu": 0}, {"silvus": 0}]},
    {"name": "G.711 alaw @ 64kbps", "engageEncoder": 2, "rtpPayloadTypes":[{"engage": 8}, {"tsm": 8}, {"mpu": 8}, {"silvus": 8}]},
    {"name": "GSM 6.10 @ 13.3kbps", "engageEncoder": 3, "rtpPayloadTypes":[{"engage": 3}]},    
    {"name": "AMR Narrowband @ 4.75kbps", "engageEncoder": 10, "rtpPayloadTypes":[{"engage": 122}]},   
    {"name": "AMR Narrowband @ 5.15kbps", "engageEncoder": 11, "rtpPayloadTypes":[{"engage": 122}]},
    {"name": "AMR Narrowband @ 5.9kbps", "engageEncoder": 12, "rtpPayloadTypes":[{"engage": 122}, {"tsm": 122}]},
    {"name": "AMR Narrowband @ 6.7kbps", "engageEncoder": 13, "rtpPayloadTypes":[{"engage": 122}]},
    {"name": "AMR Narrowband @ 7.4kbps", "engageEncoder": 14, "rtpPayloadTypes":[{"engage": 122}]},
    {"name": "AMR Narrowband @ 7.95kbps", "engageEncoder": 15, "rtpPayloadTypes":[{"engage": 122}]},
    {"name": "AMR Narrowband @ 10.2kbps", "engageEncoder": 16, "rtpPayloadTypes":[{"engage": 122}]},
    {"name": "AMR Narrowband @ 12.2kbps", "engageEncoder": 17, "rtpPayloadTypes":[{"engage": 122}]},
    {"name": "Opus @ 6kbps", "engageEncoder": 20, "rtpPayloadTypes":[{"engage": 118}, {"silvus": 118}]},
    {"name": "Opus @ 8kbps", "engageEncoder": 21, "rtpPayloadTypes":[{"engage": 118}, {"silvus": 118}]},
    {"name": "Opus @ 10kbps", "engageEncoder": 22, "rtpPayloadTypes":[{"engage": 118}, {"silvus": 118}]},
    {"name": "Opus @ 12kbps", "engageEncoder": 23, "rtpPayloadTypes":[{"engage": 118}, {"silvus": 118}]},
    {"name": "Opus @ 14kbps", "engageEncoder": 24, "rtpPayloadTypes":[{"engage": 118}, {"silvus": 118}]},    
    {"name": "Opus @ 16kbps", "engageEncoder": 25, "rtpPayloadTypes":[{"engage": 118}, {"mpu": 118}, {"silvus": 118}]},
    {"name": "Opus @ 18kbps", "engageEncoder": 26, "rtpPayloadTypes":[{"engage": 118}, {"silvus": 118}]},    
    {"name": "Opus @ 20kbps", "engageEncoder": 27, "rtpPayloadTypes":[{"engage": 118}, {"silvus": 118}]},
    {"name": "Opus @ 22kbps", "engageEncoder": 28, "rtpPayloadTypes":[{"engage": 118}, {"silvus": 118}]},
    {"name": "Opus @ 24kbps", "engageEncoder": 29, "rtpPayloadTypes":[{"engage": 118}, {"silvus": 118}]},
    {"name": "Speex Narrowband @ 2.15kbps", "engageEncoder": 30, "rtpPayloadTypes":[{"engage": 97}]},   
    {"name": "Speex Narrowband @ 3.95kbps", "engageEncoder": 31, "rtpPayloadTypes":[{"engage": 97}]},
    {"name": "Speex Narrowband @ 5.95kbps", "engageEncoder": 32, "rtpPayloadTypes":[{"engage": 97}]},
    {"name": "Speex Narrowband @ 8kbps", "engageEncoder": 33, "rtpPayloadTypes":[{"engage": 97}]},
    {"name": "Speex Narrowband @ 11kbps", "engageEncoder": 34, "rtpPayloadTypes":[{"engage": 97}]},
    {"name": "Speex Narrowband @ 15kbps", "engageEncoder": 35, "rtpPayloadTypes":[{"engage": 97}]},
    {"name": "Speex Narrowband @ 18.2kbps", "engageEncoder": 36, "rtpPayloadTypes":[{"engage": 97}]},
    {"name": "Speex Narrowband @ 24.6kbps", "engageEncoder": 37, "rtpPayloadTypes":[{"engage": 97}]},
    {"name": "Codec2 @ 0.45kbps", "engageEncoder": 40, "rtpPayloadTypes":[{"engage": 88}]},
    {"name": "Codec2 @ 0.7kbps", "engageEncoder": 41, "rtpPayloadTypes":[{"engage": 88}]},
    {"name": "Codec2 @ 1.2kbps", "engageEncoder": 42, "rtpPayloadTypes":[{"engage": 85}]},
    {"name": "Codec2 @ 1.3kbps", "engageEncoder": 43, "rtpPayloadTypes":[{"engage": 84}]},
    {"name": "Codec2 @ 1.4kbps", "engageEncoder": 44, "rtpPayloadTypes":[{"engage": 83}]},
    {"name": "Codec2 @ 1.6kbps", "engageEncoder": 45, "rtpPayloadTypes":[{"engage": 82}]},
    {"name": "Codec2 @ 2.4kbps", "engageEncoder": 46, "rtpPayloadTypes":[{"engage": 81}]},
    {"name": "Codec2 @ 3.2kbps", "engageEncoder": 47, "rtpPayloadTypes":[{"engage": 80}]},
    {"name": "MELPe @ 0.6kbps", "engageEncoder": 50, "rtpPayloadTypes":[{"engage": 79}, {"tsm": 79}]},
    {"name": "MELPe @ 1.2kbps", "engageEncoder": 51, "rtpPayloadTypes":[{"engage": 78}, {"tsm": 78}]},
    {"name": "MELPe @ 2.4kbps", "engageEncoder": 52, "rtpPayloadTypes":[{"engage": 77}, {"tsm": 77}, {"mpu": 117}]}
]


def show_usage():
    """Display usage information."""
    print("\n" + "=" * 60)
    print(f"EBS Configuration Tool v{VERSION} - Usage")
    print("=" * 60)
    print("Copyright (c) 2025 Rally Tactical Systems Inc")
    print("=" * 60)
    print("\n  ./ebs_config_tool.py [options]")
    print("\nOptions:")
    print("  (no arguments)              Use default filenames in current directory")
    print("                              Looks for: engagebridged_conf.json, bridges.json")
    print()
    print("  <directory>                 Use default filenames in specified directory")
    print("                              Example: ./ebs_config_tool.py /etc/ebs")
    print()
    print("  <main_config_file>          Use specified main config file")
    print("                              Looks for bridges.json in same directory")
    print("                              Example: ./ebs_config_tool.py config/engagebridged_conf.json")
    print()
    print("  <main_config> <bridges>     Specify both config files explicitly")
    print("                              Example: ./ebs_config_tool.py main.json bridges.json")
    print()
    print("  -h, --help                  Show this help message")
    print()
    print("Default filenames:")
    print("  - engagebridged_conf.json (main EBS configuration)")
    print("  - bridges.json (bridging configuration)")
    print()


class EBSConfigTool:
    def __init__(self, main_config_path: str, bridges_config_path: str):
        self.main_config_path = Path(main_config_path)
        self.bridges_config_path = Path(bridges_config_path)
        self.main_config = None
        self.bridges_config = None
        self.main_backup_path = None
        self.bridges_backup_path = None
        self.changes_made = False
        
        # Check if colors should be enabled (disable for non-terminal environments)
        self.colors_enabled = sys.stdout.isatty()
        if not self.colors_enabled:
            Colors.disable()
        
        # Store permission warnings to display in menus
        self.permission_warnings = []
    
    def colored(self, text: str, color: str, style: str = '') -> str:
        """Apply color and style to text."""
        if not self.colors_enabled:
            return text
        return f"{style}{color}{text}{Colors.RESET}"

    def success(self, text: str) -> str:
        """Green text for success messages."""
        return self.colored(text, Colors.BRIGHT_GREEN, Colors.BOLD)

    def error(self, text: str) -> str:
        """Red text for error messages."""
        return self.colored(text, Colors.BRIGHT_RED, Colors.BOLD)

    def warning(self, text: str) -> str:
        """Yellow text for warning messages."""
        return self.colored(text, Colors.BRIGHT_YELLOW, Colors.BOLD)

    def info(self, text: str) -> str:
        """Cyan text for info messages."""
        return self.colored(text, Colors.BRIGHT_CYAN)

    def header(self, text: str) -> str:
        """Cyan text for headers."""
        return self.colored(text, Colors.BRIGHT_CYAN, Colors.BOLD)

    def menu_option(self, text: str) -> str:
        """White text for menu options."""
        return self.colored(text, Colors.BRIGHT_WHITE)

    def highlight(self, text: str) -> str:
        """Magenta text for highlighting."""
        return self.colored(text, Colors.BRIGHT_MAGENTA, Colors.BOLD)

    def dim(self, text: str) -> str:
        """Dimmed text for secondary info."""
        return self.colored(text, Colors.WHITE, Colors.DIM)
    
    def get_visual_length(self, text: str) -> int:
        """Get the visual length of text, excluding ANSI color codes."""
        import re
        # Remove ANSI escape sequences
        ansi_escape = re.compile(r'\x1B\[[0-9;]*m')
        clean_text = ansi_escape.sub('', text)
        return len(clean_text)
    
    def check_file_permissions(self):
        """Check for file permission issues and store warnings."""
        self.permission_warnings = []
        
        # Check main config file
        try:
            if self.main_config_path.exists():
                if not os.access(self.main_config_path, os.R_OK):
                    self.permission_warnings.append(f"No read permission for main config: {self.main_config_path}")
                elif not os.access(self.main_config_path, os.W_OK):
                    self.permission_warnings.append(f"No write permission for main config: {self.main_config_path}")
            else:
                # Check directory permissions for new file creation
                parent_dir = self.main_config_path.parent
                if not os.access(parent_dir, os.W_OK):
                    self.permission_warnings.append(f"No write permission for main config directory: {parent_dir}")
        except Exception as e:
            self.permission_warnings.append(f"Error checking main config permissions: {e}")
        
        # Check bridges config file
        try:
            if self.bridges_config_path.exists():
                if not os.access(self.bridges_config_path, os.R_OK):
                    self.permission_warnings.append(f"No read permission for bridges config: {self.bridges_config_path}")
                elif not os.access(self.bridges_config_path, os.W_OK):
                    self.permission_warnings.append(f"No write permission for bridges config: {self.bridges_config_path}")
            else:
                # Check directory permissions for new file creation
                parent_dir = self.bridges_config_path.parent
                if not os.access(parent_dir, os.W_OK):
                    self.permission_warnings.append(f"No write permission for bridges config directory: {parent_dir}")
        except Exception as e:
            self.permission_warnings.append(f"Error checking bridges config permissions: {e}")
    
    def display_permission_warnings(self):
        """Display stored permission warnings if any exist."""
        if self.permission_warnings:
            print(f"\n{self.warning('⚠ Permission Warnings:')}")
            for warning in self.permission_warnings:
                print(f"  • {warning}")
            print(f"  {self.dim('Note: Configuration changes may fail to save. Consider running with appropriate privileges.')}")
            print()
    
    def display_json_colorized(self, data, indent=0, max_depth=10, file=None):
        """Display JSON data in a color-coded format.
        
        This method recursively formats and displays JSON data with syntax highlighting.
        Colors are applied to different JSON elements: keys (cyan), strings (green),
        numbers (magenta), booleans (yellow), and structural elements (white).
        
        Args:
            data: The data to display (dict, list, or primitive JSON type)
            indent: Current indentation level (used internally for recursion)
            max_depth: Maximum recursion depth to prevent infinite loops
            file: Optional file-like object to write to. Accepts any object with
                  a write() method (e.g., sys.stdout, io.StringIO, file handles).
                  Defaults to sys.stdout if None.
        
        Returns:
            None. Output is written directly to the specified file object.
        """
        if max_depth <= 0:
            return
        
        if file is None:
            file = sys.stdout
        
        indent_str = "  " * indent
        
        if isinstance(data, dict):
            if not data:
                print(f"{indent_str}{self.colored('{}', Colors.WHITE)}", end="", file=file)
                return
            print(f"{indent_str}{self.colored('{', Colors.WHITE)}", file=file)
            items = list(data.items())
            for i, (key, value) in enumerate(items):
                is_last = i == len(items) - 1
                key_str = f'"{key}"'
                print(f"{indent_str}  {self.colored(key_str, Colors.BRIGHT_CYAN)}: ", end="", file=file)
                self.display_json_colorized(value, indent + 1, max_depth - 1, file)
                if not is_last:
                    print(f"{self.colored(',', Colors.WHITE)}", file=file)
            print(file=file)
            print(f"{indent_str}{self.colored('}', Colors.WHITE)}", end="", file=file)
        elif isinstance(data, list):
            if not data:
                print(f"{indent_str}{self.colored('[]', Colors.WHITE)}", end="", file=file)
                return
            print(f"{indent_str}{self.colored('[', Colors.WHITE)}", file=file)
            for i, item in enumerate(data):
                is_last = i == len(data) - 1
                print(f"{indent_str}  ", end="", file=file)
                self.display_json_colorized(item, indent + 1, max_depth - 1, file)
                if not is_last:
                    print(f"{self.colored(',', Colors.WHITE)}", file=file)
            print(file=file)
            print(f"{indent_str}{self.colored(']', Colors.WHITE)}", end="", file=file)
        elif isinstance(data, str):
            quoted_str = f'"{data}"'
            print(f"{self.colored(quoted_str, Colors.BRIGHT_GREEN)}", end="", file=file)
        elif isinstance(data, bool):
            print(f"{self.colored(str(data).lower(), Colors.BRIGHT_YELLOW)}", end="", file=file)
        elif isinstance(data, (int, float)):
            print(f"{self.colored(str(data), Colors.BRIGHT_MAGENTA)}", end="", file=file)
        elif data is None:
            print(f"{self.dim('null')}", end="", file=file)
        else:
            print(f"{self.colored(str(data), Colors.WHITE)}", end="", file=file)
    
    def _get_pager_navigation_help(self, pager_type):
        """Get navigation help text for a specific pager type.
        
        Args:
            pager_type: String indicating pager type ('less' or 'more')
        
        Returns:
            List of strings containing formatted navigation instructions
        """
        if pager_type == 'less':
            return [
                self.dim("Navigation: ") + "q/Q=Quit | Space/↓=Down | b/↑=Up | /=Search | g=Top | G=Bottom"
            ]
        elif pager_type == 'more':
            return [
                self.dim("Navigation: ") + "q=Quit | Space=Down | Enter=Line | Ctrl+C=Exit"
            ]
        return []
    
    def display_json_colorized_with_pager(self, data, header_lines=None):
        """Display JSON data in a color-coded format with pager support for scrolling.
        
        This method displays large JSON outputs using a system pager (less or more)
        to enable scrolling through long content. The output is first captured in
        memory, then piped through a pager. If no pager is available or pager
        execution fails, the output is printed directly to stdout.
        
        Args:
            data: The data to display (dict, list, or primitive JSON type)
            header_lines: Optional list of header lines (strings) to display
                         before the JSON content. Each line is printed as-is.
        
        Returns:
            None. Output is displayed via pager or printed directly to stdout.
        
        Note:
            - Prefers 'less' with color support (-R flag) if available
            - Falls back to 'more' if 'less' is not available
            - Falls back to direct printing if pager execution fails
            - ANSI color codes are preserved when using 'less -R'
            - Navigation instructions are displayed at the top of the pager output
        """
        # Determine which pager will be used
        pager_type = None
        if shutil.which('less'):
            pager_type = 'less'
        elif shutil.which('more'):
            pager_type = 'more'
        
        # Capture output to StringIO
        output_buffer = io.StringIO()
        
        # Write header if provided
        if header_lines:
            for line in header_lines:
                print(line, file=output_buffer)
        
        # Add navigation help if using a pager
        if pager_type:
            nav_help = self._get_pager_navigation_help(pager_type)
            if nav_help:
                print(file=output_buffer)  # Blank line after header
                for line in nav_help:
                    print(line, file=output_buffer)
                print(self.dim("-" * 60), file=output_buffer)  # Separator line
        
        print(file=output_buffer)  # Blank line before JSON
        
        # Write JSON to buffer
        self.display_json_colorized(data, file=output_buffer)
        print(file=output_buffer)  # Final newline
        
        # Get the full output
        full_output = output_buffer.getvalue()
        output_buffer.close()
        
        # Try to use a pager (less with color support)
        pager_cmd = None
        if pager_type == 'less':
            # Use less with -R to preserve ANSI colors, -X to not clear screen on exit
            pager_cmd = ['less', '-R', '-X']
        elif pager_type == 'more':
            # Fallback to more (though it doesn't support colors as well)
            pager_cmd = ['more']
        
        if pager_cmd:
            try:
                # Pipe output through pager
                process = subprocess.Popen(
                    pager_cmd,
                    stdin=subprocess.PIPE,
                    stdout=sys.stdout,
                    stderr=sys.stderr,
                    text=True
                )
                process.communicate(input=full_output)
                return
            except Exception as e:
                # If pager fails, fall through to direct printing
                print(f"{self.warning('⚠ Warning:')} Could not use pager: {e}")
        
        # Fallback: print directly (no scrolling, but at least it works)
        print(full_output, end="")
    
    def show_bridges_json(self):
        """Display the bridges configuration file in color-coded JSON format."""
        self.clear_screen()
        self.display_permission_warnings()
        
        # Prepare header lines
        header_lines = [
            f"\n{self.header('=== Bridges Configuration JSON ===')}",
            f"File: {self.bridges_config_path}",
            f"{self.header('=' * 40)}"
        ]
        
        try:
            # Pretty print the JSON with color coding
            json_str = json.dumps(self.bridges_config, indent=2, ensure_ascii=False)
            parsed_data = json.loads(json_str)
            self.display_json_colorized_with_pager(parsed_data, header_lines)
        except Exception as e:
            print(f"{self.error('✗ Error displaying JSON:')} {e}")
            self.safe_input("\nPress Enter to continue...")
            return
        
        # If pager was used, it will have already exited
        # If not, we still need to wait for user input
        self.safe_input("\nPress Enter to continue...")
        
    def create_backup(self, filepath: Path) -> Path:
        """Create a backup of a file with timestamp."""
        if not filepath.exists():
            return None
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        backup_path = filepath.parent / f"{filepath.stem}.backup_{timestamp}{filepath.suffix}"
        try:
            shutil.copy2(filepath, backup_path)
            return backup_path
        except Exception as e:
            print(f"Warning: Could not create backup of {filepath}: {e}")
            return None
    
    def ensure_backups(self):
        """Create backups if they don't exist yet (lazy backup creation)."""
        if self.main_backup_path is None and self.main_config_path.exists():
            self.main_backup_path = self.create_backup(self.main_config_path)
            if self.main_backup_path:
                print(f"Backup created: {self.main_backup_path.name}")
        
        if self.bridges_backup_path is None and self.bridges_config_path.exists():
            self.bridges_backup_path = self.create_backup(self.bridges_config_path)
            if self.bridges_backup_path:
                print(f"Backup created: {self.bridges_backup_path.name}")
    
    def check_and_initialize_config(self):
        """Check and initialize required configuration fields on first run."""
        needs_save = False
        
        # Check if we need to gather info
        id_empty = not self.main_config.get('id') or self.main_config.get('id').strip() == ''
        engine_policy = self.main_config.get('enginePolicy', {})
        licensing = engine_policy.get('licensing', {})
        featureset = engine_policy.get('featureset', {})
        
        # Check if license is empty (check key fields)
        license_empty = (
            not licensing.get('entitlement') or licensing.get('entitlement').strip() == '' or
            not licensing.get('key') or licensing.get('key').strip() == ''
        )
        
        # Check if featureset is empty
        featureset_empty = (
            not featureset.get('signature') or featureset.get('signature').strip() == '' or
            not featureset.get('features') or len(featureset.get('features', [])) == 0
        )
        
        # If anything is missing, show welcome message and gather info
        if id_empty or license_empty or featureset_empty:
            print("\n" + "=" * 60)
            print("  Welcome to EBS Configuration Tool")
            print("=" * 60)
            print("\nIt looks like this is the first time you are configuring your EBS.")
            print("We need to gather some information to get started.\n")
        
        # Check ID
        if id_empty:
            while True:
                ebs_id = self.safe_input("Enter EBS ID: ").strip()
                if ebs_id:
                    self.main_config['id'] = ebs_id
                    needs_save = True
                    print(f"{self.success('✓')} EBS ID set to: {self.highlight(ebs_id)}\n")
                    break
                else:
                    print("EBS ID cannot be empty. Please enter a value.")
        
        # Check license and featureset
        if license_empty or featureset_empty:
            print(f"\n{self.warning('⚠')} License or featureset is missing or empty.")
            print("Please provide a license file (JSON) containing 'license' and 'featureset' objects.")
            
            while True:
                license_file_path = self.safe_input("Enter path to license file (or 'skip' to continue without): ").strip()
                
                if license_file_path.lower() == 'skip':
                    print("Skipping license initialization. You can add it later.")
                    break
                
                if not license_file_path:
                    print("Please enter a file path or 'skip'.")
                    continue
                
                # Try to load the license file
                license_path = Path(license_file_path)
                if not license_path.exists():
                    print(f"{self.error('✗ Error:')} File not found: {license_file_path}")
                    continue
                
                try:
                    with open(license_path, 'r') as f:
                        license_data = json.load(f)
                    
                    # Check for license and featureset in the file
                    if 'license' not in license_data and 'licensing' not in license_data:
                        print(f"{self.error('✗ Error:')} License file must contain 'license' or 'licensing' object.")
                        continue
                    
                    if 'featureset' not in license_data:
                        print(f"{self.error('✗ Error:')} License file must contain 'featureset' object.")
                        continue
                    
                    # Extract license data (try both 'license' and 'licensing' keys)
                    file_license = license_data.get('license') or license_data.get('licensing', {})
                    file_featureset = license_data.get('featureset', {})
                    
                    # Update enginePolicy
                    if 'enginePolicy' not in self.main_config:
                        self.main_config['enginePolicy'] = {}
                    
                    # Update licensing
                    if 'licensing' not in self.main_config['enginePolicy']:
                        self.main_config['enginePolicy']['licensing'] = {}
                    
                    # Merge license data
                    self.main_config['enginePolicy']['licensing'].update(file_license)
                    
                    # Update featureset - ensure it's placed directly after licensing
                    # Python 3.7+ preserves insertion order, so we rebuild the dict in the right order
                    engine_policy = self.main_config['enginePolicy']
                    new_engine_policy = {}
                    
                    # Add all keys before 'licensing'
                    for key, value in engine_policy.items():
                        if key == 'licensing':
                            break
                        if key != 'featureset':  # Skip old featureset if it exists
                            new_engine_policy[key] = value
                    
                    # Add licensing
                    new_engine_policy['licensing'] = engine_policy.get('licensing', {})
                    new_engine_policy['licensing'].update(file_license)
                    
                    # Add featureset right after licensing
                    new_engine_policy['featureset'] = file_featureset
                    
                    # Add all remaining keys (after licensing and featureset)
                    for key, value in engine_policy.items():
                        if key not in new_engine_policy and key != 'featureset':
                            new_engine_policy[key] = value
                    
                    self.main_config['enginePolicy'] = new_engine_policy
                    
                    needs_save = True
                    print(f"{self.success('✓')} License and featureset loaded from file.")
                    break
                    
                except json.JSONDecodeError as e:
                    print(f"Error: Invalid JSON in license file: {e}")
                    continue
                except Exception as e:
                    print(f"Error reading license file: {e}")
                    continue
        
        # Save if changes were made
        if needs_save:
            if self.save_main_config():
                print(f"{self.success('✓')} Configuration initialized and saved.")
            else:
                print(f"{self.warning('⚠ Warning:')} Failed to save configuration changes.")
    
    def load_configs(self):
        """Load both configuration files."""
        # Reset changes flag
        self.changes_made = False
        
        # Check file permissions early
        self.check_file_permissions()
        
        try:
            with open(self.main_config_path, 'r') as f:
                self.main_config = json.load(f)
        except FileNotFoundError:
            print(f"Error: Main config file not found: {self.main_config_path}")
            show_usage()
            sys.exit(1)
        except json.JSONDecodeError as e:
            print(f"Error: Invalid JSON in main config: {e}")
            sys.exit(1)
            
        try:
            with open(self.bridges_config_path, 'r') as f:
                self.bridges_config = json.load(f)
        except FileNotFoundError:
            print(f"Error: Bridges config file not found: {self.bridges_config_path}")
            show_usage()
            sys.exit(1)
        except json.JSONDecodeError as e:
            print(f"Error: Invalid JSON in bridges config: {e}")
            sys.exit(1)
    
    
    def save_main_config(self):
        """Save the main configuration file with JSON validation."""
        # Create backups before first save if they don't exist
        if not self.changes_made:
            self.ensure_backups()
        
        # Check write permissions before attempting to save
        try:
            if self.main_config_path.exists():
                if not os.access(self.main_config_path, os.W_OK):
                    print(f"\n{self.error('✗ Permission Error:')} No write permission for {self.main_config_path}")
                    print("Please check file permissions or run with appropriate privileges.")
                    return False
            else:
                # Check if we can write to the directory
                parent_dir = self.main_config_path.parent
                if not os.access(parent_dir, os.W_OK):
                    print(f"\n{self.error('✗ Permission Error:')} No write permission for directory {parent_dir}")
                    print("Please check directory permissions or run with appropriate privileges.")
                    return False
        except Exception as e:
            print(f"\n{self.error('✗ Permission Check Error:')} {e}")
            return False
        
        try:
            # Validate JSON by encoding to string first
            json_str = json.dumps(self.main_config, indent=3, ensure_ascii=False)
            # Validate by parsing it back
            json.loads(json_str)
            
            # Write to file
            with open(self.main_config_path, 'w') as f:
                f.write(json_str)
            print(f"\n{self.success('✓')} Successfully saved {self.highlight(str(self.main_config_path))}")
            self.changes_made = True
            return True
        except PermissionError as e:
            print(f"\n{self.error('✗ Permission Error:')} {e}")
            print("Please check file permissions or run with appropriate privileges.")
            return False
        except (TypeError, ValueError) as e:
            print(f"\n{self.error('✗ JSON Error:')} Invalid JSON structure - {e}")
            return False
        except OSError as e:
            print(f"\n{self.error('✗ File System Error:')} {e}")
            return False
        except Exception as e:
            print(f"\n{self.error('✗ Error saving main config:')} {e}")
            return False
    
    def save_bridges_config(self):
        """Save the bridges configuration file with JSON validation."""
        # Create backups before first save if they don't exist
        if not self.changes_made:
            self.ensure_backups()
        
        # Check write permissions before attempting to save
        try:
            if self.bridges_config_path.exists():
                if not os.access(self.bridges_config_path, os.W_OK):
                    print(f"\n{self.error('✗ Permission Error:')} No write permission for {self.bridges_config_path}")
                    print("Please check file permissions or run with appropriate privileges.")
                    return False
            else:
                # Check if we can write to the directory
                parent_dir = self.bridges_config_path.parent
                if not os.access(parent_dir, os.W_OK):
                    print(f"\n{self.error('✗ Permission Error:')} No write permission for directory {parent_dir}")
                    print("Please check directory permissions or run with appropriate privileges.")
                    return False
        except Exception as e:
            print(f"\n{self.error('✗ Permission Check Error:')} {e}")
            return False
        
        try:
            # Validate JSON by encoding to string first
            json_str = json.dumps(self.bridges_config, indent=4, ensure_ascii=False)
            # Validate by parsing it back
            json.loads(json_str)
            
            # Write to file
            with open(self.bridges_config_path, 'w') as f:
                f.write(json_str)
            print(f"\n{self.success('✓')} Successfully saved {self.highlight(str(self.bridges_config_path))}")
            self.changes_made = True
            return True
        except PermissionError as e:
            print(f"\n{self.error('✗ Permission Error:')} {e}")
            print("Please check file permissions or run with appropriate privileges.")
            return False
        except (TypeError, ValueError) as e:
            print(f"\n{self.error('✗ JSON Error:')} Invalid JSON structure - {e}")
            return False
        except OSError as e:
            print(f"\n{self.error('✗ File System Error:')} {e}")
            return False
        except Exception as e:
            print(f"\n{self.error('✗ Error saving bridges config:')} {e}")
            return False
    
    def clear_screen(self):
        """Clear the screen (simple version)."""
        os.system('clear' if os.name != 'nt' else 'cls')
    
    def print_separator(self):
        """Print a separator line."""
        print("-" * 60)
    
    def cleanup_backups(self):
        """Delete temporary backup files."""
        deleted = False
        if self.main_backup_path and self.main_backup_path.exists():
            try:
                self.main_backup_path.unlink()
                deleted = True
            except Exception as e:
                print(f"Warning: Could not delete backup {self.main_backup_path.name}: {e}")
        
        if self.bridges_backup_path and self.bridges_backup_path.exists():
            try:
                self.bridges_backup_path.unlink()
                deleted = True
            except Exception as e:
                print(f"Warning: Could not delete backup {self.bridges_backup_path.name}: {e}")
        
        return deleted
    
    def restore_from_backup(self):
        """Restore config files from backups."""
        restored = False
        if self.main_backup_path and self.main_backup_path.exists():
            try:
                shutil.copy2(self.main_backup_path, self.main_config_path)
                print(f"✓ Restored main config from {self.main_backup_path.name}")
                restored = True
            except Exception as e:
                print(f"✗ Error restoring main config: {e}")
        
        if self.bridges_backup_path and self.bridges_backup_path.exists():
            try:
                shutil.copy2(self.bridges_backup_path, self.bridges_config_path)
                print(f"✓ Restored bridges config from {self.bridges_backup_path.name}")
                restored = True
            except Exception as e:
                print(f"✗ Error restoring bridges config: {e}")
        
        if restored:
            self.changes_made = False
            print("\nAll changes have been reverted.")
            # Clean up backups after restore
            self.cleanup_backups()
        return restored
    
    def handle_quit(self) -> bool:
        """
        Handle quit request (q key). Returns True if should quit, False if should continue.
        """
        # If no changes were made, simply exit
        if not self.changes_made:
            self.cleanup_backups()
            print("\nExiting...")
            sys.exit(0)
        
        # Changes were made, show options
        print("\nOptions:")
        print("1. Quit")
        print("2. Revert all changes and Quit")
        print("3. Continue editing")
        
        while True:
            try:
                choice = input("\nSelect option (1-3): ").strip()
                if choice == '1':
                    # Exit - changes are already saved (they're saved immediately after each edit)
                    # Just clean up backups
                    self.cleanup_backups()
                    print("\nExiting...")
                    sys.exit(0)
                elif choice == '2':
                    # Revert all changes and exit - restore from backup
                    self.restore_from_backup()
                    # restore_from_backup already cleans up backups
                    print("\nReverted all changes. Exiting...")
                    sys.exit(0)
                elif choice == '3':
                    return False  # Continue
                else:
                    print("Invalid option. Please enter 1, 2, or 3.")
            except KeyboardInterrupt:
                # If they press Ctrl+C during selection, just exit
                print("\n\nExiting...")
                sys.exit(0)
    
    def _get_char(self) -> str:
        """Read a single character from stdin, handling Esc key."""
        if not HAS_TERMIOS:
            # Fallback: use regular input (Esc won't be detected)
            return None
        
        try:
            fd = sys.stdin.fileno()
            old_settings = termios.tcgetattr(fd)
            try:
                tty.setraw(fd)
                ch = sys.stdin.read(1)
                # Check for Esc key (0x1b)
                if ch == '\x1b':
                    # Check if there are more characters (escape sequence)
                    if select.select([sys.stdin], [], [], 0.1)[0]:
                        # Read the rest of the escape sequence
                        seq = sys.stdin.read(2)
                        # If it's just Esc (not an escape sequence), return ESC marker
                        if not seq or seq == '':
                            return 'ESC'
                    else:
                        # Just Esc key pressed
                        return 'ESC'
                return ch
            finally:
                termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        except Exception:
            return None
    
    def menu_input(self, prompt: str = "") -> str:
        """Get menu input with Esc key detection (Esc treated as 'b' for back)."""
        result = self.safe_input(prompt, detect_esc=True)
        # Treat Esc as 'b' (back)
        if result == 'ESC':
            return 'b'
        return result
    
    def safe_input(self, prompt: str = "", detect_esc: bool = False) -> str:
        """Safe input wrapper that handles KeyboardInterrupt and optionally Esc key.
        
        Args:
            prompt: The prompt string to display
            detect_esc: If True, detect Esc key and return 'ESC' marker
        """
        if detect_esc and HAS_TERMIOS:
            # Use character-by-character reading to detect Esc
            try:
                sys.stdout.write(prompt)
                sys.stdout.flush()
                chars = []
                while True:
                    ch = self._get_char()
                    if ch is None:
                        # Fallback to regular input
                        return input(prompt)
                    if ch == 'ESC':
                        return 'ESC'
                    elif ch == '\r' or ch == '\n':
                        break
                    elif ch == '\x7f' or ch == '\b':  # Backspace
                        if chars:
                            chars.pop()
                            sys.stdout.write('\b \b')
                            sys.stdout.flush()
                    elif ord(ch) >= 32:  # Printable character
                        chars.append(ch)
                        sys.stdout.write(ch)
                        sys.stdout.flush()
                sys.stdout.write('\n')
                sys.stdout.flush()
                return ''.join(chars)
            except Exception:
                # Fallback to regular input
                pass
        
        try:
            return input(prompt)
        except KeyboardInterrupt:
            print("\n\n⚠ Interrupted by user (Ctrl+C)")
            
            # If no changes were made, simply exit
            if not self.changes_made:
                self.cleanup_backups()
                print("\nExiting...")
                sys.exit(0)
            
            # Changes were made, show options
            print("\nOptions:")
            print("1. Quit")
            print("2. Revert all changes and Quit")
            print("3. Continue editing")
            
            while True:
                try:
                    choice = input("\nSelect option (1-3): ").strip()
                    if choice == '1':
                        # Exit - changes are already saved (they're saved immediately after each edit)
                        # Just clean up backups
                        self.cleanup_backups()
                        print("\nExiting...")
                        sys.exit(0)
                    elif choice == '2':
                        # Revert all changes and exit - restore from backup
                        self.restore_from_backup()
                        # restore_from_backup already cleans up backups
                        print("\nReverted all changes. Exiting...")
                        sys.exit(0)
                    elif choice == '3':
                        return ""  # Continue
                    else:
                        print("Invalid option. Please enter 1, 2, or 3.")
                except KeyboardInterrupt:
                    # If they press Ctrl+C again during selection, just exit
                    print("\n\nExiting...")
                    sys.exit(0)
    
    def _confirm_abort(self):
        """Prompt user to confirm aborting the current operation."""
        if self.get_yes_no("Do you want to abort this wizard and return to the menu?", True):
            raise AbortOperationException()
    
    def _get_input_with_abort(self, prompt: str, default: str = None) -> str:
        """Get user input with Esc abort detection for wizards."""
        if default:
            full_prompt = f"{prompt} [{default}]: "
        else:
            full_prompt = f"{prompt}: "
        result = self.safe_input(full_prompt, detect_esc=True).strip()
        if result == 'ESC':
            print()  # Move to next line before showing abort confirmation
            self._confirm_abort()
            # If user didn't confirm abort, re-prompt
            return self._get_input_with_abort(prompt, default)
        return result if result else default
    
    def _get_int_with_abort(self, prompt: str, default: int = None) -> Optional[int]:
        """Get integer input with Esc abort detection for wizards."""
        while True:
            try:
                value = self._get_input_with_abort(prompt, str(default) if default is not None else None)
                if not value:
                    if default is not None:
                        return default
                    else:
                        print("Error: Please enter a number.")
                        continue
                try:
                    return int(value)
                except ValueError:
                    print("Please enter a valid integer.")
            except AbortOperationException:
                raise
    
    def _get_ipv4_address_with_abort(self, prompt: str, default: str = None, allow_empty: bool = False) -> str:
        """Get IPv4 address input with Esc abort detection for wizards."""
        while True:
            try:
                address = self._get_input_with_abort(prompt, default)
                if not address:
                    if default:
                        return default
                    elif allow_empty:
                        return ""
                    else:
                        print("Error: Address is required.")
                        continue
                
                if self.is_valid_ipv4(address):
                    return address
                else:
                    print(f"Error: '{address}' is not a valid IPv4 address. Please enter a valid IPv4 address (e.g., 192.168.1.1).")
            except AbortOperationException:
                raise
    
    def _get_host_address_with_abort(self, prompt: str, default: str = None, allow_empty: bool = False) -> str:
        """Get hostname or IPv4 address input with Esc abort detection for wizards."""
        while True:
            try:
                address = self._get_input_with_abort(prompt, default)
                if not address:
                    if default:
                        return default
                    elif allow_empty:
                        return ""
                    else:
                        print("Error: Address is required.")
                        continue
                
                if self.is_valid_ipv4(address) or self.is_valid_hostname(address):
                    return address
                else:
                    print(f"Error: '{address}' is not a valid hostname or IPv4 address. Please enter a valid hostname (e.g., server.example.com) or IPv4 address (e.g., 192.168.1.1).")
            except AbortOperationException:
                raise
    
    def _get_interface_name_with_abort(self, default: str = None) -> str:
        """Get interface name input with Esc abort detection for wizards."""
        while True:
            try:
                prompt = "Interface name (? for available interfaces)"
                if default:
                    prompt += f" [{default}]"
                value = self.safe_input(f"{prompt}: ", detect_esc=True).strip()
                
                if value == 'ESC':
                    print()  # Move to next line before showing abort confirmation
                    self._confirm_abort()
                    continue
                
                if not value:
                    if default:
                        return default
                    print("Error: Interface name is required for multicast groups.")
                    continue
                
                if value == '?':
                    self.show_network_interfaces()
                    continue
                
                # Basic validation - interface names are typically alphanumeric with some special chars
                if re.match(r'^[a-zA-Z0-9_\-\.]+$', value):
                    return value
                else:
                    print("Error: Invalid interface name format.")
            except AbortOperationException:
                raise
    
    def _get_yes_no_with_abort(self, prompt: str, default: bool = True) -> bool:
        """Get yes/no input with Esc abort detection for wizards."""
        try:
            default_str = "Y/n" if default else "y/N"
            response = self.safe_input(f"{prompt} [{default_str}]: ", detect_esc=True).strip().lower()
            
            if response == 'ESC':
                print()  # Move to next line before showing abort confirmation
                self._confirm_abort()
                # If user didn't confirm abort, return default
                return default
            
            if not response:
                return default
            return response in ('y', 'yes', '1', 'true')
        except AbortOperationException:
            raise
    
    def _get_audio_encoder_with_abort(self, default: Optional[int] = 25) -> int:
        """Get audio encoder input with Esc abort detection for wizards."""
        valid_encoders = [-1, 0, 1, 2, 3, 4, 5, 10, 11, 12, 13, 14, 15, 16, 17,
                         20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                         30, 31, 32, 33, 34, 35, 36, 37,
                         40, 41, 42, 43, 44, 45, 46, 47,
                         50, 51, 52, 60]
        while True:
            try:
                if default is not None:
                    # Show current encoder in brackets in the prompt
                    current_desc = self.get_audio_encoder_description(default)
                    prompt = f"Audio encoder (? for help) [{default} - {current_desc}]"
                else:
                    # No default - require input
                    prompt = "Audio encoder (? for help)"
                value = self.safe_input(f"{prompt}: ", detect_esc=True).strip()
                
                if value == 'ESC':
                    print()  # Move to next line before showing abort confirmation
                    self._confirm_abort()
                    continue
                
                if not value:
                    if default is not None:
                        return default
                    else:
                        print("Error: Audio encoder is required. Please enter a value or ? for help.")
                        continue
                if value == '?':
                    self.show_audio_encoders()
                    continue
                try:
                    encoder = int(value)
                    if encoder in valid_encoders:
                        return encoder
                    else:
                        print(f"Error: Invalid encoder value. Valid values: {valid_encoders}")
                except ValueError:
                    print("Error: Please enter a valid number or ? for help.")
            except AbortOperationException:
                raise
    
    def _get_audio_encoder_for_radio_type_with_abort(self, radio_type: str, default: Optional[int] = 25) -> Optional[int]:
        """Get audio encoder for radio type with Esc abort detection for wizards."""
        compatible_encoders = self.get_encoders_for_radio_type(radio_type)
        
        if not compatible_encoders:
            print(f"No encoders available for {radio_type.upper()} radio type.")
            return None
        
        while True:
            try:
                # Show current encoder in brackets in the prompt
                if default and default in compatible_encoders:
                    current_desc = self.get_audio_encoder_description(default)
                    prompt = f"Audio encoder (? for help) [{default} - {current_desc}]"
                else:
                    prompt = "Audio encoder (? for help)"
                
                value = self.safe_input(f"{prompt}: ", detect_esc=True).strip()
                
                if value == 'ESC':
                    print()  # Move to next line before showing abort confirmation
                    self._confirm_abort()
                    continue
                
                if not value and default and default in compatible_encoders:
                    return default
                if value == '?':
                    self.show_audio_encoders_for_radio_type(radio_type)
                    continue
                try:
                    encoder = int(value)
                    if encoder in compatible_encoders:
                        return encoder
                    else:
                        print(f"Error: Encoder {encoder} is not compatible with {radio_type.upper()} radio type.")
                        print("Use '?' to see compatible encoders.")
                except ValueError:
                    print("Error: Please enter a valid encoder number or ? for help.")
            except AbortOperationException:
                raise
    
    def _get_group_type_with_abort(self, default: int = 1) -> int:
        """Get group type input with Esc abort detection for wizards."""
        while True:
            try:
                # Show current type in brackets in the prompt
                current_desc = self.get_group_type_description(default)
                prompt = f"Group type (0-3, ? for help) [{default} - {current_desc}]"
                value = self.safe_input(f"{prompt}: ", detect_esc=True).strip()
                
                if value == 'ESC':
                    print()  # Move to next line before showing abort confirmation
                    self._confirm_abort()
                    continue
                
                if not value:
                    return default
                if value == '?':
                    self.show_group_types()
                    continue
                try:
                    group_type = int(value)
                    if 0 <= group_type <= 3:
                        return group_type
                    else:
                        print("Error: Group type must be between 0 and 3.")
                except ValueError:
                    print("Error: Please enter a valid number (0-3) or ? for help.")
            except AbortOperationException:
                raise
    
    def get_input(self, prompt: str, default: str = None) -> str:
        """Get user input with optional default value."""
        if default:
            full_prompt = f"{prompt} [{default}]: "
        else:
            full_prompt = f"{prompt}: "
        result = self.safe_input(full_prompt).strip()
        return result if result else default
    
    def get_yes_no(self, prompt: str, default: bool = True, _handling_interrupt: bool = False) -> bool:
        """Get yes/no input from user."""
        try:
            default_str = "Y/n" if default else "y/N"
            response = input(f"{prompt} [{default_str}]: ").strip().lower()
            if not response:
                return default
            return response in ('y', 'yes', '1', 'true')
        except KeyboardInterrupt:
            if _handling_interrupt:
                # If we're already handling an interrupt, just exit
                print("\nExiting...")
                sys.exit(0)
            print("\n\n⚠ Interrupted by user (Ctrl+C)")
            # Use _handling_interrupt flag to prevent recursion
            if self.get_yes_no("Do you want to abort editing and exit?", True, _handling_interrupt=True):
                sys.exit(0)
            # User chose to continue, return default
            return default
    
    def get_int(self, prompt: str, default: int = None) -> Optional[int]:
        """Get integer input from user."""
        while True:
            try:
                value = self.get_input(prompt, str(default) if default is not None else None)
                if not value:
                    if default is not None:
                        return default
                    else:
                        print("Error: Please enter a number.")
                        continue
                try:
                    return int(value)
                except ValueError:
                    print("Please enter a valid integer.")
            except KeyboardInterrupt:
                # get_input already handles KeyboardInterrupt, but we need to break the loop
                return default
    
    def is_valid_ipv4(self, address: str) -> bool:
        """Validate if a string is a valid IPv4 address."""
        try:
            parts = address.split('.')
            if len(parts) != 4:
                return False
            for part in parts:
                if not part.isdigit():
                    return False
                num = int(part)
                if num < 0 or num > 255:
                    return False
            return True
        except:
            return False
    
    def get_ipv4_address(self, prompt: str, default: str = None, allow_empty: bool = False) -> str:
        """Get a valid IPv4 address from user input."""
        while True:
            address = self.get_input(prompt, default)
            if not address:
                if default:
                    return default
                elif allow_empty:
                    return ""
                else:
                    print("Error: Address is required.")
                    continue
            
            if self.is_valid_ipv4(address):
                return address
            else:
                print(f"Error: '{address}' is not a valid IPv4 address. Please enter a valid IPv4 address (e.g., 192.168.1.1).")
    
    def is_valid_hostname(self, hostname: str) -> bool:
        """Check if a string is a valid hostname."""
        import re
        # Allow alphanumeric characters, hyphens, and dots
        # Must not start or end with hyphen or dot
        # Each label must be 1-63 characters
        # Total length must be <= 253 characters
        if len(hostname) > 253:
            return False
        
        if hostname.endswith('.'):
            hostname = hostname[:-1]  # Remove trailing dot
        
        # Split into labels and validate each
        labels = hostname.split('.')
        for label in labels:
            if len(label) < 1 or len(label) > 63:
                return False
            if label.startswith('-') or label.endswith('-'):
                return False
            if not re.match(r'^[a-zA-Z0-9-]+$', label):
                return False
        
        return True
    
    def get_host_address(self, prompt: str, default: str = None, allow_empty: bool = False) -> str:
        """Get a valid hostname or IPv4 address from user input."""
        while True:
            address = self.get_input(prompt, default)
            if not address:
                if default:
                    return default
                elif allow_empty:
                    return ""
                else:
                    print("Error: Address is required.")
                    continue
            
            if self.is_valid_ipv4(address) or self.is_valid_hostname(address):
                return address
            else:
                print(f"Error: '{address}' is not a valid hostname or IPv4 address. Please enter a valid hostname (e.g., server.example.com) or IPv4 address (e.g., 192.168.1.1).")
    
    def get_group_type_description(self, group_type: int) -> str:
        """Get descriptive name for a group type value."""
        descriptions = {
            0: "Unknown",
            1: "Audio",
            2: "Presence",
            3: "Raw"
        }
        return descriptions.get(group_type, f"Unknown type ({group_type})")
    
    def show_group_types(self):
        """Display valid group type values and descriptions."""
        print("\nValid Group Types:")
        print("  0 - Unknown (gtUnknown)")
        print("  1 - Audio (gtAudio) - Used to transmit Audio")
        print("  2 - Presence (gtPresence) - Used to relay presence data")
        print("  3 - Raw (gtRaw) - No special processing, binary payload")
        print()
    
    def get_group_type(self, default: int = 1) -> int:
        """Get group type from user with validation and help."""
        while True:
            # Show current type in brackets in the prompt
            current_desc = self.get_group_type_description(default)
            prompt = f"Group type (0-3, ? for help) [{default} - {current_desc}]"
            value = self.safe_input(f"{prompt}: ").strip()
            
            if not value:
                return default
            if value == '?':
                self.show_group_types()
                continue
            try:
                group_type = int(value)
                if 0 <= group_type <= 3:
                    return group_type
                else:
                    print("Error: Group type must be between 0 and 3.")
            except ValueError:
                print("Error: Please enter a valid number (0-3) or ? for help.")
    
    def get_radio_types_for_encoder(self, encoder_id: int) -> List[str]:
        """Get available radio types for a given encoder ID."""
        for codec in CODEC_MAPPINGS:
            if codec['engageEncoder'] == encoder_id:
                radio_types = []
                for payload_type in codec['rtpPayloadTypes']:
                    radio_types.extend(payload_type.keys())
                return list(set(radio_types))  # Remove duplicates
        return ['engage']  # Default to engage if not found
    
    def get_payload_mapping(self, encoder_id: int, radio_type: str) -> Dict[str, int]:
        """Get payload type mappings for encoder + radio type combination."""
        for codec in CODEC_MAPPINGS:
            if codec['engageEncoder'] == encoder_id:
                engage_payload = None
                external_payload = None
                
                # Find engage payload type
                for payload_type in codec['rtpPayloadTypes']:
                    if 'engage' in payload_type:
                        engage_payload = payload_type['engage']
                        break
                
                # Find external payload type for the specific radio type
                for payload_type in codec['rtpPayloadTypes']:
                    if radio_type in payload_type:
                        external_payload = payload_type[radio_type]
                        break
                
                if engage_payload is not None and external_payload is not None:
                    return {
                        'engage': engage_payload,
                        'external': external_payload
                    }
        return None
    
    def configure_payload_transformations(self, group: Dict[str, Any], encoder_id: int, radio_type: str) -> bool:
        """Apply payload transformations to a group based on encoder and radio type."""
        if radio_type == 'engage':
            # No transformations needed for engage - clean up any existing fields
            if 'txAudio' in group and 'customRtpPayloadType' in group['txAudio']:
                del group['txAudio']['customRtpPayloadType']
            if 'inboundRtpPayloadTypeTranslations' in group:
                del group['inboundRtpPayloadTypeTranslations']
            return True
            
        mapping = self.get_payload_mapping(encoder_id, radio_type)
        if not mapping:
            return False
        
        # Check if external payload type matches engage payload type
        if mapping['external'] == mapping['engage']:
            # No transformations needed - payload types match
            # Clean up any existing fields
            if 'txAudio' in group and 'customRtpPayloadType' in group['txAudio']:
                del group['txAudio']['customRtpPayloadType']
            if 'inboundRtpPayloadTypeTranslations' in group:
                del group['inboundRtpPayloadTypeTranslations']
            return True
            
        # Payload types differ - add transformations
        # Add custom RTP payload type to txAudio
        if 'txAudio' not in group:
            group['txAudio'] = {}
        group['txAudio']['customRtpPayloadType'] = mapping['external']
        
        # Add inbound RTP payload type translations
        group['inboundRtpPayloadTypeTranslations'] = [{
            'external': mapping['external'],
            'engage': mapping['engage']
        }]
        
        return True
    
    def get_radio_type_selection(self, encoder_id: int) -> str:
        """Interactive radio type selection for payload transformations."""
        radio_types = self.get_radio_types_for_encoder(encoder_id)
        
        if len(radio_types) == 1 and radio_types[0] == 'engage':
            print("No payload transformations available for this encoder.")
            return 'engage'
        
        print("\nAvailable radio types:")
        options = []
        for i, radio_type in enumerate(radio_types, 1):
            print(f"  {i}. {radio_type.upper()}")
            options.append(radio_type)
        
        print(f"  {len(options) + 1}. Manual (enter custom values)")
        print(f"  {len(options) + 2}. None (no transformations)")
        
        while True:
            try:
                choice = int(self.safe_input("Select radio type: ").strip())
                if 1 <= choice <= len(options):
                    return options[choice - 1]
                elif choice == len(options) + 1:
                    return 'manual'
                elif choice == len(options) + 2:
                    return 'engage'
                else:
                    print(f"Error: Please enter a number between 1 and {len(options) + 2}.")
            except ValueError:
                print("Error: Please enter a valid number.")
    
    def configure_manual_payload_transformations(self, group: Dict[str, Any]) -> bool:
        """Configure manual payload transformations."""
        try:
            custom_payload = self.get_int("Custom RTP payload type (96-127)")
            if custom_payload is None or custom_payload < 96 or custom_payload > 127:
                print("Error: Custom RTP payload type must be between 96 and 127.")
                return False
                
            engage_payload = self.get_int("Engage RTP payload type")
            if engage_payload is None:
                print("Error: Engage RTP payload type is required.")
                return False
            
            # Check if custom payload matches engage payload
            if custom_payload == engage_payload:
                # No transformations needed - payload types match
                # Clean up any existing fields
                if 'txAudio' in group and 'customRtpPayloadType' in group['txAudio']:
                    del group['txAudio']['customRtpPayloadType']
                if 'inboundRtpPayloadTypeTranslations' in group:
                    del group['inboundRtpPayloadTypeTranslations']
                return True
            
            # Payload types differ - add transformations
            # Add custom RTP payload type to txAudio
            if 'txAudio' not in group:
                group['txAudio'] = {}
            group['txAudio']['customRtpPayloadType'] = custom_payload
            
            # Add inbound RTP payload type translations
            group['inboundRtpPayloadTypeTranslations'] = [{
                'external': custom_payload,
                'engage': engage_payload
            }]
            
            return True
        except Exception as e:
            print(f"Error configuring manual transformations: {e}")
            return False
    
    def get_encoders_for_radio_type(self, radio_type: str) -> List[int]:
        """Get list of encoder IDs that support a specific radio type."""
        if radio_type == 'engage':
            # All encoders support engage
            return [codec['engageEncoder'] for codec in CODEC_MAPPINGS]
        
        compatible_encoders = []
        for codec in CODEC_MAPPINGS:
            for payload_type in codec['rtpPayloadTypes']:
                if radio_type in payload_type:
                    compatible_encoders.append(codec['engageEncoder'])
                    break
        return compatible_encoders
    
    def get_audio_encoder_for_radio_type(self, radio_type: str, default: Optional[int] = 25) -> Optional[int]:
        """Get audio encoder with validation for specific radio type compatibility."""
        compatible_encoders = self.get_encoders_for_radio_type(radio_type)
        
        if not compatible_encoders:
            print(f"No encoders available for {radio_type.upper()} radio type.")
            return None
        
        while True:
            # Show current encoder in brackets in the prompt
            if default and default in compatible_encoders:
                current_desc = self.get_audio_encoder_description(default)
                prompt = f"Audio encoder (? for help) [{default} - {current_desc}]"
            else:
                prompt = "Audio encoder (? for help)"
            
            value = self.safe_input(f"{prompt}: ").strip()
            
            if not value and default and default in compatible_encoders:
                return default
            if value == '?':
                self.show_audio_encoders_for_radio_type(radio_type)
                continue
            try:
                encoder = int(value)
                if encoder in compatible_encoders:
                    return encoder
                else:
                    print(f"Error: Encoder {encoder} is not compatible with {radio_type.upper()} radio type.")
                    print("Use '?' to see compatible encoders.")
            except ValueError:
                print("Error: Please enter a valid encoder number or ? for help.")
    
    def show_audio_encoders_for_radio_type(self, radio_type: str):
        """Display valid audio encoder values and descriptions for a specific radio type."""
        compatible_encoders = self.get_encoders_for_radio_type(radio_type)
        
        print(f"\nValid Audio Encoders for {radio_type.upper()}:")
        for codec in CODEC_MAPPINGS:
            if codec['engageEncoder'] in compatible_encoders:
                encoder_id = codec['engageEncoder']
                name = codec['name']
                print(f"  {encoder_id} - {name}")
        print()
    
    def get_audio_encoder_description(self, encoder: int) -> str:
        """Get descriptive name for an audio encoder value."""
        descriptions = {
            -1: "External (Externally implemented)",
            0: "Unknown Codec type",
            1: "G.711 U-Law 64 (kbit/s)",
            2: "G.711 A-Law 64 (kbit/s)",
            3: "GSM Full Rate 13.2 (kbit/s)",
            4: "G.729a 8 (kbit/s)",
            5: "PCM",
            10: "AMR Narrowband 4.75 (kbit/s)",
            11: "AMR Narrowband 5.15 (kbit/s)",
            12: "AMR Narrowband 5.9 (kbit/s)",
            13: "AMR Narrowband 6.7 (kbit/s)",
            14: "AMR Narrowband 7.4 (kbit/s)",
            15: "AMR Narrowband 7.95 (kbit/s)",
            16: "AMR Narrowband 10.2 (kbit/s)",
            17: "AMR Narrowband 12.2 (kbit/s)",
            20: "Opus 6 (kbit/s)",
            21: "Opus 8 (kbit/s)",
            22: "Opus 10 (kbit/s)",
            23: "Opus 12 (kbit/s)",
            24: "Opus 14 (kbit/s)",
            25: "Opus 16 (kbit/s)",
            26: "Opus 18 (kbit/s)",
            27: "Opus 20 (kbit/s)",
            28: "Opus 22 (kbit/s)",
            29: "Opus 24 (kbit/s)",
            30: "Speex 2.15 (kbit/s)",
            31: "Speex 3.95 (kbit/s)",
            32: "Speex 5.95 (kbit/s)",
            33: "Speex 8 (kbit/s)",
            34: "Speex 11 (kbit/s)",
            35: "Speex 15 (kbit/s)",
            36: "Speex 18.2 (kbit/s)",
            37: "Speex 24.6 (kbit/s)",
            40: "Codec2 0.45 (kbit/s)",
            41: "Codec2 0.70 (kbit/s)",
            42: "Codec2 1.2 (kbit/s)",
            43: "Codec2 1.3 (kbit/s)",
            44: "Codec2 1.4 (kbit/s)",
            45: "Codec2 1.6 (kbit/s)",
            46: "Codec2 2.4 (kbit/s)",
            47: "Codec2 3.2 (kbit/s)",
            50: "MELPe 0.60 (kbit/s)",
            51: "MELPe 1.2 (kbit/s)",
            52: "MELPe 2.4 (kbit/s)",
            60: "CVSD (Continuous Variable Slope Delta Modulation)"
        }
        return descriptions.get(encoder, f"Unknown encoder ({encoder})")
    
    def show_audio_encoders(self):
        """Display valid audio encoder values and descriptions."""
        print("\nValid Audio Encoders:")
        print("\n  Special:")
        print("    -1 - External (Externally implemented)")
        print("     0 - Unknown Codec type")
        print("\n  G.711:")
        print("     1 - G.711 U-Law 64 (kbit/s)")
        print("     2 - G.711 A-Law 64 (kbit/s)")
        print("\n  GSM:")
        print("     3 - GSM Full Rate 13.2 (kbit/s)")
        print("\n  G.729:")
        print("     4 - G.729a 8 (kbit/s)")
        print("\n  PCM:")
        print("     5 - PCM")
        print("\n  AMR Narrowband:")
        print("    10 - AMR Narrowband 4.75 (kbit/s)")
        print("    11 - AMR Narrowband 5.15 (kbit/s)")
        print("    12 - AMR Narrowband 5.9 (kbit/s)")
        print("    13 - AMR Narrowband 6.7 (kbit/s)")
        print("    14 - AMR Narrowband 7.4 (kbit/s)")
        print("    15 - AMR Narrowband 7.95 (kbit/s)")
        print("    16 - AMR Narrowband 10.2 (kbit/s)")
        print("    17 - AMR Narrowband 12.2 (kbit/s)")
        print("\n  Opus:")
        print("    20 - Opus 6 (kbit/s)")
        print("    21 - Opus 8 (kbit/s)")
        print("    22 - Opus 10 (kbit/s)")
        print("    23 - Opus 12 (kbit/s)")
        print("    24 - Opus 14 (kbit/s)")
        print("    25 - Opus 16 (kbit/s)")
        print("    26 - Opus 18 (kbit/s)")
        print("    27 - Opus 20 (kbit/s)")
        print("    28 - Opus 22 (kbit/s)")
        print("    29 - Opus 24 (kbit/s)")
        print("\n  Speex:")
        print("    30 - Speex 2.15 (kbit/s)")
        print("    31 - Speex 3.95 (kbit/s)")
        print("    32 - Speex 5.95 (kbit/s)")
        print("    33 - Speex 8 (kbit/s)")
        print("    34 - Speex 11 (kbit/s)")
        print("    35 - Speex 15 (kbit/s)")
        print("    36 - Speex 18.2 (kbit/s)")
        print("    37 - Speex 24.6 (kbit/s)")
        print("\n  Codec2:")
        print("    40 - Codec2 0.45 (kbit/s)")
        print("    41 - Codec2 0.70 (kbit/s)")
        print("    42 - Codec2 1.2 (kbit/s)")
        print("    43 - Codec2 1.3 (kbit/s)")
        print("    44 - Codec2 1.4 (kbit/s)")
        print("    45 - Codec2 1.6 (kbit/s)")
        print("    46 - Codec2 2.4 (kbit/s)")
        print("    47 - Codec2 3.2 (kbit/s)")
        print("\n  MELPe:")
        print("    50 - MELPe 0.60 (kbit/s)")
        print("    51 - MELPe 1.2 (kbit/s)")
        print("    52 - MELPe 2.4 (kbit/s)")
        print("\n  CVSD:")
        print("    60 - CVSD (Continuous Variable Slope Delta Modulation)")
        print()
    
    def get_network_interfaces(self) -> List[str]:
        """Get list of available network interfaces that support multicast."""
        interfaces = []
        
        # Try 'ip' command (Linux) - best for checking MULTICAST flag
        try:
            result = subprocess.run(['ip', 'link', 'show'], 
                                  capture_output=True, text=True, timeout=2)
            if result.returncode == 0:
                for line in result.stdout.split('\n'):
                    # Match lines like "2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP>"
                    # Only include if MULTICAST is in the flags
                    if 'MULTICAST' in line:
                        match = re.search(r'^\d+:\s+([^:]+):', line)
                        if match:
                            iface_name = match.group(1).strip()
                            # Skip loopback interfaces (lo, lo0)
                            if iface_name and iface_name not in interfaces and not iface_name.startswith('lo'):
                                interfaces.append(iface_name)
        except (FileNotFoundError, subprocess.TimeoutExpired):
            pass
        
        # Try 'ifconfig' command (macOS, Linux) - check for MULTICAST flag
        if not interfaces:
            try:
                result = subprocess.run(['ifconfig'], 
                                      capture_output=True, text=True, timeout=2)
                if result.returncode == 0:
                    current_iface = None
                    for line in result.stdout.split('\n'):
                        # Match interface name line like "en0: flags=8863<UP,BROADCAST,SMART,RUNNING,SIMPLEX,MULTICAST>"
                        iface_match = re.search(r'^([a-zA-Z0-9]+):\s+flags=', line)
                        if iface_match:
                            current_iface = iface_match.group(1).strip()
                            # Check if MULTICAST is in the flags
                            if 'MULTICAST' in line and current_iface:
                                # Skip loopback interfaces
                                if not current_iface.startswith('lo'):
                                    if current_iface not in interfaces:
                                        interfaces.append(current_iface)
                        # Also check for MULTICAST in subsequent lines (some systems format differently)
                        elif current_iface and 'MULTICAST' in line:
                            if not current_iface.startswith('lo') and current_iface not in interfaces:
                                interfaces.append(current_iface)
            except (FileNotFoundError, subprocess.TimeoutExpired):
                pass
        
        # Fallback: Try socket.if_nameindex() and check flags (Linux)
        if not interfaces:
            try:
                if hasattr(socket, 'if_nameindex'):
                    for iface in socket.if_nameindex():
                        iface_name = iface[1]
                        # Skip loopback
                        if iface_name.startswith('lo'):
                            continue
                        # Try to get interface flags using socket
                        try:
                            # On Linux, we can check /sys/class/net/<iface>/flags
                            flags_path = f'/sys/class/net/{iface_name}/flags'
                            if os.path.exists(flags_path):
                                with open(flags_path, 'r') as f:
                                    flags = int(f.read().strip(), 16)
                                    # IFF_MULTICAST flag is 0x1000
                                    if flags & 0x1000:
                                        interfaces.append(iface_name)
                        except (OSError, ValueError):
                            # If we can't check flags, include it (better to show more than less)
                            interfaces.append(iface_name)
            except (AttributeError, OSError):
                pass
        
        # Sort and return
        return sorted(interfaces) if interfaces else []
    
    def show_network_interfaces(self):
        """Display available network interfaces."""
        interfaces = self.get_network_interfaces()
        if interfaces:
            print("\nAvailable Network Interfaces:")
            for iface in interfaces:
                print(f"  - {iface}")
        else:
            print("\nNo network interfaces found. You may need to enter the interface name manually.")
    
    def get_interface_name(self, default: str = None) -> str:
        """Get interface name from user with validation and help."""
        while True:
            prompt = "Interface name (? for available interfaces)"
            if default:
                prompt += f" [{default}]"
            value = self.safe_input(f"{prompt}: ").strip()
            
            if not value:
                if default:
                    return default
                print("Error: Interface name is required for multicast groups.")
                continue
            
            if value == '?':
                self.show_network_interfaces()
                continue
            
            # Basic validation - interface names are typically alphanumeric with some special chars
            if re.match(r'^[a-zA-Z0-9_\-\.]+$', value):
                return value
            else:
                print("Error: Invalid interface name format.")
    
    def get_audio_encoder(self, default: Optional[int] = 25) -> int:
        """Get audio encoder from user with validation and help.
        
        Args:
            default: Default encoder value. If None, no default is used and input is required.
        """
        valid_encoders = [-1, 0, 1, 2, 3, 4, 5, 10, 11, 12, 13, 14, 15, 16, 17,
                         20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                         30, 31, 32, 33, 34, 35, 36, 37,
                         40, 41, 42, 43, 44, 45, 46, 47,
                         50, 51, 52, 60]
        while True:
            if default is not None:
                # Show current encoder in brackets in the prompt
                current_desc = self.get_audio_encoder_description(default)
                prompt = f"Audio encoder (? for help) [{default} - {current_desc}]"
            else:
                # No default - require input
                prompt = "Audio encoder (? for help)"
            value = self.safe_input(f"{prompt}: ").strip()
            
            if not value:
                if default is not None:
                    return default
                else:
                    print("Error: Audio encoder is required. Please enter a value or ? for help.")
                    continue
            if value == '?':
                self.show_audio_encoders()
                continue
            try:
                encoder = int(value)
                if encoder in valid_encoders:
                    return encoder
                else:
                    print(f"Error: Invalid encoder value. Valid values: {valid_encoders}")
            except ValueError:
                print("Error: Please enter a valid number or ? for help.")
    
    def show_main_config(self):
        """Display the main EBS configuration in color-coded JSON format."""
        self.clear_screen()
        self.display_permission_warnings()
        
        # Prepare header lines
        header_lines = [
            f"\n{self.header('=== EBS Main Configuration JSON ===')}",
            f"File: {self.main_config_path}",
            f"{self.header('=' * 40)}"
        ]
        
        try:
            # Pretty print the JSON with color coding
            json_str = json.dumps(self.main_config, indent=2, ensure_ascii=False)
            parsed_data = json.loads(json_str)
            self.display_json_colorized_with_pager(parsed_data, header_lines)
        except Exception as e:
            print(f"{self.error('✗ Error displaying JSON:')} {e}")
            self.safe_input("\nPress Enter to continue...")
            return
        
        # If pager was used, it will have already exited
        # If not, we still need to wait for user input
        self.safe_input("\nPress Enter to continue...")
    
    def display_groups_list(self, start_number=1, show_numbers=False):
        """Display groups in table format (for menu display).
        
        Args:
            start_number: Starting number for numbering sequence
            show_numbers: Whether to show numbers in first column
            
        Returns:
            Dictionary mapping numbers to group objects
        """
        groups = self.bridges_config.get('groups', [])
        
        if not groups:
            print("Current Groups:")
            if show_numbers:
                print(f"{'#':<4} {'ID':<10} {'Name':<8} {'Type':<8} {'Encoder':<8} {'Multicast':<8} {'Rallypoint':<10} {'Used In':<8}")
                print("-" * (4 + 10 + 8 + 8 + 8 + 8 + 10 + 8 + 7))  # +7 for spaces between columns
            else:
                print(f"{'ID':<10} {'Name':<8} {'Type':<8} {'Encoder':<8} {'Multicast':<8} {'Rallypoint':<10} {'Used In':<8}")
                print("-" * (10 + 8 + 8 + 8 + 8 + 10 + 8 + 6))  # +6 for spaces between columns
            print("No groups configured.")
            print()
            return {}
        
        if groups:
            # Build a map of which bridges use each group
            group_to_bridges = {}
            if 'bridges' in self.bridges_config:
                for bridge in self.bridges_config['bridges']:
                    bridge_id = bridge.get('id', 'Unknown')
                    for group_id in bridge.get('groups', []):
                        if group_id not in group_to_bridges:
                            group_to_bridges[group_id] = []
                        group_to_bridges[group_id].append(bridge_id)
            
            # Prepare all data first to calculate column widths
            rows = []
            group_mapping = {}
            for i, group in enumerate(self.bridges_config['groups'], start_number):
                group_id = group.get('id', 'N/A')
                name = group.get('name', 'N/A')
                
                # Type
                if 'type' in group:
                    type_val = group['type']
                    type_desc = self.get_group_type_description(type_val)
                    type_str = f"{type_val} ({type_desc})"
                else:
                    type_str = "N/A"
                
                # Encoder (with payload transformation info if configured)
                if 'txAudio' in group and 'encoder' in group['txAudio']:
                    encoder_val = group['txAudio']['encoder']
                    encoder_desc = self.get_audio_encoder_description(encoder_val)
                    encoder_str = f"{encoder_val} ({encoder_desc})"
                    
                    # Add payload transformation info if configured
                    if 'customRtpPayloadType' in group['txAudio']:
                        custom_payload = group['txAudio']['customRtpPayloadType']
                        
                        # Try to determine radio type from payload mappings
                        radio_type = "Custom"
                        if 'inboundRtpPayloadTypeTranslations' in group and group['inboundRtpPayloadTypeTranslations']:
                            external_payload = group['inboundRtpPayloadTypeTranslations'][0].get('external')
                            engage_payload = group['inboundRtpPayloadTypeTranslations'][0].get('engage')
                            
                            # Check if this matches a known radio type mapping
                            for radio in ['tsm', 'mpu']:
                                mapping = self.get_payload_mapping(encoder_val, radio)
                                if mapping and mapping['external'] == external_payload and mapping['engage'] == engage_payload:
                                    radio_type = radio.upper()
                                    break
                        
                        encoder_str += f" [RTP:{custom_payload} {radio_type}]"
                else:
                    encoder_str = "N/A"
                
                # Multicast
                has_multicast = 'rx' in group and 'tx' in group
                multicast_str = "Yes" if has_multicast else "No"
                
                # Rallypoints
                has_rallypoints = 'rallypoints' in group and group['rallypoints']
                if has_rallypoints:
                    rallypoint_str = f"Yes ({len(group['rallypoints'])})"
                else:
                    rallypoint_str = "No"
                
                # Bridges using this group
                used_in = group_to_bridges.get(group_id, [])
                if used_in:
                    bridges_str = ', '.join(used_in)
                else:
                    bridges_str = "None"
                
                rows.append((i, group_id, name, type_str, encoder_str, multicast_str, rallypoint_str, bridges_str))
                group_mapping[i] = group
            
            # Calculate column widths (minimum width is header length)
            if show_numbers:
                col_widths = [
                    4,  # # column
                    max(len('ID'), max(len(row[1]) for row in rows), 10),
                    max(len('Name'), max(len(row[2]) for row in rows), 8),
                    max(len('Type'), max(len(row[3]) for row in rows), 8),
                    max(len('Encoder'), max(len(row[4]) for row in rows), 8),
                    max(len('Multicast'), max(len(row[5]) for row in rows), 8),
                    max(len('Rallypoint'), max(len(row[6]) for row in rows), 10),
                    max(len('Used In'), max(len(row[7]) for row in rows), 8)
                ]
            else:
                col_widths = [
                    max(len('ID'), max(len(row[1]) for row in rows), 10),
                    max(len('Name'), max(len(row[2]) for row in rows), 8),
                    max(len('Type'), max(len(row[3]) for row in rows), 8),
                    max(len('Encoder'), max(len(row[4]) for row in rows), 8),
                    max(len('Multicast'), max(len(row[5]) for row in rows), 8),
                    max(len('Rallypoint'), max(len(row[6]) for row in rows), 10),
                    max(len('Used In'), max(len(row[7]) for row in rows), 8)
                ]
            
            print(f"{self.info('Current Groups:')}")
            # Header
            if show_numbers:
                header_line = f"{'#':<{col_widths[0]}} {'ID':<{col_widths[1]}} {'Name':<{col_widths[2]}} {'Type':<{col_widths[3]}} {'Encoder':<{col_widths[4]}} {'Multicast':<{col_widths[5]}} {'Rallypoint':<{col_widths[6]}} {'Used In':<{col_widths[7]}}"
                print(self.header(header_line))
                print(self.dim("-" * (sum(col_widths) + 7)))  # +7 for spaces between columns
                
                # Group rows
                for row in rows:
                    print(f"{self.highlight(f'{row[0]:<{col_widths[0]}}')} {row[1]:<{col_widths[1]}} {row[2]:<{col_widths[2]}} {row[3]:<{col_widths[3]}} {row[4]:<{col_widths[4]}} {row[5]:<{col_widths[5]}} {row[6]:<{col_widths[6]}} {row[7]:<{col_widths[7]}}")
            else:
                header_line = f"{'ID':<{col_widths[0]}} {'Name':<{col_widths[1]}} {'Type':<{col_widths[2]}} {'Encoder':<{col_widths[3]}} {'Multicast':<{col_widths[4]}} {'Rallypoint':<{col_widths[5]}} {'Used In':<{col_widths[6]}}"
                print(self.header(header_line))
                print(self.dim("-" * (sum(col_widths) + 6)))  # +6 for spaces between columns
                
                # Group rows
                for row in rows:
                    print(f"{row[1]:<{col_widths[0]}} {row[2]:<{col_widths[1]}} {row[3]:<{col_widths[2]}} {row[4]:<{col_widths[3]}} {row[5]:<{col_widths[4]}} {row[6]:<{col_widths[5]}} {row[7]:<{col_widths[6]}}")
            print()
            
            return group_mapping
    
    def build_item_mapping(self):
        """Create a dictionary mapping numbers to (type, item_id, item_object) tuples.
        
        Returns:
            Dictionary mapping display numbers to ('bridge'/'group', item_id, item_object)
        """
        mapping = {}
        current_number = 1
        
        # Add bridges first
        bridges = self.bridges_config.get('bridges', [])
        for bridge in bridges:
            mapping[current_number] = ('bridge', bridge.get('id'), bridge)
            current_number += 1
        
        # Add groups
        groups = self.bridges_config.get('groups', [])
        for group in groups:
            mapping[current_number] = ('group', group.get('id'), group)
            current_number += 1
            
        return mapping
    
    def get_item_by_number(self, number: int):
        """Retrieve bridge or group by its display number.
        
        Args:
            number: Display number from the unified table
            
        Returns:
            Tuple of (type, item_id, item_object) or None if not found
        """
        mapping = self.build_item_mapping()
        return mapping.get(number)
    
    def get_item_type(self, number: int) -> str:
        """Determine if a number corresponds to a bridge or group.
        
        Args:
            number: Display number from the unified table
            
        Returns:
            'bridge', 'group', or None if not found
        """
        item = self.get_item_by_number(number)
        return item[0] if item else None

    def get_group_info(self, group: Dict[str, Any]) -> str:
        """Get info string for a single group."""
        info_parts = []
        
        # Rallypoint info
        if 'rallypoints' in group and group['rallypoints']:
            for rp in group['rallypoints']:
                if 'host' in rp:
                    addr = rp['host'].get('address', '')
                    port = rp['host'].get('port', '')
                    if addr and port:
                        info_parts.append(f"{self.dim('RP:')}{addr}:{port}")
                        break  # Only show first rallypoint
        
        # Multicast info
        if 'rx' in group:
            rx_addr = group['rx'].get('address', '')
            rx_port = group['rx'].get('port', '')
            if rx_addr and rx_port:
                if 'tx' in group:
                    tx_addr = group['tx'].get('address', '')
                    tx_port = group['tx'].get('port', '')
                    if tx_addr == rx_addr and tx_port == rx_port:
                        # Same RX and TX, combine
                        info_parts.append(f"{self.dim('MC:')}{rx_addr}:{rx_port}")
                    else:
                        # Different RX and TX
                        rx_str = f"RX {rx_addr}:{rx_port}" if rx_addr and rx_port else ""
                        tx_str = f"TX {tx_addr}:{tx_port}" if tx_addr and tx_port else ""
                        if rx_str and tx_str:
                            info_parts.append(f"{self.dim('MC:')}{rx_str}, {tx_str}")
                        elif rx_str:
                            info_parts.append(f"{self.dim('MC:')}{rx_str}")
                        elif tx_str:
                            info_parts.append(f"{self.dim('MC:')}{tx_str}")
                else:
                    info_parts.append(f"{self.dim('MC:')}{rx_addr}:{rx_port}")
        
        # Interface name
        if 'interfaceName' in group and group['interfaceName']:
            info_parts.append(f"{self.dim('IF:')}{group['interfaceName']}")
        
        # Encoder (from txAudio) - payload transformation info now shown in detailed tables
        if 'txAudio' in group and 'encoder' in group['txAudio']:
            encoder_val = group['txAudio']['encoder']
            encoder_desc = self.get_audio_encoder_description(encoder_val)
            encoder_str = f"{self.dim('Enc:')}{encoder_val} ({encoder_desc})"
            
            # Add payload transformation info for compact display (bridges table)
            if 'customRtpPayloadType' in group['txAudio']:
                custom_payload = group['txAudio']['customRtpPayloadType']
                
                # Try to determine radio type from payload mappings
                radio_type = "Custom"
                if 'inboundRtpPayloadTypeTranslations' in group and group['inboundRtpPayloadTypeTranslations']:
                    external_payload = group['inboundRtpPayloadTypeTranslations'][0].get('external')
                    engage_payload = group['inboundRtpPayloadTypeTranslations'][0].get('engage')
                    
                    # Check if this matches a known radio type mapping
                    for radio in ['tsm', 'mpu']:
                        mapping = self.get_payload_mapping(encoder_val, radio)
                        if mapping and mapping['external'] == external_payload and mapping['engage'] == engage_payload:
                            radio_type = radio.upper()
                            break
                
                encoder_str += f" [RTP:{custom_payload} {radio_type}]"
            
            info_parts.append(encoder_str)
        
        # noHdrExt (from txAudio) - only show if True
        if 'txAudio' in group and group['txAudio'].get('noHdrExt') is True:
            info_parts.append(f"{self.dim('noHdrExt:')}{True}")
        
        return ' '.join(info_parts) if info_parts else "No config"
    
    def display_bridges_table(self, bridges_list=None, show_numbers=False, show_wizard_indicator=True, start_number=1):
        """Display bridges in table format with each group on a new line.
        
        Args:
            bridges_list: List of bridges to display
            show_numbers: Whether to show numbers in first column
            show_wizard_indicator: Whether to show * for manually created bridges
            start_number: Starting number for numbering sequence
            
        Returns:
            Next available number for continuing sequence
        """
        if bridges_list is None:
            bridges_list = self.bridges_config.get('bridges', [])
        
        # Check if there are any manually created bridges (only show indicator column if there are)
        has_manual_bridges = any(self.get_wizard_type(bridge) is None for bridge in bridges_list) if bridges_list else False
        actual_show_indicator = show_wizard_indicator and has_manual_bridges
        
        # Calculate column widths - use defaults for empty list
        wizard_col_width = 2 if actual_show_indicator else 0  # "* " or empty
        enabled_width = max(len('En'), 2)  # "En" or "Y"/"N"
        if bridges_list:
            id_width = max(len('ID'), max(len(bridge.get('id', 'N/A')) for bridge in bridges_list), 10)
            groups_width = max(len('Groups'), max(len(group_id) for bridge in bridges_list for group_id in bridge.get('groups', []) or ['None']), 15)
            
            # Calculate info width - need to check all groups in all bridges
            all_groups = self.bridges_config.get('groups', [])
            info_strings = []
            for bridge in bridges_list:
                groups = bridge.get('groups', [])
                for group_id in groups:
                    group = next((g for g in all_groups if g.get('id') == group_id), None)
                    if group:
                        info_strings.append(self.get_group_info(group))
            info_width = max(len('Info'), max(self.get_visual_length(info) for info in info_strings) if info_strings else 0, 20)
        else:
            # Default widths for empty table
            id_width = max(len('ID'), 10)
            groups_width = max(len('Groups'), 15)
            info_width = max(len('Info'), 20)
        
        # Header
        if show_numbers:
            if actual_show_indicator:
                header_line = f"{'#':<4} {'*':<{wizard_col_width}} {'ID':<{id_width}} {'En':<{enabled_width}} {'Groups':<{groups_width}} {'Info':<{info_width}}"
                print(self.header(header_line))
                print(self.dim("-" * (4 + wizard_col_width + id_width + enabled_width + groups_width + info_width + 5)))  # +5 for spaces
            else:
                header_line = f"{'#':<4} {'ID':<{id_width}} {'En':<{enabled_width}} {'Groups':<{groups_width}} {'Info':<{info_width}}"
                print(self.header(header_line))
                print(self.dim("-" * (4 + id_width + enabled_width + groups_width + info_width + 4)))  # +4 for spaces
        else:
            if actual_show_indicator:
                header_line = f"{'*':<{wizard_col_width}} {'ID':<{id_width}} {'En':<{enabled_width}} {'Groups':<{groups_width}} {'Info':<{info_width}}"
                print(self.header(header_line))
                print(self.dim("-" * (wizard_col_width + id_width + enabled_width + groups_width + info_width + 4)))  # +4 for spaces
            else:
                header_line = f"{'ID':<{id_width}} {'En':<{enabled_width}} {'Groups':<{groups_width}} {'Info':<{info_width}}"
                print(self.header(header_line))
                print(self.dim("-" * (id_width + enabled_width + groups_width + info_width + 3)))  # +3 for spaces
        
        # Print rows - each group on a new line, ID only shown once
        all_groups = self.bridges_config.get('groups', [])
        
        if not bridges_list:
            # Show empty table message
            empty_message = "No bridges configured."
            if show_numbers:
                if actual_show_indicator:
                    print(f"{'':<4} {'':<{wizard_col_width}} {empty_message:<{id_width + enabled_width + groups_width + info_width + 3}}")
                else:
                    print(f"{'':<4} {empty_message:<{id_width + enabled_width + groups_width + info_width + 3}}")
            else:
                if actual_show_indicator:
                    print(f"{'':<{wizard_col_width}} {empty_message:<{id_width + enabled_width + groups_width + info_width + 3}}")
                else:
                    print(f"{empty_message:<{id_width + enabled_width + groups_width + info_width + 3}}")
        
        for i, bridge in enumerate(bridges_list, start_number):
            bridge_id = bridge.get('id', 'N/A')
            groups = bridge.get('groups', [])
            is_wizard = self.get_wizard_type(bridge) is not None
            wizard_indicator = "*" if not is_wizard and actual_show_indicator else ""
            enabled_status = "Y" if bridge.get('enabled', True) else "N"
            
            if not groups:
                # No groups - show ID and "None"
                if show_numbers:
                    if actual_show_indicator:
                        print(f"{i:<4} {wizard_indicator:<{wizard_col_width}} {bridge_id:<{id_width}} {enabled_status:<{enabled_width}} {'None':<{groups_width}} {'No config':<{info_width}}")
                    else:
                        print(f"{i:<4} {bridge_id:<{id_width}} {enabled_status:<{enabled_width}} {'None':<{groups_width}} {'No config':<{info_width}}")
                else:
                    if actual_show_indicator:
                        print(f"{wizard_indicator:<{wizard_col_width}} {bridge_id:<{id_width}} {enabled_status:<{enabled_width}} {'None':<{groups_width}} {'No config':<{info_width}}")
                    else:
                        print(f"{bridge_id:<{id_width}} {enabled_status:<{enabled_width}} {'None':<{groups_width}} {'No config':<{info_width}}")
            else:
                # First group - show ID with wizard indicator and group info
                first_group_id = groups[0]
                first_group = next((g for g in all_groups if g.get('id') == first_group_id), None)
                first_group_info = self.get_group_info(first_group) if first_group else "No config"
                
                if show_numbers:
                    if actual_show_indicator:
                        print(f"{self.highlight(f'{i:<4}')} {wizard_indicator:<{wizard_col_width}} {bridge_id:<{id_width}} {enabled_status:<{enabled_width}} {first_group_id:<{groups_width}} {first_group_info:<{info_width}}")
                    else:
                        print(f"{self.highlight(f'{i:<4}')} {bridge_id:<{id_width}} {enabled_status:<{enabled_width}} {first_group_id:<{groups_width}} {first_group_info:<{info_width}}")
                else:
                    if actual_show_indicator:
                        print(f"{wizard_indicator:<{wizard_col_width}} {bridge_id:<{id_width}} {enabled_status:<{enabled_width}} {first_group_id:<{groups_width}} {first_group_info:<{info_width}}")
                    else:
                        print(f"{bridge_id:<{id_width}} {enabled_status:<{enabled_width}} {first_group_id:<{groups_width}} {first_group_info:<{info_width}}")
                
                # Remaining groups - show empty ID, wizard, and enabled columns, but show group info
                for group_id in groups[1:]:
                    group = next((g for g in all_groups if g.get('id') == group_id), None)
                    group_info = self.get_group_info(group) if group else "No config"
                    
                    if show_numbers:
                        if actual_show_indicator:
                            # 4 spaces for number column + 1 space + wizard column + 1 space + ID column + 1 space + enabled column + 1 space
                            spaces_before_groups = ' ' * (4 + 1 + wizard_col_width + 1 + id_width + 1 + enabled_width + 1)
                            print(f"{spaces_before_groups}{group_id:<{groups_width}} {group_info:<{info_width}}")
                        else:
                            # 4 spaces for number column + 1 space + ID column + 1 space + enabled column + 1 space
                            spaces_before_groups = ' ' * (4 + 1 + id_width + 1 + enabled_width + 1)
                            print(f"{spaces_before_groups}{group_id:<{groups_width}} {group_info:<{info_width}}")
                    else:
                        if actual_show_indicator:
                            # wizard column + 1 space + ID column + 1 space + enabled column + 1 space
                            spaces_before_groups = ' ' * (wizard_col_width + 1 + id_width + 1 + enabled_width + 1)
                            print(f"{spaces_before_groups}{group_id:<{groups_width}} {group_info:<{info_width}}")
                        else:
                            # ID column + 1 space + enabled column + 1 space
                            spaces_before_groups = ' ' * (id_width + 1 + enabled_width + 1)
                            print(f"{spaces_before_groups}{group_id:<{groups_width}} {group_info:<{info_width}}")
            
            # Add blank line between bridges (except for the last one)
            if i < len(bridges_list):
                print()
        
        # Show legend if manually created bridges exist and indicator is shown
        if actual_show_indicator:
            print("* = Manually created")
        print()
        
        # Return next available number for continuing sequence
        return start_number + len(bridges_list)
    
    def display_groups_table(self, groups_list, show_numbers=False, show_status=False, current_groups=None):
        """Display groups in table format with optional numbers and status."""
        # Prepare all data first to calculate column widths
        rows = []
        if groups_list:
            for i, group in enumerate(groups_list, 1):
                group_id = group.get('id', 'N/A')
                name = group.get('name', 'N/A')
                status = ""
                if show_status and current_groups:
                    status = "[CURRENT]" if group_id in current_groups else ""
                rows.append((i, group_id, name, status))
        
        # Calculate column widths
        if rows:
            id_width = max(len('ID'), max(len(row[1]) for row in rows), 10)
            name_width = max(len('Name'), max(len(row[2]) for row in rows), 8)
            status_width = 0
            if show_status:
                status_width = max(len('Status'), max(len(row[3]) for row in rows), 10)
        else:
            # Default widths for empty table
            id_width = max(len('ID'), 10)
            name_width = max(len('Name'), 8)
            status_width = max(len('Status'), 10) if show_status else 0
        
        # Header
        if show_numbers:
            if show_status:
                print(f"{'#':<4} {'ID':<{id_width}} {'Name':<{name_width}} {'Status':<{status_width}}")
                print("-" * (4 + id_width + name_width + status_width + 3))  # +3 for spaces
            else:
                print(f"{'#':<4} {'ID':<{id_width}} {'Name':<{name_width}}")
                print("-" * (4 + id_width + name_width + 2))  # +2 for spaces
        else:
            print(f"{'ID':<{id_width}} {'Name':<{name_width}}")
            print("-" * (id_width + name_width + 1))  # +1 for space
        
        # Group rows
        if not rows:
            # Show empty table message
            empty_message = "No groups configured."
            if show_numbers:
                if show_status:
                    print(f"{'':<4} {empty_message:<{id_width + name_width + status_width + 2}}")
                else:
                    print(f"{'':<4} {empty_message:<{id_width + name_width + 1}}")
            else:
                print(f"{empty_message:<{id_width + name_width + 1}}")
        
        for row in rows:
            if show_numbers:
                if show_status:
                    print(f"{row[0]:<4} {row[1]:<{id_width}} {row[2]:<{name_width}} {row[3]:<{status_width}}")
                else:
                    print(f"{row[0]:<4} {row[1]:<{id_width}} {row[2]:<{name_width}}")
            else:
                print(f"{row[1]:<{id_width}} {row[2]:<{name_width}}")
        print()
    
    def display_bridges_list(self):
        """Display bridges in table format (for menu display)."""
        print("Current Bridges:")
        self.display_bridges_table()
    
    def show_bridges(self):
        """Display all bridges."""
        self.clear_screen()
        print("\n=== Bridges ===\n")
        if 'bridges' not in self.bridges_config or not self.bridges_config['bridges']:
            print("No bridges configured.")
        else:
            for i, bridge in enumerate(self.bridges_config['bridges'], 1):
                print(f"{i}. Bridge ID: {bridge.get('id', 'N/A')}")
                print(f"   Groups: {', '.join(bridge.get('groups', []))}")
                print()
        input("\nPress Enter to continue...")
    
    def show_groups(self):
        """Display all groups with essential information."""
        self.clear_screen()
        print("\n=== Groups ===\n")
        if 'groups' not in self.bridges_config or not self.bridges_config['groups']:
            print("No groups configured.")
            input("\nPress Enter to continue...")
        else:
            # Build a map of which bridges use each group
            group_to_bridges = {}
            if 'bridges' in self.bridges_config:
                for bridge in self.bridges_config['bridges']:
                    bridge_id = bridge.get('id', 'Unknown')
                    for group_id in bridge.get('groups', []):
                        if group_id not in group_to_bridges:
                            group_to_bridges[group_id] = []
                        group_to_bridges[group_id].append(bridge_id)
            
            for i, group in enumerate(self.bridges_config['groups'], 1):
                group_id = group.get('id', 'N/A')
                
                print(f"{i}. id: {group_id}")
                
                # Name
                if 'name' in group:
                    print(f"   name: {group.get('name', 'N/A')}")
                
                # Type with description
                if 'type' in group:
                    type_val = group['type']
                    type_desc = self.get_group_type_description(type_val)
                    print(f"   type: {type_val} ({type_desc})")
                
                # Encoder with description
                if 'txAudio' in group and 'encoder' in group['txAudio']:
                    encoder_val = group['txAudio']['encoder']
                    encoder_desc = self.get_audio_encoder_description(encoder_val)
                    print(f"   encoder: {encoder_val} ({encoder_desc})")
                
                # Multicast configuration
                has_multicast = 'rx' in group and 'tx' in group
                print(f"   multicast: {'Yes' if has_multicast else 'No'}")
                if has_multicast:
                    rx = group['rx']
                    tx = group['tx']
                    print(f"      rx: {rx.get('address', 'N/A')}:{rx.get('port', 'N/A')}")
                    print(f"      tx: {tx.get('address', 'N/A')}:{tx.get('port', 'N/A')}")
                
                # Rallypoint configuration
                if 'rallypoints' in group and group['rallypoints']:
                    print(f"   rallypoints: Yes ({len(group['rallypoints'])} configured)")
                    for j, rp in enumerate(group['rallypoints'], 1):
                        host = rp.get('host', {})
                        addr = host.get('address', 'N/A')
                        port = host.get('port', 'N/A')
                        print(f"      {j}. {addr}:{port}")
                else:
                    print(f"   rallypoints: No")
                
                # Bridges using this group
                if group_id in group_to_bridges:
                    bridges = group_to_bridges[group_id]
                    print(f"   used in bridges: {', '.join(bridges)}")
                else:
                    print(f"   used in bridges: None")
                
                print()
            
            # Prompt for detailed view or return
            print("\nEnter group number for full details, or press Enter to return")
            choice = input("Group number: ").strip()
            
            if choice:
                try:
                    idx = int(choice) - 1
                    if 0 <= idx < len(self.bridges_config['groups']):
                        self.show_group_full_details(self.bridges_config['groups'][idx], group_to_bridges)
                    else:
                        print("Invalid group number.")
                        self.safe_input("Press Enter to continue...")
                except ValueError:
                    print("Invalid input.")
                    self.safe_input("Press Enter to continue...")
    
    def show_group_full_details(self, group: Dict[str, Any], group_to_bridges: Dict[str, List[str]]):
        """Display full details of a single group."""
        self.clear_screen()
        group_id = group.get('id', 'N/A')
        print(f"\n=== Full Details: {group_id} ===\n")
        
        # Basic info
        print(f"id: {group_id}")
        if 'name' in group:
            print(f"name: {group.get('name', 'N/A')}")
        
        # Type with description
        if 'type' in group:
            type_val = group['type']
            type_desc = self.get_group_type_description(type_val)
            print(f"type: {type_val} ({type_desc})")
        
        # Network configuration
        if 'interfaceName' in group:
            print(f"interfaceName: {group['interfaceName']}")
        if 'rx' in group:
            rx = group['rx']
            print(f"rx: {rx.get('address', 'N/A')}:{rx.get('port', 'N/A')}")
        if 'tx' in group:
            tx = group['tx']
            print(f"tx: {tx.get('address', 'N/A')}:{tx.get('port', 'N/A')}")
        
        # Multicast options
        if 'enableMulticastFailover' in group:
            print(f"enableMulticastFailover: {group['enableMulticastFailover']}")
        
        # TX Options
        if 'txOptions' in group:
            tx_opts = group['txOptions']
            if 'ttl' in tx_opts:
                print(f"txOptions.ttl: {tx_opts['ttl']}")
            if 'priority' in tx_opts:
                print(f"txOptions.priority: {tx_opts['priority']}")
        
        # Rallypoints
        if 'rallypoints' in group and group['rallypoints']:
            print(f"rallypoints ({len(group['rallypoints'])}):")
            for j, rp in enumerate(group['rallypoints'], 1):
                host = rp.get('host', {})
                addr = host.get('address', 'N/A')
                port = host.get('port', 'N/A')
                print(f"   {j}. host.address: {addr}, host.port: {port}")
        
        # Audio configuration
        if 'txAudio' in group:
            tx_audio = group['txAudio']
            print(f"txAudio:")
            if 'encoder' in tx_audio:
                encoder_val = tx_audio['encoder']
                encoder_desc = self.get_audio_encoder_description(encoder_val)
                print(f"   encoder: {encoder_val} ({encoder_desc})")
            if 'fdx' in tx_audio:
                print(f"   fdx: {tx_audio['fdx']}")
            if 'framingMs' in tx_audio:
                print(f"   framingMs: {tx_audio['framingMs']}")
            if 'maxTxSecs' in tx_audio:
                print(f"   maxTxSecs: {tx_audio['maxTxSecs']}")
            if 'noHdrExt' in tx_audio:
                print(f"   noHdrExt: {tx_audio['noHdrExt']}")
        
        # Encryption
        if 'cryptoPassword' in group and group['cryptoPassword']:
            crypto = group['cryptoPassword']
            # Show first and last few chars for security
            if len(crypto) > 20:
                display_crypto = f"{crypto[:10]}...{crypto[-10:]}"
            else:
                display_crypto = crypto
            print(f"cryptoPassword: {display_crypto}")
        
        # Bridges using this group
        if group_id in group_to_bridges:
            bridges = group_to_bridges[group_id]
            print(f"\nused in bridges: {', '.join(bridges)}")
        else:
            print(f"\nused in bridges: None")
        
        input("\nPress Enter to continue...")
    
    def edit_main_config(self):
        """Interactive editor for main EBS configuration."""
        while True:
            self.clear_screen()
            # Display permission warnings first so they're always visible
            self.display_permission_warnings()
            # Calculate lengths to match copyright line
            copyright_text = 'Copyright (c) 2025 Rally Tactical Systems Inc'
            copyright_length = len(copyright_text)
            header_text = f' EBS Config v{VERSION} '
            # Calculate padding needed to center header and match copyright length
            padding_needed = copyright_length - len(header_text)
            left_padding = padding_needed // 2
            right_padding = padding_needed - left_padding
            header_padded = f"{'=' * left_padding}{header_text}{'=' * right_padding}"
            print(f"\n{self.header(header_padded)}")
            print(f"{self.info(copyright_text)}")
            print(f"Working on: {self.main_config_path}")
            # Separator matches header/copyright length
            print(f"{self.header('=' * copyright_length)}")
            print()
            print(f"{self.menu_option('1.')} Update License from File")
            print(f"{self.menu_option('2.')} General Settings")
            print(f"{self.menu_option('3.')} Status Report Settings")
            print(f"{self.menu_option('4.')} Health Check Responder")
            print(f"{self.menu_option('5.')} Multicast TX Options")
            print(f"{self.menu_option('6.')} TX Options")
            print(f"{self.menu_option('7.')} Show Main Config")
            print(f"{self.menu_option('b.')} Back (Esc)")
            print(f"{self.menu_option('q.')} Quit")
            
            choice = self.menu_input("\nSelect option: ").strip().lower()
            
            if choice == '1':
                self.update_license_from_file()
            elif choice == '2':
                self.edit_general_settings()
            elif choice == '3':
                self.edit_status_report()
            elif choice == '4':
                self.edit_health_check()
            elif choice == '5':
                self.edit_multicast_tx_options()
            elif choice == '6':
                self.edit_tx_options()
            elif choice == '7':
                self.show_main_config()
            elif choice == 'b':
                break
            elif choice == 'q':
                if self.handle_quit():
                    break
            else:
                print("Invalid option. Please try again.")
                self.safe_input("Press Enter to continue...")
    
    def edit_status_report(self):
        """Edit status report settings."""
        try:
            self.clear_screen()
            print("\n=== Status Report Settings ===\n")
            
            # Check for statusReport (ignore _statusReport)
            status_key = 'statusReport' if 'statusReport' in self.main_config else None
            if status_key is None:
                self.main_config['statusReport'] = {}
                status_key = 'statusReport'
            
            status = self.main_config[status_key]
            
            # Show current values
            print("Current configuration:")
            print(f"  enabled: {status.get('enabled', False)}")
            if 'fileName' in status:
                print(f"  fileName: {status.get('fileName', 'N/A')}")
            if 'intervalSecs' in status:
                print(f"  intervalSecs: {status.get('intervalSecs', 'N/A')}")
            if 'includeGroupDetail' in status:
                print(f"  includeGroupDetail: {status.get('includeGroupDetail', 'N/A')}")
            if 'includeBridgeDetail' in status:
                print(f"  includeBridgeDetail: {status.get('includeBridgeDetail', 'N/A')}")
            if 'includeBridgeGroupDetail' in status:
                print(f"  includeBridgeGroupDetail: {status.get('includeBridgeGroupDetail', 'N/A')}")
            if 'runCmd' in status:
                print(f"  runCmd: {status.get('runCmd', 'N/A')}")
            print()
            
            if not self._get_yes_no_with_abort("Do you want to edit these settings?", False):
                self.safe_input("Press Enter to continue...")
                return
            
            # Don't clear screen - keep showing current values
            print("\n--- Editing Status Report Settings ---\n")
            
            enabled = self._get_yes_no_with_abort("Enable status report", status.get('enabled', False))
            status['enabled'] = enabled
            
            if enabled:
                filename = self._get_input_with_abort("Status report file name", status.get('fileName', '/tmp/${id}_status.json'))
                status['fileName'] = filename
                
                interval = self._get_int_with_abort("Status report interval (seconds)", status.get('intervalSecs', 5))
                status['intervalSecs'] = interval
                
                status['includeGroupDetail'] = self._get_yes_no_with_abort("Include group detail", status.get('includeGroupDetail', True))
                status['includeBridgeDetail'] = self._get_yes_no_with_abort("Include bridge detail", status.get('includeBridgeDetail', True))
                status['includeBridgeGroupDetail'] = self._get_yes_no_with_abort("Include bridge group detail", status.get('includeBridgeGroupDetail', True))
                
                run_cmd = self._get_input_with_abort("Run command after status report (leave empty for none)", status.get('runCmd', ''))
                status['runCmd'] = run_cmd
            
            if self.save_main_config():
                input("\nPress Enter to continue...")
        except AbortOperationException:
            return
    
    def edit_health_check(self):
        """Edit health check responder settings."""
        try:
            self.clear_screen()
            print("\n=== Health Check Responder ===\n")
            
            if 'externalHealthCheckResponder' not in self.main_config:
                self.main_config['externalHealthCheckResponder'] = {}
            
            hc = self.main_config['externalHealthCheckResponder']
            
            # Show current values
            print("Current configuration:")
            print(f"  listenPort: {hc.get('listenPort', 0)}")
            if 'immediateClose' in hc:
                print(f"  immediateClose: {hc.get('immediateClose', 'N/A')}")
            print()
            
            if not self._get_yes_no_with_abort("Do you want to edit these settings?", False):
                self.safe_input("Press Enter to continue...")
                return
            
            # Don't clear screen - keep showing current values
            print("\n--- Editing Health Check Responder Settings ---\n")
            
            port = self._get_int_with_abort("Listen port (0 to disable)", hc.get('listenPort', 0))
            hc['listenPort'] = port
            
            if port > 0:
                immediate_close = self._get_yes_no_with_abort("Immediate close after connection", hc.get('immediateClose', True))
                hc['immediateClose'] = immediate_close
            
            if self.save_main_config():
                input("\nPress Enter to continue...")
        except AbortOperationException:
            return
    
    def edit_licensing(self):
        """Edit licensing settings."""
        self.clear_screen()
        print("\n=== Licensing ===\n")
        
        if 'enginePolicy' not in self.main_config:
            self.main_config['enginePolicy'] = {}
        if 'licensing' not in self.main_config['enginePolicy']:
            self.main_config['enginePolicy']['licensing'] = {}
        
        lic = self.main_config['enginePolicy']['licensing']
        
        # Show current values
        print("Current configuration:")
        print(f"  entitlement: {lic.get('entitlement', 'N/A')}")
        print(f"  key: {lic.get('key', 'N/A')}")
        print(f"  activationCode: {lic.get('activationCode', 'N/A')}")
        print()
        
        # Show featureset (read-only)
        if 'featureset' in self.main_config['enginePolicy']:
            featureset = self.main_config['enginePolicy']['featureset']
            print("Featureset (read-only):")
            if 'signature' in featureset:
                sig = featureset['signature']
                if len(sig) > 40:
                    display_sig = f"{sig[:20]}...{sig[-20:]}"
                else:
                    display_sig = sig
                print(f"  signature: {display_sig}")
            if 'features' in featureset and featureset['features']:
                print(f"  features: {len(featureset['features'])} feature(s) configured")
                for i, feature in enumerate(featureset['features'], 1):
                    count = feature.get('count', 'N/A')
                    comments = feature.get('comments', 'N/A')
                    print(f"    {i}. {comments} (count: {count})")
            print()
        
        if not self.get_yes_no("Do you want to edit licensing settings?", False):
            self.safe_input("Press Enter to continue...")
            return
        
        # Don't clear screen - keep showing current values
        print("\n--- Editing Licensing Settings ---\n")
        print("(Note: Featureset is read-only and cannot be edited)\n")
        
        entitlement = self.get_input("Entitlement GUID", lic.get('entitlement', ''))
        lic['entitlement'] = entitlement
        
        key = self.get_input("License key", lic.get('key', ''))
        lic['key'] = key
        
        activation = self.get_input("Activation code (leave empty if not needed)", lic.get('activationCode', ''))
        lic['activationCode'] = activation
        
        if self.save_main_config():
            input("\nPress Enter to continue...")
    
    def update_license_from_file(self):
        """Update license and featureset from a license file."""
        try:
            self.clear_screen()
            self.display_permission_warnings()
            print(f"\n{self.header('=== Update License from File ===')}")
            print()
            
            # Display current licensing information
            print(f"{self.info('Current License Information:')}")
            print()
            
            if 'enginePolicy' in self.main_config and 'licensing' in self.main_config['enginePolicy']:
                licensing_data = {
                    'licensing': self.main_config['enginePolicy']['licensing']
                }
                if 'featureset' in self.main_config['enginePolicy']:
                    licensing_data['featureset'] = self.main_config['enginePolicy']['featureset']
                
                try:
                    self.display_json_colorized(licensing_data)
                    print()
                except Exception as e:
                    print(f"{self.error('✗ Error displaying current license:')} {e}")
                    print()
            else:
                print(f"{self.dim('No license information currently configured.')}")
                print()
            
            print("This will update the license and featureset from a JSON license file.")
            print("The file should contain 'license' (or 'licensing') and 'featureset' objects.")
            print()
            
            if not self._get_yes_no_with_abort("Do you want to update license from file?", False):
                self.safe_input("Press Enter to continue...")
                return
            
            while True:
                license_file_path = self.safe_input("\nEnter path to license file (or 'cancel' to go back): ", detect_esc=True).strip()
                
                if license_file_path == 'ESC':
                    print()  # Move to next line before showing abort confirmation
                    self._confirm_abort()
                    continue
                
                if license_file_path.lower() == 'cancel':
                    print("License update cancelled.")
                    self.safe_input("Press Enter to continue...")
                    return
                
                if not license_file_path:
                    print("Please enter a file path or 'cancel'.")
                    continue
                
                # Try to load the license file
                license_path = Path(license_file_path)
                if not license_path.exists():
                    print(f"{self.error('✗ Error:')} File not found: {license_file_path}")
                    if not self._get_yes_no_with_abort("Try again?", True):
                        self.safe_input("Press Enter to continue...")
                        return
                    continue
                
                try:
                    with open(license_path, 'r') as f:
                        license_data = json.load(f)
                    
                    # Check for license and featureset in the file
                    if 'license' not in license_data and 'licensing' not in license_data:
                        print(f"{self.error('✗ Error:')} License file must contain 'license' or 'licensing' object.")
                        if not self._get_yes_no_with_abort("Try again?", True):
                            self.safe_input("Press Enter to continue...")
                            return
                        continue
                    
                    if 'featureset' not in license_data:
                        print(f"{self.error('✗ Error:')} License file must contain 'featureset' object.")
                        if not self._get_yes_no_with_abort("Try again?", True):
                            self.safe_input("Press Enter to continue...")
                            return
                        continue
                    
                    # Extract license data (try both 'license' and 'licensing' keys)
                    file_license = license_data.get('license') or license_data.get('licensing', {})
                    file_featureset = license_data.get('featureset', {})
                    
                    # Update enginePolicy
                    if 'enginePolicy' not in self.main_config:
                        self.main_config['enginePolicy'] = {}
                    
                    # Update licensing
                    if 'licensing' not in self.main_config['enginePolicy']:
                        self.main_config['enginePolicy']['licensing'] = {}
                    
                    # Merge license data
                    self.main_config['enginePolicy']['licensing'].update(file_license)
                    
                    # Update featureset - ensure it's placed directly after licensing
                    # Python 3.7+ preserves insertion order, so we rebuild the dict in the right order
                    engine_policy = self.main_config['enginePolicy']
                    new_engine_policy = {}
                    
                    # Add all keys before 'licensing'
                    for key, value in engine_policy.items():
                        if key == 'licensing':
                            break
                        if key != 'featureset':  # Skip old featureset if it exists
                            new_engine_policy[key] = value
                    
                    # Add licensing
                    new_engine_policy['licensing'] = engine_policy.get('licensing', {})
                    new_engine_policy['licensing'].update(file_license)
                    
                    # Add featureset right after licensing
                    new_engine_policy['featureset'] = file_featureset
                    
                    # Add all remaining keys (after licensing and featureset)
                    for key, value in engine_policy.items():
                        if key not in new_engine_policy and key != 'featureset':
                            new_engine_policy[key] = value
                    
                    self.main_config['enginePolicy'] = new_engine_policy
                    
                    if self.save_main_config():
                        print(f"\n{self.success('✓')} License and featureset updated successfully from file.")
                    else:
                        print(f"\n{self.error('✗')} Failed to save configuration.")
                    
                    self.safe_input("\nPress Enter to continue...")
                    return
                    
                except json.JSONDecodeError as e:
                    print(f"{self.error('✗ Error:')} Invalid JSON in license file: {e}")
                    if not self._get_yes_no_with_abort("Try again?", True):
                        self.safe_input("Press Enter to continue...")
                        return
                    continue
                except AbortOperationException:
                    raise
                except Exception as e:
                    print(f"{self.error('✗ Error:')} Error reading license file: {e}")
                    if not self._get_yes_no_with_abort("Try again?", True):
                        self.safe_input("Press Enter to continue...")
                        return
                    continue
        except AbortOperationException:
            return
    
    def edit_multicast_tx_options(self):
        """Edit multicast TX options."""
        try:
            self.clear_screen()
            print("\n=== Multicast TX Options ===\n")
            
            if 'multicastTxOptions' not in self.main_config:
                self.main_config['multicastTxOptions'] = {}
            
            opts = self.main_config['multicastTxOptions']
            
            # Show current values
            print("Current configuration:")
            print(f"  priority: {opts.get('priority', 'N/A')}")
            print(f"  ttl: {opts.get('ttl', 'N/A')}")
            print()
            
            if not self._get_yes_no_with_abort("Do you want to edit these settings?", False):
                self.safe_input("Press Enter to continue...")
                return
            
            # Don't clear screen - keep showing current values
            print("\n--- Editing Multicast TX Options ---\n")
            
            priority = self._get_int_with_abort("Priority", opts.get('priority', 4))
            opts['priority'] = priority
            
            ttl = self._get_int_with_abort("TTL", opts.get('ttl', 10))
            opts['ttl'] = ttl
            
            if self.save_main_config():
                input("\nPress Enter to continue...")
        except AbortOperationException:
            return
    
    def edit_tx_options(self):
        """Edit TX options."""
        try:
            self.clear_screen()
            print("\n=== TX Options ===\n")
            
            if 'txOptions' not in self.main_config:
                self.main_config['txOptions'] = {}
            
            opts = self.main_config['txOptions']
            
            # Show current values
            print("Current configuration:")
            print(f"  priority: {opts.get('priority', 'N/A')}")
            print(f"  ttl: {opts.get('ttl', 'N/A')}")
            print()
            
            if not self._get_yes_no_with_abort("Do you want to edit these settings?", False):
                self.safe_input("Press Enter to continue...")
                return
            
            # Don't clear screen - keep showing current values
            print("\n--- Editing TX Options ---\n")
            
            priority = self._get_int_with_abort("Priority", opts.get('priority', 4))
            opts['priority'] = priority
            
            ttl = self._get_int_with_abort("TTL", opts.get('ttl', 10))
            opts['ttl'] = ttl
            
            if self.save_main_config():
                input("\nPress Enter to continue...")
        except AbortOperationException:
            return
    
    def edit_general_settings(self):
        """Edit general EBS settings."""
        try:
            self.clear_screen()
            print("\n=== General Settings ===\n")
            
            # Show current values
            print("Current configuration:")
            print(f"  id: {self.main_config.get('id', 'N/A')}")
            print(f"  mode: {self.main_config.get('mode', 'N/A')}")
            print(f"  bridgingConfigurationFileCheckSecs: {self.main_config.get('bridgingConfigurationFileCheckSecs', 'N/A')}")
            print(f"  bridgingConfigurationFileName: {self.main_config.get('bridgingConfigurationFileName', 'N/A')}")
            print(f"  certStoreFileName: {self.main_config.get('certStoreFileName', 'N/A')}")
            print()
            
            if not self._get_yes_no_with_abort("Do you want to edit these settings?", False):
                self.safe_input("Press Enter to continue...")
                return
            
            # Don't clear screen - keep showing current values
            print("\n--- Editing General Settings ---\n")
            
            ebs_id = self._get_input_with_abort("EBS ID", self.main_config.get('id', 'EBS'))
            self.main_config['id'] = ebs_id
            
            mode = self._get_int_with_abort("Mode", self.main_config.get('mode', 2))
            self.main_config['mode'] = mode
            
            check_secs = self._get_int_with_abort("Bridging config file check interval (seconds)", 
                                     self.main_config.get('bridgingConfigurationFileCheckSecs', 5))
            self.main_config['bridgingConfigurationFileCheckSecs'] = check_secs
            
            config_file = self._get_input_with_abort("Bridging configuration file name", 
                                        self.main_config.get('bridgingConfigurationFileName', 'bridges.json'))
            self.main_config['bridgingConfigurationFileName'] = config_file
            
            cert_file = self._get_input_with_abort("Certificate store file name", 
                                      self.main_config.get('certStoreFileName', 'streamlc_client.certstore'))
            self.main_config['certStoreFileName'] = cert_file
            
            if self.save_main_config():
                input("\nPress Enter to continue...")
        except AbortOperationException:
            return
    
    def edit_groups(self):
        """Interactive editor for groups."""
        while True:
            self.clear_screen()
            print("\n=== Groups ===\n")
            
            # Always show the list of groups first
            self.display_groups_list()
            
            print("1. Add Group")
            print("2. Edit Group")
            print("3. Remove Group")
            print("4. List Groups")
            print("b. Back (Esc)")
            print("q. Quit")
            
            choice = self.menu_input("\nSelect option: ").strip().lower()
            
            if choice == '1':
                self.add_group_interactive()
            elif choice == '2':
                self.edit_group_interactive()
            elif choice == '3':
                self.remove_group_interactive()
            elif choice == '4':
                self.show_groups()
            elif choice == 'b':
                break
            elif choice == 'q':
                if self.handle_quit():
                    break
            else:
                print("Invalid option. Please try again.")
                self.safe_input("Press Enter to continue...")
    
    def add_group_interactive(self):
        """Walk user through adding a new group."""
        try:
            self.clear_screen()
            print("\n=== Add New Group ===\n")
            
            if 'groups' not in self.bridges_config:
                self.bridges_config['groups'] = []
            
            group = {}
            
            # Basic info
            group_id = self._get_input_with_abort("Group ID")
            if not group_id:
                print("Group ID is required!")
                self.safe_input("Press Enter to continue...")
                return
            
            # Check if group already exists
            for existing in self.bridges_config['groups']:
                if existing.get('id') == group_id:
                    if not self._get_yes_no_with_abort(f"Group '{group_id}' already exists. Replace it?", False):
                        self.safe_input("Press Enter to continue...")
                        return
                    self.bridges_config['groups'].remove(existing)
                    break
            
            group['id'] = group_id
            group['name'] = self._get_input_with_abort("Group name", group_id)
            group['type'] = self._get_group_type_with_abort(1)
            
            # Network configuration
            print("\n--- Network Configuration ---")
            use_multicast = self._get_yes_no_with_abort("Use multicast?", True)
            
            rallypoints = []
            if use_multicast:
                # Get default interface name from existing multicast groups if available
                default_interface = self.get_existing_interface_name()
                interface_name = self._get_interface_name_with_abort(default_interface)
                group['interfaceName'] = interface_name
                
                # RX multicast/unicast address and port - validate combination is unique
                while True:
                    rx_addr = self._get_ipv4_address_with_abort("RX multicast/unicast address", "239.0.0.190")
                    rx_port = self._get_int_with_abort("RX port", 1234)
                    
                    if self.multicast_address_port_exists(rx_addr, rx_port):
                        print(f"Error: Multicast/unicast address+port combination '{rx_addr}:{rx_port}' is already used by another group.")
                        continue
                    break
                
                group['rx'] = {'address': rx_addr, 'port': rx_port}
                
                # TX multicast/unicast address and port - validate combination is unique
                while True:
                    tx_addr = self._get_ipv4_address_with_abort("TX multicast/unicast address", rx_addr)
                    tx_port = self._get_int_with_abort("TX port", rx_port)
                    
                    if self.multicast_address_port_exists(tx_addr, tx_port):
                        print(f"Error: Multicast/unicast address+port combination '{tx_addr}:{tx_port}' is already used by another group.")
                        continue
                    break
                
                group['tx'] = {'address': tx_addr, 'port': tx_port}
                
                ttl = self._get_int_with_abort("TX TTL", 64)
                group['txOptions'] = {'ttl': ttl}
            else:
                # Rallypoint configuration
                print("\n--- Rallypoint Configuration ---")
                # Get default rallypoint address from existing groups if available
                default_rp_addr = self.get_existing_rallypoint_address()
                while True:
                    rp_addr = self._get_host_address_with_abort("Rallypoint address (hostname or IPv4)", default_rp_addr, allow_empty=True)
                    if not rp_addr:
                        break
                    rp_port = self._get_int_with_abort("Rallypoint port", 7443)
                    rallypoints.append({
                        'host': {
                            'address': rp_addr,
                            'port': rp_port
                        }
                    })
                    if not self._get_yes_no_with_abort("Add another rallypoint?", False):
                        break
                if rallypoints:
                    group['rallypoints'] = rallypoints
            
            # Audio configuration (only for Audio groups - type 1)
            if group['type'] == 1:
                print("\n--- Audio Configuration ---")
                group['txAudio'] = {}
                
                # Only ask about radio system interface for multicast groups (trunk side)
                # Rallypoint groups (enterprise side) don't need radio system transformations
                radio_type = 'engage'  # Default
                encoder_id = None
                
                if use_multicast:
                    # Multicast/trunk side - ask about radio system interfacing
                    if self._get_yes_no_with_abort("Interfacing with a radio system?", False):
                        print("\nRadio system types:")
                        print("  1. TSM (Tactical Secure Messaging)")
                        print("  2. MPU (Multi-Purpose Unit)")
                        print("  3. Silvus")
                        print("  4. Manual (custom payload types)")
                        print("  5. None (standard Engage)")
                        
                        while True:
                            try:
                                choice_input = self.safe_input("Select radio system type: ", detect_esc=True).strip()
                                if choice_input == 'ESC':
                                    print()  # Move to next line before showing abort confirmation
                                    self._confirm_abort()
                                    continue
                                choice = int(choice_input)
                                if choice == 1:
                                    radio_type = 'tsm'
                                    break
                                elif choice == 2:
                                    radio_type = 'mpu'
                                    break
                                elif choice == 3:
                                    radio_type = 'silvus'
                                    break
                                elif choice == 4:
                                    radio_type = 'manual'
                                    break
                                elif choice == 5:
                                    radio_type = 'engage'
                                    break
                                else:
                                    print("Error: Please enter a number between 1 and 5.")
                            except ValueError:
                                print("Error: Please enter a valid number.")
                            except AbortOperationException:
                                raise
                    
                    # Get encoder based on radio type compatibility for multicast groups
                    if radio_type == 'manual':
                        encoder_id = self._get_audio_encoder_with_abort(25)  # Any encoder for manual
                    else:
                        encoder_id = self._get_audio_encoder_for_radio_type_with_abort(radio_type, 25)
                        if encoder_id is None:
                            print("Error: No compatible encoders available for selected radio type.")
                            self.safe_input("Press Enter to continue...")
                            return
                else:
                    # Rallypoint/enterprise side - show all codecs (no radio system filtering)
                    encoder_id = self._get_audio_encoder_with_abort(25)
                
                group['txAudio']['encoder'] = encoder_id
                group['txAudio']['fdx'] = self._get_yes_no_with_abort("Full duplex?", False)
                # Trunk-side (multicast) groups default to 20ms, enterprise-side (rallypoint) groups default to 60ms
                framing_default = 20 if use_multicast else 60
                group['txAudio']['framingMs'] = self._get_int_with_abort("Framing (ms)", framing_default)
                group['txAudio']['maxTxSecs'] = self._get_int_with_abort("Max transmit seconds", 60)
                group['txAudio']['noHdrExt'] = self._get_yes_no_with_abort("No header extension?", False)
                
                # Only apply payload transformations for multicast groups with radio systems
                if use_multicast:
                    if radio_type == 'manual':
                        self.configure_manual_payload_transformations(group)
                    else:
                        # Call configure_payload_transformations for all radio types (including 'engage' for cleanup)
                        if not self.configure_payload_transformations(group, encoder_id, radio_type):
                            if radio_type != 'engage':
                                print(f"Warning: Could not configure {radio_type.upper()} transformations for encoder {encoder_id}")
            
            # Encryption
            crypto = self._get_input_with_abort("Crypto password (leave empty for none)", "")
            if crypto:
                group['cryptoPassword'] = crypto
            
            # Bridge target output detail - set mode based on group type
            group['bridgeTargetOutputDetail'] = {}
            if group['type'] == 1:  # Audio
                group['bridgeTargetOutputDetail']['mode'] = 2  # bomMixedStream
            elif group['type'] == 3:  # Raw
                group['bridgeTargetOutputDetail']['mode'] = 0  # bomRaw
            else:
                # Default to bomNone for other types
                group['bridgeTargetOutputDetail']['mode'] = 3  # bomNone
            
            # Add the group
            self.bridges_config['groups'].append(group)
            
            if self.save_bridges_config():
                print(f"\n{self.success('✓')} Group '{self.highlight(group_id)}' added successfully!")
                self.safe_input("Press Enter to continue...")
        except AbortOperationException:
            return
    
    def remove_group_interactive(self):
        """Remove a group interactively."""
        self.clear_screen()
        print("\n=== Remove Group ===\n")
        
        if 'groups' not in self.bridges_config or not self.bridges_config['groups']:
            print("No groups configured.")
            self.safe_input("Press Enter to continue...")
            return
        
        print("Available groups:")
        groups = self.bridges_config['groups']
        self.display_groups_table(groups, show_numbers=True)
        
        choice = self.menu_input("Enter group number to remove (or 'c' to cancel, Esc to go back): ").strip()
        
        if choice.lower() == 'c' or choice == 'b':
            return
        
        try:
            idx = int(choice) - 1
            if 0 <= idx < len(self.bridges_config['groups']):
                group = self.bridges_config['groups'][idx]
                group_id = group.get('id', 'Unknown')
                
                # Check if group is referenced in any bridges
                referencing_bridges = []
                if 'bridges' in self.bridges_config:
                    for bridge in self.bridges_config['bridges']:
                        if group_id in bridge.get('groups', []):
                            referencing_bridges.append(bridge)
                
                if referencing_bridges:
                    print(f"\n{self.warning('⚠ Warning:')} Group '{self.highlight(group_id)}' is referenced in {len(referencing_bridges)} bridge(s):")
                    for bridge in referencing_bridges:
                        print(f"   - Bridge '{bridge.get('id', 'N/A')}'")
                    
                    print("\nOptions:")
                    print("1. Remove group from all bridges and delete the group")
                    print("2. Cancel deletion")
                    
                    option = input("\nSelect option (1 or 2): ").strip()
                    
                    if option == '1':
                        # Remove group from all referencing bridges
                        for bridge in referencing_bridges:
                            if group_id in bridge.get('groups', []):
                                bridge['groups'].remove(group_id)
                                print(f"   Removed from bridge '{bridge.get('id', 'N/A')}'")
                        
                        # Now delete the group
                        if self.get_yes_no(f"\nRemove group '{group_id}'?", False):
                            self.bridges_config['groups'].pop(idx)
                            if self.save_bridges_config():
                                print(f"\n✓ Group '{group_id}' removed successfully!")
                                if referencing_bridges:
                                    print(f"✓ Group removed from {len(referencing_bridges)} bridge(s)")
                        else:
                            print("Cancelled.")
                    else:
                        print("Deletion cancelled.")
                else:
                    # No bridges reference this group, proceed with normal deletion
                    if self.get_yes_no(f"Are you sure you want to remove group '{group_id}'?", False):
                        self.bridges_config['groups'].pop(idx)
                        if self.save_bridges_config():
                            print(f"\n✓ Group '{group_id}' removed successfully!")
                    else:
                        print("Cancelled.")
            else:
                print("Invalid group number.")
        except ValueError:
            print("Invalid input.")
        
        input("Press Enter to continue...")
    
    def edit_group_interactive(self):
        """Edit an existing group."""
        try:
            self.clear_screen()
            print("\n=== Edit Group ===\n")
            
            if 'groups' not in self.bridges_config or not self.bridges_config['groups']:
                print("No groups configured.")
                self.safe_input("Press Enter to continue...")
                return
            
            print("Available groups:")
            groups = self.bridges_config['groups']
            self.display_groups_table(groups, show_numbers=True)
            
            choice = self.menu_input("Enter group number to edit (or 'c' to cancel, Esc to go back): ").strip()
            
            if choice.lower() == 'c' or choice == 'b':
                return
            
            try:
                idx = int(choice) - 1
                if not (0 <= idx < len(self.bridges_config['groups'])):
                    print("Invalid group number.")
                    self.safe_input("Press Enter to continue...")
                    return
                
                # Use the extracted editing method
                self._edit_group_at_index(idx)
            except AbortOperationException:
                raise
            except ValueError:
                print("Invalid input.")
                self.safe_input("Press Enter to continue...")
        except AbortOperationException:
            return
    
    def edit_bridges(self):
        """Interactive editor for bridges."""
        while True:
            self.clear_screen()
            print("\n=== Bridges ===\n")
            
            # Always show the list of bridges first
            self.display_bridges_list()
            
            print("1. Add Bridge")
            print("2. Edit Bridge")
            print("3. Remove Bridge")
            print("b. Back (Esc)")
            print("q. Quit")
            
            choice = self.menu_input("\nSelect option: ").strip().lower()
            
            if choice == '1':
                self.add_bridge_interactive()
            elif choice == '2':
                self.edit_bridge_interactive()
            elif choice == '3':
                self.remove_bridge_interactive()
            elif choice == 'b':
                break
            elif choice == 'q':
                if self.handle_quit():
                    break
            else:
                print("Invalid option. Please try again.")
                self.safe_input("Press Enter to continue...")
    
    def edit_bridges_submenu(self):
        """Edit Bridges & Groups sub-menu - unified interface with numbered entries."""
        while True:
            self.clear_screen()
            # Display permission warnings first so they're always visible
            self.display_permission_warnings()
            print(f"\n{self.header('=== Edit Bridges & Groups ===')} \n")
            
            # Display bridges and their groups in table format with numbering
            print(f"{self.header('Bridges:')}")
            next_number = self.display_bridges_table(show_numbers=True, start_number=1)
            
            print()
            
            # Display groups using the detailed table with continued numbering
            print(f"{self.header('Groups:')}")
            self.display_groups_list(start_number=next_number, show_numbers=True)
            
            print()
            print(f"{self.menu_option('1.')} {self.colored('Edit', Colors.GREEN)} (enter number)")
            print(f"{self.menu_option('2.')} {self.colored('Add', Colors.BLUE)} (Bridge or Group)")
            print(f"{self.menu_option('3.')} {self.colored('Remove', Colors.RED)} (enter number)")
            print(f"{self.menu_option('4.')} {self.colored('Show JSON', Colors.BRIGHT_CYAN)}")
            print(f"{self.menu_option('b.')} Back (Esc)")
            print(f"{self.menu_option('q.')} Quit")
            
            choice = self.menu_input("\nSelect option: ").strip().lower()
            
            if choice == '1':
                self.handle_edit_by_number()
            elif choice == '2':
                self.handle_add_item()
            elif choice == '3':
                self.handle_remove_by_number()
            elif choice == '4':
                self.show_bridges_json()
            elif choice == 'b':
                break
            elif choice == 'q':
                if self.handle_quit():
                    break
            else:
                print("Invalid option. Please try again.")
                self.safe_input("Press Enter to continue...")
    
    def handle_edit_by_number(self):
        """Handle editing an item by its display number."""
        try:
            # Get valid number range
            mapping = self.build_item_mapping()
            if not mapping:
                print("No items available to edit.")
                self.safe_input("Press Enter to continue...")
                return
            
            min_num = min(mapping.keys())
            max_num = max(mapping.keys())
            
            try:
                number_str = self._get_input_with_abort(f"Enter number to edit ({min_num}-{max_num}): ")
                if not number_str.strip():
                    return  # User cancelled
                    
                number = int(number_str)
                
                if number < min_num or number > max_num:
                    print(f"Error: Number must be between {min_num} and {max_num}")
                    self.safe_input("Press Enter to continue...")
                    return
                
                item = self.get_item_by_number(number)
                
                if not item:
                    print(f"Error: No item found with number {number}")
                    self.safe_input("Press Enter to continue...")
                    return
                
                item_type, item_id, item_object = item
                
                if item_type == 'bridge':
                    # Find bridge index and edit it
                    bridges = self.bridges_config.get('bridges', [])
                    for i, bridge in enumerate(bridges):
                        if bridge.get('id') == item_id:
                            self.edit_bridge_by_index(i)
                            break
                elif item_type == 'group':
                    # Find group index and edit it
                    groups = self.bridges_config.get('groups', [])
                    for i, group in enumerate(groups):
                        if group.get('id') == item_id:
                            self.edit_group_by_index(i)
                            break
                            
            except ValueError:
                print("Error: Please enter a valid number.")
                self.safe_input("Press Enter to continue...")
            except AbortOperationException:
                raise
        except AbortOperationException:
            return
        except Exception as e:
            print(f"Error editing item: {e}")
            self.safe_input("Press Enter to continue...")
    
    def handle_add_item(self):
        """Handle adding a new bridge or group."""
        print("\nWhat would you like to add?")
        print("1. Bridge")
        print("2. Group")
        print("b. Back")
        
        choice = self.menu_input("\nSelect option: ").strip().lower()
        
        if choice == '1':
            self.add_bridge_interactive()
        elif choice == '2':
            self.add_group_interactive()
        elif choice == 'b':
            return
        else:
            print("Invalid option. Please try again.")
            self.safe_input("Press Enter to continue...")
    
    def handle_remove_by_number(self):
        """Handle removing an item by its display number."""
        try:
            # Get valid number range
            mapping = self.build_item_mapping()
            if not mapping:
                print("No items available to remove.")
                self.safe_input("Press Enter to continue...")
                return
            
            min_num = min(mapping.keys())
            max_num = max(mapping.keys())
            
            try:
                number_str = self._get_input_with_abort(f"Enter number to remove ({min_num}-{max_num}): ")
                if not number_str.strip():
                    return  # User cancelled
                    
                number = int(number_str)
                
                if number < min_num or number > max_num:
                    print(f"Error: Number must be between {min_num} and {max_num}")
                    self.safe_input("Press Enter to continue...")
                    return
                
                item = self.get_item_by_number(number)
                
                if not item:
                    print(f"Error: No item found with number {number}")
                    self.safe_input("Press Enter to continue...")
                    return
                
                item_type, item_id, item_object = item
                
                # Confirm removal
                if item_type == 'bridge':
                    if not self._get_yes_no_with_abort(f"Are you sure you want to remove bridge '{item_id}'?", False):
                        return
                    self.remove_bridge_by_id(item_id)
                elif item_type == 'group':
                    # Check if group is referenced in any bridges BEFORE asking for confirmation
                    referencing_bridges = []
                    if 'bridges' in self.bridges_config:
                        for bridge in self.bridges_config['bridges']:
                            if item_id in bridge.get('groups', []):
                                referencing_bridges.append(bridge)
                    
                    # Show warning if group is in use
                    if referencing_bridges:
                        print(f"\n{self.warning('⚠ Warning:')} Group '{self.highlight(item_id)}' is currently used in {len(referencing_bridges)} bridge(s):")
                        for bridge in referencing_bridges:
                            print(f"   - Bridge '{bridge.get('id', 'N/A')}'")
                        print()
                        confirmation_msg = f"Remove group '{item_id}' from all bridges and delete the group?"
                    else:
                        confirmation_msg = f"Are you sure you want to remove group '{item_id}'?"
                    
                    if not self._get_yes_no_with_abort(confirmation_msg, False):
                        return
                    # Pass skip_warning=True since we already showed the warning above
                    self.remove_group_by_id(item_id, skip_warning=True)
                            
            except ValueError:
                print("Error: Please enter a valid number.")
                self.safe_input("Press Enter to continue...")
            except AbortOperationException:
                raise
        except AbortOperationException:
            return
        except Exception as e:
            print(f"Error removing item: {e}")
            self.safe_input("Press Enter to continue...")
    
    def edit_bridge_by_index(self, idx):
        """Edit a bridge by its index - wrapper for existing method."""
        self._edit_bridge_by_index(idx)
    
    def edit_group_by_index(self, idx):
        """Edit a group by its index - wrapper for existing method."""
        if not (0 <= idx < len(self.bridges_config.get('groups', []))):
            print("Invalid group index.")
            self.safe_input("Press Enter to continue...")
            return
        
        # Call the existing edit group interactive method but with pre-selected index
        self._edit_group_by_index(idx)
    
    def _edit_group_at_index(self, idx):
        """Edit a group at the given index - core editing logic."""
        try:
            group = self.bridges_config['groups'][idx].copy()  # Work with a copy
            original_id = group.get('id')
            
            self.clear_screen()
            print(f"\n=== Editing Group: {original_id} ===\n")
            
            # Basic info
            group_id = self._get_input_with_abort("Group ID", group.get('id', ''))
            if not group_id:
                print("Group ID is required!")
                self.safe_input("Press Enter to continue...")
                return
            
            # If ID changed, check for conflicts
            if group_id != original_id:
                for existing in self.bridges_config['groups']:
                    if existing.get('id') == group_id and existing != self.bridges_config['groups'][idx]:
                        print(f"Error: Group ID '{group_id}' already exists!")
                        self.safe_input("Press Enter to continue...")
                        return
            
            group['id'] = group_id
            group['name'] = self._get_input_with_abort("Group name", group.get('name', group_id))
            group['type'] = self._get_group_type_with_abort(group.get('type', 1))
            
            # Network configuration
            print("\n--- Network Configuration ---")
            has_multicast = 'rx' in group and 'tx' in group
            has_rallypoints = 'rallypoints' in group
            
            if has_multicast:
                current_setting = True
            elif has_rallypoints:
                current_setting = False
            else:
                current_setting = True
            
            use_multicast = self._get_yes_no_with_abort("Use multicast?", current_setting)
            
            # Remove old network config
            if 'rx' in group:
                del group['rx']
            if 'tx' in group:
                del group['tx']
            if 'interfaceName' in group:
                del group['interfaceName']
            if 'rallypoints' in group:
                del group['rallypoints']
            if 'enableMulticastFailover' in group:
                del group['enableMulticastFailover']
            if 'txOptions' in group:
                del group['txOptions']
            
            rallypoints = []
            if use_multicast:
                old_interface_name = self.bridges_config['groups'][idx].get('interfaceName', '')
                interface_name = self._get_interface_name_with_abort(old_interface_name)
                group['interfaceName'] = interface_name
                
                old_rx_addr = self.bridges_config['groups'][idx].get('rx', {}).get('address', '239.0.0.190')
                old_rx_port = self.bridges_config['groups'][idx].get('rx', {}).get('port', 1234)
                old_tx_addr = self.bridges_config['groups'][idx].get('tx', {}).get('address', old_rx_addr)
                old_tx_port = self.bridges_config['groups'][idx].get('tx', {}).get('port', old_rx_port)
                old_ttl = self.bridges_config['groups'][idx].get('txOptions', {}).get('ttl', 64)
                
                # RX multicast/unicast address and port - validate combination is unique
                while True:
                    rx_addr = self._get_ipv4_address_with_abort("RX multicast/unicast address", old_rx_addr)
                    rx_port = self._get_int_with_abort("RX port", old_rx_port)
                    
                    if self.multicast_address_port_exists(rx_addr, rx_port, exclude_group=self.bridges_config['groups'][idx]):
                        print(f"Error: Multicast/unicast address+port combination '{rx_addr}:{rx_port}' is already used by another group.")
                        continue
                    break
                
                group['rx'] = {'address': rx_addr, 'port': rx_port}
                
                # TX multicast/unicast address and port - validate combination is unique
                while True:
                    tx_addr = self._get_ipv4_address_with_abort("TX multicast/unicast address", old_tx_addr)
                    tx_port = self._get_int_with_abort("TX port", old_tx_port)
                    
                    if self.multicast_address_port_exists(tx_addr, tx_port, exclude_group=self.bridges_config['groups'][idx]):
                        print(f"Error: Multicast/unicast address+port combination '{tx_addr}:{tx_port}' is already used by another group.")
                        continue
                    break
                
                group['tx'] = {'address': tx_addr, 'port': tx_port}
                
                ttl = self._get_int_with_abort("TX TTL", old_ttl)
                group['txOptions'] = {'ttl': ttl}
            else:
                # Rallypoint configuration
                print("\n--- Rallypoint Configuration ---")
                old_rallypoints = self.bridges_config['groups'][idx].get('rallypoints', [])
                rallypoints = []
                
                if old_rallypoints:
                    print("Current rallypoints:")
                    for i, rp in enumerate(old_rallypoints, 1):
                        addr = rp.get('host', {}).get('address', 'N/A')
                        port = rp.get('host', {}).get('port', 'N/A')
                        print(f"  {i}. {addr}:{port}")
                    if not self._get_yes_no_with_abort("Keep existing rallypoints?", True):
                        old_rallypoints = []
                
                # Add new rallypoints
                # Get default rallypoint address from existing groups if available
                default_rp_addr = self.get_existing_rallypoint_address()
                while True:
                    if old_rallypoints:
                        rp = old_rallypoints.pop(0)
                        rallypoints.append(rp)
                        if not old_rallypoints:
                            break
                    
                    rp_addr = self._get_host_address_with_abort("Rallypoint address (hostname or IPv4, leave empty to finish)", default_rp_addr, allow_empty=True)
                    if not rp_addr:
                        break
                    rp_port = self._get_int_with_abort("Rallypoint port", 7443)
                    rallypoints.append({
                        'host': {
                            'address': rp_addr,
                            'port': rp_port
                        }
                    })
                    if not self._get_yes_no_with_abort("Add another rallypoint?", False):
                        break
                
                if rallypoints:
                    group['rallypoints'] = rallypoints
            
            # Audio configuration (only for Audio groups - type 1)
            if group['type'] == 1:
                print("\n--- Audio Configuration ---")
                old_tx_audio = self.bridges_config['groups'][idx].get('txAudio', {})
                group['txAudio'] = {}
                
                # Check for existing transformations to determine default radio type
                has_existing_transformations = 'customRtpPayloadType' in old_tx_audio or 'inboundRtpPayloadTypeTranslations' in self.bridges_config['groups'][idx]
                
                # Check if group was created by a wizard and get the wizard type
                wizard_type = self.get_wizard_type(self.bridges_config['groups'][idx])
                default_radio_type = None
                if wizard_type in ['tsm', 'mpu', 'silvus']:
                    default_radio_type = wizard_type
                    has_existing_transformations = True  # Wizard-created groups with radio types should default to Yes
                
                # First ask about radio system interface
                radio_type = 'engage'  # Default
                if self._get_yes_no_with_abort("Interfacing with a radio system?", has_existing_transformations):
                    print("\nRadio system types:")
                    print("  1. TSM (Tactical Secure Messaging)")
                    print("  2. MPU (Multi-Purpose Unit)")
                    print("  3. Silvus")
                    print("  4. Manual (custom payload types)")
                    print("  5. None (standard Engage)")
                    
                    # Determine default choice based on wizard type
                    default_choice = None
                    if default_radio_type == 'tsm':
                        default_choice = 1
                    elif default_radio_type == 'mpu':
                        default_choice = 2
                    elif default_radio_type == 'silvus':
                        default_choice = 3
                    
                    while True:
                        try:
                            prompt = "Select radio system type"
                            if default_choice:
                                prompt += f" [{default_choice}]"
                            prompt += ": "
                            choice_input = self.safe_input(prompt, detect_esc=True).strip()
                            if choice_input == 'ESC':
                                print()  # Move to next line before showing abort confirmation
                                self._confirm_abort()
                                continue
                            if not choice_input and default_choice:
                                choice = default_choice
                            else:
                                choice = int(choice_input)
                            if choice == 1:
                                radio_type = 'tsm'
                                break
                            elif choice == 2:
                                radio_type = 'mpu'
                                break
                            elif choice == 3:
                                radio_type = 'silvus'
                                break
                            elif choice == 4:
                                radio_type = 'manual'
                                break
                            elif choice == 5:
                                radio_type = 'engage'
                                break
                            else:
                                print("Error: Please enter a number between 1 and 5.")
                        except ValueError:
                            print("Error: Please enter a valid number.")
                        except AbortOperationException:
                            raise
                
                # Get encoder based on radio type compatibility
                old_encoder = old_tx_audio.get('encoder', 25)
                if radio_type == 'manual':
                    encoder_id = self._get_audio_encoder_with_abort(old_encoder)  # Any encoder for manual
                else:
                    encoder_id = self._get_audio_encoder_for_radio_type_with_abort(radio_type, old_encoder)
                    if encoder_id is None:
                        print("Error: No compatible encoders available for selected radio type.")
                        self.safe_input("Press Enter to continue...")
                        return
                
                group['txAudio']['encoder'] = encoder_id
                group['txAudio']['fdx'] = self._get_yes_no_with_abort("Full duplex?", old_tx_audio.get('fdx', False))
                # Trunk-side (multicast) groups default to 20ms, enterprise-side (rallypoint) groups default to 60ms
                framing_default = old_tx_audio.get('framingMs', 20 if use_multicast else 60)
                group['txAudio']['framingMs'] = self._get_int_with_abort("Framing (ms)", framing_default)
                group['txAudio']['maxTxSecs'] = self._get_int_with_abort("Max transmit seconds", old_tx_audio.get('maxTxSecs', 60))
                group['txAudio']['noHdrExt'] = self._get_yes_no_with_abort("No header extension?", old_tx_audio.get('noHdrExt', False))
                
                # Apply payload transformations based on radio type
                if radio_type == 'manual':
                    self.configure_manual_payload_transformations(group)
                else:
                    # Call configure_payload_transformations for all radio types (including 'engage' for cleanup)
                    if not self.configure_payload_transformations(group, encoder_id, radio_type):
                        if radio_type != 'engage':
                            print(f"Warning: Could not configure {radio_type.upper()} transformations for encoder {encoder_id}")
            
            # Encryption
            old_crypto = self.bridges_config['groups'][idx].get('cryptoPassword', '')
            crypto = self._get_input_with_abort("Crypto password (leave empty for none)", old_crypto)
            if crypto:
                group['cryptoPassword'] = crypto
            elif 'cryptoPassword' in group:
                del group['cryptoPassword']
            
            # Bridge target output detail - set mode based on group type
            group['bridgeTargetOutputDetail'] = {}
            if group['type'] == 1:  # Audio
                group['bridgeTargetOutputDetail']['mode'] = 2  # bomMixedStream
            elif group['type'] == 3:  # Raw
                group['bridgeTargetOutputDetail']['mode'] = 0  # bomRaw
            else:
                # Default to bomNone for other types
                group['bridgeTargetOutputDetail']['mode'] = 3  # bomNone
            
            # Update the group
            self.bridges_config['groups'][idx] = group
            
            if self.save_bridges_config():
                print(f"\n{self.success('✓')} Group '{self.highlight(group_id)}' updated successfully!")
                self.safe_input("Press Enter to continue...")
        except AbortOperationException:
            return
    
    def _edit_group_by_index(self, idx):
        """Edit a group by its index."""
        if not (0 <= idx < len(self.bridges_config.get('groups', []))):
            print("Invalid group index.")
            self.safe_input("Press Enter to continue...")
            return
        
        # Directly edit the group at the given index
        self._edit_group_at_index(idx)
    
    def remove_bridge_by_id(self, bridge_id):
        """Remove a bridge by its ID with proper group cleanup."""
        bridges = self.bridges_config.get('bridges', [])
        for i, bridge in enumerate(bridges):
            if bridge.get('id') == bridge_id:
                bridge_groups = bridge.get('groups', [])
                is_wizard_created = self.get_wizard_type(bridge) is not None
                
                # Check which groups are only used by this bridge
                groups_to_remove = []
                if bridge_groups:
                    for group_id in bridge_groups:
                        # Check if this group is used by any other bridge
                        used_in_other_bridges = False
                        for other_bridge in self.bridges_config['bridges']:
                            if other_bridge != bridge:  # Don't check the bridge we're removing
                                if group_id in other_bridge.get('groups', []):
                                    used_in_other_bridges = True
                                    break
                        
                        if not used_in_other_bridges:
                            groups_to_remove.append(group_id)
                
                # Handle group removal based on bridge type
                remove_groups = False
                if groups_to_remove:
                    if is_wizard_created:
                        # Wizard bridge: automatically remove unused groups
                        remove_groups = True
                        print(f"\nWizard-created bridge '{bridge_id}' uses groups: {', '.join(bridge_groups)}")
                        print(f"Automatically removing {len(groups_to_remove)} unused group(s):")
                        for group_id in groups_to_remove:
                            print(f"  - {group_id}")
                    else:
                        # Manual bridge: prompt user for group removal
                        print(f"\nManually-created bridge '{bridge_id}' uses groups: {', '.join(bridge_groups)}")
                        print(f"\nThe following groups are not used by any other bridge:")
                        for group_id in groups_to_remove:
                            print(f"  - {group_id}")
                        
                        remove_groups = self.get_yes_no(f"\nDo you want to also remove these {len(groups_to_remove)} group(s)?", True)
                
                # Remove the groups if requested/required
                if remove_groups and groups_to_remove:
                    if 'groups' in self.bridges_config:
                        groups_to_delete = []
                        for group in self.bridges_config['groups']:
                            if group.get('id') in groups_to_remove:
                                groups_to_delete.append(group)
                        
                        for group in groups_to_delete:
                            self.bridges_config['groups'].remove(group)
                            print(f"  {self.success('✓')} Removed group: {self.highlight(group.get('id'))}")
                
                # Remove the bridge
                self.bridges_config['bridges'].pop(i)
                if self.save_bridges_config():
                    print(f"\n{self.success('✓')} Bridge '{self.highlight(bridge_id)}' removed successfully!")
                    if remove_groups and groups_to_remove:
                        print(f"  Also removed {len(groups_to_remove)} unused group(s).")
                else:
                    print(f"\n{self.warning('⚠ Error:')} Failed to save configuration after removing bridge '{bridge_id}'.")
                self.safe_input("Press Enter to continue...")
                return
        
        print(f"{self.error('✗ Error:')} Bridge '{bridge_id}' not found.")
        self.safe_input("Press Enter to continue...")
    
    def remove_group_by_id(self, group_id, skip_warning=False):
        """Remove a group by its ID.
        
        Args:
            group_id: ID of the group to remove
            skip_warning: If True, skip the bridge usage warning (already shown)
        """
        groups = self.bridges_config.get('groups', [])
        for i, group in enumerate(groups):
            if group.get('id') == group_id:
                # Check if group is referenced in any bridges
                referencing_bridges = []
                if 'bridges' in self.bridges_config:
                    for bridge in self.bridges_config['bridges']:
                        if group_id in bridge.get('groups', []):
                            referencing_bridges.append(bridge)
                
                if referencing_bridges and not skip_warning:
                    print(f"\n{self.warning('⚠ Warning:')} Group '{self.highlight(group_id)}' is referenced in {len(referencing_bridges)} bridge(s):")
                    for bridge in referencing_bridges:
                        print(f"   - Bridge '{bridge.get('id', 'N/A')}'")
                    
                    if self.get_yes_no("\nRemove group from all bridges and delete the group?", False):
                        # Remove group from all referencing bridges
                        for bridge in referencing_bridges:
                            if group_id in bridge.get('groups', []):
                                bridge['groups'].remove(group_id)
                        
                        # Remove the group
                        self.bridges_config['groups'].pop(i)
                        if self.save_bridges_config():
                            print(f"\n✓ Group '{group_id}' removed successfully!")
                        else:
                            print(f"\n⚠ Error: Failed to save configuration after removing group '{group_id}'.")
                    else:
                        print("Group removal cancelled.")
                        self.safe_input("Press Enter to continue...")
                        return
                elif referencing_bridges and skip_warning:
                    # Warning already shown, just proceed with removal
                    # Remove group from all referencing bridges
                    for bridge in referencing_bridges:
                        if group_id in bridge.get('groups', []):
                            bridge['groups'].remove(group_id)
                    
                    # Remove the group
                    self.bridges_config['groups'].pop(i)
                    if self.save_bridges_config():
                        print(f"\n✓ Group '{group_id}' removed successfully!")
                    else:
                        print(f"\n⚠ Error: Failed to save configuration after removing group '{group_id}'.")
                else:
                    # No references, safe to remove
                    self.bridges_config['groups'].pop(i)
                    if self.save_bridges_config():
                        print(f"\n✓ Group '{group_id}' removed successfully!")
                    else:
                        print(f"\n⚠ Error: Failed to save configuration after removing group '{group_id}'.")
                
                self.safe_input("Press Enter to continue...")
                return
        
        print(f"Error: Group '{group_id}' not found.")
        self.safe_input("Press Enter to continue...")

    def generate_bridge_id_from_name(self, name: str) -> str:
        """Generate a valid bridge ID from a bridge name."""
        # Convert to lowercase, replace spaces with hyphens, remove special chars
        id_str = name.lower()
        id_str = re.sub(r'[^a-z0-9\-_]', '', id_str)
        id_str = re.sub(r'[\s]+', '-', id_str)
        id_str = re.sub(r'-+', '-', id_str)  # Replace multiple hyphens with single
        id_str = id_str.strip('-')  # Remove leading/trailing hyphens
        return id_str if id_str else "bridge"
    
    def bridge_id_exists(self, bridge_id: str, exclude_bridge=None) -> bool:
        """Check if a bridge ID already exists."""
        if 'bridges' not in self.bridges_config:
            return False
        for bridge in self.bridges_config['bridges']:
            if bridge.get('id') == bridge_id and bridge != exclude_bridge:
                return True
        return False
    
    def group_id_exists(self, group_id: str, exclude_group=None) -> bool:
        """Check if a group ID already exists."""
        if 'groups' not in self.bridges_config:
            return False
        for group in self.bridges_config['groups']:
            if group.get('id') == group_id and group != exclude_group:
                return True
        return False
    
    def get_existing_rallypoint_address(self) -> Optional[str]:
        """Get the first existing rallypoint address from any group."""
        if 'groups' not in self.bridges_config:
            return None
        for group in self.bridges_config['groups']:
            if 'rallypoints' in group and group['rallypoints']:
                for rp in group['rallypoints']:
                    if 'host' in rp and 'address' in rp['host']:
                        return rp['host']['address']
        return None
    
    def get_existing_interface_name(self) -> Optional[str]:
        """Get the first existing interface name from any multicast group."""
        if 'groups' not in self.bridges_config:
            return None
        for group in self.bridges_config['groups']:
            if 'interfaceName' in group and ('rx' in group or 'tx' in group):
                return group['interfaceName']
        return None
    
    def multicast_address_port_exists(self, address: str, port: int, exclude_group=None) -> bool:
        """Check if a multicast address+port combination is already used in any group."""
        if 'groups' not in self.bridges_config:
            return False
        for group in self.bridges_config['groups']:
            if group == exclude_group:
                continue
            # Check RX address+port combination
            if ('rx' in group and 
                group['rx'].get('address') == address and 
                group['rx'].get('port') == port):
                return True
            # Check TX address+port combination
            if ('tx' in group and 
                group['tx'].get('address') == address and 
                group['tx'].get('port') == port):
                return True
        return False
    
    def get_wizard_type(self, item):
        """Get wizard type from _wizardCreated attribute.
        
        Args:
            item: A bridge or group dictionary
            
        Returns:
            str or None: The wizard type ('raw', 'audio', 'tsm', 'mpu', 'silvus') or None if not wizard-created
        """
        wizard_created = item.get('_wizardCreated', False)
        if isinstance(wizard_created, dict):
            return wizard_created.get('type')
        elif wizard_created is True:
            # Legacy format - return None (unknown type)
            return None
        return None
    
    def get_wizard_created_bridges(self):
        """Get list of bridges created by wizards."""
        if 'bridges' not in self.bridges_config:
            return []
        return [b for b in self.bridges_config['bridges'] if self.get_wizard_type(b) is not None or b.get('_wizardCreated', False)]
    
    def display_wizard_bridges_table(self, show_numbers=False):
        """Display wizard-created bridges in a table format with each group on a new line."""
        wizard_bridges = self.get_wizard_created_bridges()
        
        if not wizard_bridges:
            print("No wizard-created bridges found.")
            return
        
        # Calculate column widths
        id_width = max(len('Bridge ID'), max(len(bridge.get('id', 'N/A')) for bridge in wizard_bridges), 10)
        groups_width = max(len('Groups'), max(len(group_id) for bridge in wizard_bridges for group_id in bridge.get('groups', []) or ['None']), 8)
        
        # Print header
        header = f"{'#':<4} " if show_numbers else ""
        header += f"{'Bridge ID':<{id_width}} {'Groups':<{groups_width}}"
        print(header)
        print("-" * len(header))
        
        # Print rows - each group on a new line, ID only shown once
        for i, bridge in enumerate(wizard_bridges, 1):
            bridge_id = bridge.get('id', 'N/A')
            groups = bridge.get('groups', [])
            
            if not groups:
                # No groups - show ID and "None"
                row_str = f"{i:<4} " if show_numbers else ""
                row_str += f"{bridge_id:<{id_width}} None"
                print(row_str)
            else:
                # First group - show ID
                row_str = f"{i:<4} " if show_numbers else ""
                row_str += f"{bridge_id:<{id_width}} {groups[0]:<{groups_width}}"
                print(row_str)
                
                # Remaining groups - show empty ID column
                for group in groups[1:]:
                    row_str = f"{'':<4} " if show_numbers else ""
                    row_str += f"{'':<{id_width}} {group:<{groups_width}}"
                    print(row_str)
            
            # Add blank line between bridges (except for the last one)
            if i < len(wizard_bridges):
                print()
    
    def edit_wizard_bridge_interactive(self):
        """Edit a wizard-created bridge."""
        wizard_bridges = self.get_wizard_created_bridges()
        
        if not wizard_bridges:
            print("No wizard-created bridges found.")
            self.safe_input("Press Enter to continue...")
            return
        
        self.clear_screen()
        print("\n=== Edit Wizard-Created Bridge ===\n")
        print("Wizard-created bridges:")
        self.display_wizard_bridges_table(show_numbers=True)
        
        choice = self.menu_input("\nEnter bridge number to edit (or 'c' to cancel, Esc to go back): ").strip()
        
        if choice.lower() == 'c' or choice == 'b':
            return
        
        try:
            idx = int(choice) - 1
            if not (0 <= idx < len(wizard_bridges)):
                print("Invalid bridge number.")
                self.safe_input("Press Enter to continue...")
                return
            
            # Find the bridge in the full bridges list
            wizard_bridge = wizard_bridges[idx]
            bridge_id = wizard_bridge.get('id')
            
            # Find index in full bridges list
            full_idx = None
            for i, bridge in enumerate(self.bridges_config['bridges']):
                if bridge.get('id') == bridge_id:
                    full_idx = i
                    break
            
            if full_idx is None:
                print("Error: Bridge not found in configuration.")
                self.safe_input("Press Enter to continue...")
                return
            
            # Use the existing edit_bridge_interactive logic but with a specific index
            # We'll need to modify the approach - let's create a helper that edits by index
            self._edit_bridge_by_index(full_idx)
        except ValueError:
            print("Invalid input.")
            self.safe_input("Press Enter to continue...")
    
    def remove_wizard_bridge_interactive(self):
        """Remove a wizard-created bridge."""
        wizard_bridges = self.get_wizard_created_bridges()
        
        if not wizard_bridges:
            print("No wizard-created bridges found.")
            self.safe_input("Press Enter to continue...")
            return
        
        self.clear_screen()
        print("\n=== Remove Wizard-Created Bridge ===\n")
        print("Wizard-created bridges:")
        self.display_wizard_bridges_table(show_numbers=True)
        
        choice = self.menu_input("\nEnter bridge number to remove (or 'c' to cancel, Esc to go back): ").strip()
        
        if choice.lower() == 'c' or choice == 'b':
            return
        
        try:
            idx = int(choice) - 1
            if not (0 <= idx < len(wizard_bridges)):
                print("Invalid bridge number.")
                self.safe_input("Press Enter to continue...")
                return
            
            # Find the bridge in the full bridges list
            wizard_bridge = wizard_bridges[idx]
            bridge_id = wizard_bridge.get('id')
            
            # Find index in full bridges list
            full_idx = None
            for i, bridge in enumerate(self.bridges_config['bridges']):
                if bridge.get('id') == bridge_id:
                    full_idx = i
                    break
            
            if full_idx is None:
                print("Error: Bridge not found in configuration.")
                self.safe_input("Press Enter to continue...")
                return
            
            # Remove the bridge (with group cleanup like regular remove)
            bridge = self.bridges_config['bridges'][full_idx]
            bridge_groups = bridge.get('groups', [])
            
            if self.get_yes_no(f"Are you sure you want to remove bridge '{bridge_id}'?", False):
                # Check which groups are only used by this bridge
                groups_to_remove = []
                if bridge_groups:
                    print(f"\nBridge '{bridge_id}' uses groups: {', '.join(bridge_groups)}")
                    
                    for group_id in bridge_groups:
                        # Check if this group is used by any other bridge
                        used_in_other_bridges = False
                        for other_bridge in self.bridges_config['bridges']:
                            if other_bridge != bridge:  # Don't check the bridge we're removing
                                if group_id in other_bridge.get('groups', []):
                                    used_in_other_bridges = True
                                    break
                        
                        if not used_in_other_bridges:
                            groups_to_remove.append(group_id)
                    
                    if groups_to_remove:
                        print(f"\nThe following groups are not used by any other bridge:")
                        for group_id in groups_to_remove:
                            print(f"  - {group_id}")
                        
                        if self.get_yes_no(f"\nDo you want to also remove these {len(groups_to_remove)} group(s)?", True):
                            # Remove the groups
                            if 'groups' in self.bridges_config:
                                groups_to_delete = []
                                for group in self.bridges_config['groups']:
                                    if group.get('id') in groups_to_remove:
                                        groups_to_delete.append(group)
                                
                                for group in groups_to_delete:
                                    self.bridges_config['groups'].remove(group)
                                    print(f"  ✓ Removed group: {group.get('id')}")
                
                # Remove the bridge
                self.bridges_config['bridges'].pop(full_idx)
                if self.save_bridges_config():
                    print(f"\n✓ Bridge '{bridge_id}' removed successfully!")
                    if groups_to_remove:
                        print(f"  Also removed {len(groups_to_remove)} unused group(s).")
                else:
                    print("\n⚠ Error: Failed to save configuration.")
            else:
                print("Cancelled.")
        except ValueError:
            print("Invalid input.")
        
        self.safe_input("Press Enter to continue...")
    
    def _edit_bridge_by_index(self, idx):
        """Edit a bridge by its index in the bridges list."""
        try:
            if not (0 <= idx < len(self.bridges_config['bridges'])):
                print("Invalid bridge index.")
                self.safe_input("Press Enter to continue...")
                return
            
            bridge = self.bridges_config['bridges'][idx]
            original_id = bridge.get('id', '')
            original_groups = bridge.get('groups', []).copy()
            
            self.clear_screen()
            print(f"\n=== Editing Bridge: {original_id} ===\n")
            
            # Edit bridge ID (pre-filled with current ID, press Enter to keep it)
            bridge_id = self._get_input_with_abort(f"Bridge ID [{original_id}]", original_id)
            if not bridge_id:
                bridge_id = original_id  # Keep current ID if empty
            
            # If ID changed, check for conflicts
            if bridge_id != original_id:
                for existing in self.bridges_config['bridges']:
                    if existing.get('id') == bridge_id and existing != bridge:
                        print(f"Error: Bridge ID '{bridge_id}' already exists!")
                        self.safe_input("Press Enter to continue...")
                        return
            
            bridge['id'] = bridge_id
            
            # Edit groups
            print("\n--- Bridge Groups ---")
            available_groups = self.bridges_config.get('groups', [])
            if not available_groups:
                print("No groups available.")
                self.safe_input("Press Enter to continue...")
                return
            
            # Display available groups with status
            print("\nAvailable groups:")
            self.display_groups_table(available_groups, show_numbers=True, show_status=True, current_groups=original_groups)
            
            # Pre-fill with current groups
            current_group_nums = []
            for group_id in original_groups:
                for i, group in enumerate(available_groups, 1):
                    if group.get('id') == group_id:
                        current_group_nums.append(str(i))
                        break
            
            current_groups_str = ','.join(current_group_nums) if current_group_nums else ''
            print("\nSelect groups to bridge (enter numbers separated by commas, e.g., 1,2,3)")
            groups_input = self._get_input_with_abort(f"Groups [{','.join(original_groups)}]", current_groups_str)
            
            if groups_input:
                try:
                    group_numbers = [int(x.strip()) for x in groups_input.split(',')]
                    selected_groups = []
                    for num in group_numbers:
                        if 1 <= num <= len(available_groups):
                            selected_groups.append(available_groups[num - 1].get('id'))
                        else:
                            print(f"Warning: Group number {num} is invalid. Skipping.")
                    
                    if selected_groups:
                        bridge['groups'] = selected_groups
                    else:
                        print("Error: At least one group must be selected.")
                        self.safe_input("Press Enter to continue...")
                        return
                except ValueError:
                    print("Invalid input. Please enter numbers separated by commas.")
                    self.safe_input("Press Enter to continue...")
                    return
            else:
                bridge['groups'] = original_groups
            
            # Edit enabled status
            current_enabled = bridge.get('enabled', True)
            enabled_str = "Yes" if current_enabled else "No"
            enabled_input = self._get_yes_no_with_abort(f"Bridge enabled [{enabled_str}]", current_enabled)
            bridge['enabled'] = enabled_input
            
            # Save changes
            if self.save_bridges_config():
                print(f"\n✓ Bridge '{bridge_id}' updated successfully!")
            else:
                print("\n⚠ Error: Failed to save configuration.")
            
            self.safe_input("Press Enter to continue...")
        except AbortOperationException:
            return
    
    def bridge_wizard_menu(self):
        """Bridge wizard menu - shows available wizards and wizard-created bridges."""
        while True:
            self.clear_screen()
            print("\n=== Bridge Wizard ===\n")
            
            # Show wizard-created bridges
            wizard_bridges = self.get_wizard_created_bridges()
            if wizard_bridges:
                print("Wizard-created bridges:")
                self.display_wizard_bridges_table()
                print()
            else:
                print("No wizard-created bridges yet.\n")
            
            print("Create Bridge wizards:")
            print("1. Raw Bridge (Multicast to Rallypoint)")
            print()
            if wizard_bridges:
                print("Wizard-created bridge management:")
                print("2. Edit bridge")
                print("3. Remove bridge")
            print("b. Back (Esc)")
            print("q. Quit")
            
            choice = self.menu_input("\nSelect option: ").strip().lower()
            
            if choice == '1':
                self.wizard_raw_bridge_multicast_to_rallypoint()
            elif choice == '2':
                if wizard_bridges:
                    self.edit_wizard_bridge_interactive()
                else:
                    print("Invalid option. Please try again.")
                    self.safe_input("Press Enter to continue...")
            elif choice == '3' and wizard_bridges:
                self.remove_wizard_bridge_interactive()
            elif choice == 'b':
                break
            elif choice == 'q':
                if self.handle_quit():
                    break
            else:
                print("Invalid option. Please try again.")
                self.safe_input("Press Enter to continue...")
    
    def wizard_raw_bridge_multicast_to_rallypoint(self):
        """Wizard to create a raw bridge from multicast to rallypoint."""
        try:
            self.clear_screen()
            print("\n=== Raw Bridge Wizard (Multicast to Rallypoint) ===\n")
            print("This wizard will create:")
            print("  - A Raw group with rallypoint (enterprise side)")
            print("  - A Raw group with multicast (trunk side)")
            print("  - A bridge connecting both groups\n")
            
            # Ensure groups and bridges arrays exist
            if 'groups' not in self.bridges_config:
                self.bridges_config['groups'] = []
            if 'bridges' not in self.bridges_config:
                self.bridges_config['bridges'] = []
            
            # Step 1: Get bridge name and generate ID
            while True:
                bridge_name_input = self._get_input_with_abort("Enter bridge name")
                bridge_name = bridge_name_input.strip() if bridge_name_input else ""
                if not bridge_name:
                    print("Error: Bridge name is required and cannot be empty.")
                    continue
                
                bridge_id = self.generate_bridge_id_from_name(bridge_name)
                
                # Check if bridge already exists
                if self.bridge_id_exists(bridge_id):
                    print(f"Error: Bridge ID '{bridge_id}' already exists. Please choose a different name.")
                    continue
                
                break
            
            # Step 2: Get stream ID for enterprise side
            while True:
                stream_id_input = self._get_input_with_abort("Enter stream ID (for enterprise/rallypoint side)")
                stream_id = stream_id_input.strip() if stream_id_input else ""
                if not stream_id:
                    print("Error: Stream ID is required and cannot be empty.")
                    continue
                
                # Check if group ID already exists
                if self.group_id_exists(stream_id):
                    print(f"Error: Group ID '{stream_id}' already exists. Please choose a different stream ID.")
                    continue
                
                # Check if trunk group ID already exists
                trunk_group_id = f"{stream_id}-trunk"
                if self.group_id_exists(trunk_group_id):
                    print(f"Error: Group ID '{trunk_group_id}' already exists. Please choose a different stream ID.")
                    continue
                
                break
            
            enterprise_group_id = stream_id
            trunk_group_id = f"{stream_id}-trunk"
            
            # Remove any existing groups with these IDs (shouldn't happen due to validation, but just in case)
            existing_enterprise = None
            existing_trunk = None
            for group in self.bridges_config['groups']:
                if group.get('id') == enterprise_group_id:
                    existing_enterprise = group
                if group.get('id') == trunk_group_id:
                    existing_trunk = group
            
            if existing_enterprise:
                self.bridges_config['groups'].remove(existing_enterprise)
            if existing_trunk:
                self.bridges_config['groups'].remove(existing_trunk)
            
            # Step 3: Create enterprise group (rallypoint side)
            print("\n--- Enterprise Group (Rallypoint Side) ---")
            enterprise_group = {
                'id': enterprise_group_id,
                'name': enterprise_group_id,
                'type': 3,  # Raw
                '_wizardCreated': {'type': 'raw'}
            }
            
            # Rallypoint configuration
            print("\nRallypoint Configuration:")
            # Get default rallypoint address from existing groups if available
            default_rp_addr = self.get_existing_rallypoint_address()
            while True:
                rp_addr_input = self._get_host_address_with_abort("Rallypoint address (hostname or IPv4)", default_rp_addr)
                rp_addr = rp_addr_input.strip() if rp_addr_input else ""
                if not rp_addr:
                    print("Error: Rallypoint address is required and cannot be empty.")
                    continue
                break
            
            rp_port = self._get_int_with_abort("Rallypoint port", 7443)
            enterprise_group['rallypoints'] = [{
                'host': {
                    'address': rp_addr,
                    'port': rp_port
                }
            }]
            
            # Step 4: Create trunk group (multicast side)
            print("\n--- Trunk Group (Multicast Side) ---")
            trunk_group = {
                'id': trunk_group_id,
                'name': trunk_group_id,
                'type': 3,  # Raw
                '_wizardCreated': {'type': 'raw'}
            }
            
            # Multicast configuration
            print("\nMulticast Configuration:")
            # Get default interface name from existing multicast groups if available
            default_interface = self.get_existing_interface_name()
            interface_name = self._get_interface_name_with_abort(default_interface)
            trunk_group['interfaceName'] = interface_name
            
            # RX multicast/unicast address and port - validate combination is unique
            while True:
                rx_addr = self._get_ipv4_address_with_abort("RX multicast/unicast address")
                
                rx_port = self._get_int_with_abort("RX multicast port")
                if rx_port is None:
                    print("Error: RX multicast port is required and cannot be empty.")
                    continue
                
                if self.multicast_address_port_exists(rx_addr, rx_port):
                    print(f"Error: Multicast/unicast address+port combination '{rx_addr}:{rx_port}' is already used by another group.")
                    continue
                break
            
            trunk_group['rx'] = {'address': rx_addr, 'port': rx_port}
            
            # TX defaults to RX values
            tx_addr = self._get_ipv4_address_with_abort("TX multicast/unicast address", rx_addr)
            tx_port = self._get_int_with_abort("TX multicast port", rx_port)
            trunk_group['tx'] = {'address': tx_addr, 'port': tx_port}
            
            ttl = self._get_int_with_abort("TX TTL", 64)
            trunk_group['txOptions'] = {'ttl': ttl}
            
            # Step 5: Create bridge
            bridge = {
                'id': bridge_id,
                'groups': [enterprise_group_id, trunk_group_id],
                '_wizardCreated': {'type': 'raw'},
                'enabled': True
            }
            
            # Add bridge target output detail to both groups
            enterprise_group['bridgeTargetOutputDetail'] = {'mode': 0}  # bomRaw for Raw groups
            trunk_group['bridgeTargetOutputDetail'] = {'mode': 0}  # bomRaw for Raw groups
            
            # Add groups and bridge
            self.bridges_config['groups'].append(enterprise_group)
            self.bridges_config['groups'].append(trunk_group)
            self.bridges_config['bridges'].append(bridge)
            
            # Save configuration
            if self.save_bridges_config():
                print(f"\n✓ Bridge '{bridge_id}' created successfully!")
                print(f"  Enterprise group: {enterprise_group_id} (Rallypoint)")
                print(f"  Trunk group: {trunk_group_id} (Multicast)")
                print(f"  Bridge connects: {enterprise_group_id} ↔ {trunk_group_id}")
                self.safe_input("\nPress Enter to continue...")
            else:
                print("\n⚠ Error: Failed to save configuration.")
                self.safe_input("Press Enter to continue...")
        except AbortOperationException:
            return
    
    def wizard_audio_bridge_multicast_to_rallypoint(self):
        """Wizard to create an audio bridge from multicast to rallypoint."""
        try:
            self.clear_screen()
            print("\n=== Audio Bridge Wizard (Multicast to Rallypoint) ===\n")
            print("This wizard will create:")
            print("  - An Audio group with rallypoint (enterprise side)")
            print("  - An Audio group with multicast (trunk side)")
            print("  - A bridge connecting both groups\n")
            
            # Ensure groups and bridges arrays exist
            if 'groups' not in self.bridges_config:
                self.bridges_config['groups'] = []
            if 'bridges' not in self.bridges_config:
                self.bridges_config['bridges'] = []
            
            # Step 1: Get bridge name and generate ID
            while True:
                bridge_name_input = self._get_input_with_abort("Enter bridge name")
                bridge_name = bridge_name_input.strip() if bridge_name_input else ""
                if not bridge_name:
                    print("Error: Bridge name is required and cannot be empty.")
                    continue
                
                bridge_id = self.generate_bridge_id_from_name(bridge_name)
                
                # Check if bridge already exists
                if self.bridge_id_exists(bridge_id):
                    print(f"Error: Bridge ID '{bridge_id}' already exists. Please choose a different name.")
                    continue
                
                break
            
            # Step 2: Get stream ID for enterprise side
            while True:
                stream_id_input = self._get_input_with_abort("Enter stream ID (for enterprise/rallypoint side)")
                stream_id = stream_id_input.strip() if stream_id_input else ""
                if not stream_id:
                    print("Error: Stream ID is required and cannot be empty.")
                    continue
                
                # Check if group ID already exists
                if self.group_id_exists(stream_id):
                    print(f"Error: Group ID '{stream_id}' already exists. Please choose a different stream ID.")
                    continue
                
                # Check if trunk group ID already exists
                trunk_group_id = f"{stream_id}-trunk"
                if self.group_id_exists(trunk_group_id):
                    print(f"Error: Group ID '{trunk_group_id}' already exists. Please choose a different stream ID.")
                    continue
                
                break
            
            enterprise_group_id = stream_id
            trunk_group_id = f"{stream_id}-trunk"
            
            # Remove any existing groups with these IDs (shouldn't happen due to validation, but just in case)
            existing_enterprise = None
            existing_trunk = None
            for group in self.bridges_config['groups']:
                if group.get('id') == enterprise_group_id:
                    existing_enterprise = group
                if group.get('id') == trunk_group_id:
                    existing_trunk = group
            
            if existing_enterprise:
                self.bridges_config['groups'].remove(existing_enterprise)
            if existing_trunk:
                self.bridges_config['groups'].remove(existing_trunk)
            
            # Step 3: Create enterprise group (rallypoint side)
            print("\n--- Enterprise Group (Rallypoint Side) ---")
            enterprise_group = {
                'id': enterprise_group_id,
                'name': enterprise_group_id,
                'type': 1,  # Audio
                '_wizardCreated': {'type': 'audio'}
            }
            
            # Rallypoint configuration
            print("\nRallypoint Configuration:")
            # Get default rallypoint address from existing groups if available
            default_rp_addr = self.get_existing_rallypoint_address()
            while True:
                rp_addr_input = self._get_host_address_with_abort("Rallypoint address (hostname or IPv4)", default_rp_addr)
                rp_addr = rp_addr_input.strip() if rp_addr_input else ""
                if not rp_addr:
                    print("Error: Rallypoint address is required and cannot be empty.")
                    continue
                break
            
            rp_port = self._get_int_with_abort("Rallypoint port", 7443)
            enterprise_group['rallypoints'] = [{
                'host': {
                    'address': rp_addr,
                    'port': rp_port
                }
            }]
            
            # Audio configuration for enterprise group
            print("\n--- Audio Configuration (Enterprise Group) ---")
            enterprise_group['txAudio'] = {}
            enterprise_group['txAudio']['encoder'] = self._get_audio_encoder_with_abort(25)
            enterprise_group['txAudio']['fdx'] = self._get_yes_no_with_abort("Full duplex?", False)
            enterprise_group['txAudio']['framingMs'] = self._get_int_with_abort("Framing (ms)", 60)
            enterprise_group['txAudio']['maxTxSecs'] = self._get_int_with_abort("Max transmit seconds", 60)
            enterprise_group['txAudio']['noHdrExt'] = self._get_yes_no_with_abort("No header extension?", False)
            
            # Step 4: Create trunk group (multicast side)
            print("\n--- Trunk Group (Multicast Side) ---")
            trunk_group = {
                'id': trunk_group_id,
                'name': trunk_group_id,
                'type': 1,  # Audio
                '_wizardCreated': {'type': 'audio'}
            }
            
            # Multicast configuration
            print("\nMulticast Configuration:")
            # Get default interface name from existing multicast groups if available
            default_interface = self.get_existing_interface_name()
            interface_name = self._get_interface_name_with_abort(default_interface)
            trunk_group['interfaceName'] = interface_name
            
            # RX multicast/unicast address and port - validate combination is unique
            while True:
                rx_addr = self._get_ipv4_address_with_abort("RX multicast/unicast address")
                
                rx_port = self._get_int_with_abort("RX multicast port")
                if rx_port is None:
                    print("Error: RX multicast port is required and cannot be empty.")
                    continue
                
                if self.multicast_address_port_exists(rx_addr, rx_port):
                    print(f"Error: Multicast/unicast address+port combination '{rx_addr}:{rx_port}' is already used by another group.")
                    continue
                break
            
            trunk_group['rx'] = {'address': rx_addr, 'port': rx_port}
            
            # TX defaults to RX values
            tx_addr = self._get_ipv4_address_with_abort("TX multicast/unicast address", rx_addr)
            tx_port = self._get_int_with_abort("TX multicast port", rx_port)
            trunk_group['tx'] = {'address': tx_addr, 'port': tx_port}
            
            ttl = self._get_int_with_abort("TX TTL", 64)
            trunk_group['txOptions'] = {'ttl': ttl}
            
            # Audio configuration for trunk group
            print("\n--- Audio Configuration (Trunk Group) ---")
            trunk_group['txAudio'] = {}
            trunk_group['txAudio']['encoder'] = self._get_audio_encoder_with_abort(None)
            trunk_group['txAudio']['fdx'] = self._get_yes_no_with_abort("Full duplex?", False)
            trunk_group['txAudio']['framingMs'] = self._get_int_with_abort("Framing (ms)", 20)  # Trunk-side default is 20ms
            trunk_group['txAudio']['maxTxSecs'] = self._get_int_with_abort("Max transmit seconds", 60)
            trunk_group['txAudio']['noHdrExt'] = self._get_yes_no_with_abort("No header extension?", True)
            
            # Step 5: Create bridge
            bridge = {
                'id': bridge_id,
                'groups': [enterprise_group_id, trunk_group_id],
                '_wizardCreated': {'type': 'audio'},
                'enabled': True
            }
            
            # Add bridge target output detail to both groups
            enterprise_group['bridgeTargetOutputDetail'] = {'mode': 2}  # bomMixedStream for Audio groups
            trunk_group['bridgeTargetOutputDetail'] = {'mode': 2}  # bomMixedStream for Audio groups
            
            # Add groups and bridge
            self.bridges_config['groups'].append(enterprise_group)
            self.bridges_config['groups'].append(trunk_group)
            self.bridges_config['bridges'].append(bridge)
            
            # Save configuration
            if self.save_bridges_config():
                print(f"\n✓ Bridge '{bridge_id}' created successfully!")
                print(f"  Enterprise group: {enterprise_group_id} (Rallypoint)")
                print(f"  Trunk group: {trunk_group_id} (Multicast)")
                print(f"  Bridge connects: {enterprise_group_id} ↔ {trunk_group_id}")
                self.safe_input("\nPress Enter to continue...")
            else:
                print("\n⚠ Error: Failed to save configuration.")
                self.safe_input("Press Enter to continue...")
        except AbortOperationException:
            return
    
    def wizard_tsm_bridge_multicast_to_rallypoint(self):
        """Wizard to create a TSM bridge from multicast to rallypoint."""
        try:
            self.clear_screen()
            print("\n=== TSM Bridge Wizard (TSM Radio to Rallypoint) ===\n")
            print("This wizard will create:")
            print("  - An Audio group with rallypoint (enterprise side)")
            print("  - An Audio group with multicast for TSM radio (trunk side)")
            print("  - Automatic TSM payload transformations")
            print("  - A bridge connecting both groups")
            print()
            
            # Step 1: Get bridge name and generate ID
            bridge_name = self._get_input_with_abort("Bridge name (e.g., 'TSM Bridge 1')")
            if not bridge_name:
                print("Bridge name is required!")
                self.safe_input("Press Enter to continue...")
                return
            
            bridge_id = self.generate_bridge_id_from_name(bridge_name)
            
            # Step 2: Get stream ID for enterprise side
            stream_id = self._get_input_with_abort("Stream ID for enterprise side")
            if not stream_id:
                print("Stream ID is required!")
                self.safe_input("Press Enter to continue...")
                return
            
            enterprise_group_id = stream_id
            trunk_group_id = f"{stream_id}-trunk"
            
            # Check for existing groups
            existing_enterprise = next((g for g in self.bridges_config.get('groups', []) if g.get('id') == enterprise_group_id), None)
            existing_trunk = next((g for g in self.bridges_config.get('groups', []) if g.get('id') == trunk_group_id), None)
            
            if existing_enterprise or existing_trunk:
                print(f"\nWarning: Groups with IDs '{enterprise_group_id}' or '{trunk_group_id}' already exist.")
                if not self._get_yes_no_with_abort("Replace existing groups?", False):
                    self.safe_input("Press Enter to continue...")
                    return
            
            # Remove existing groups if they exist
            if existing_enterprise:
                self.bridges_config['groups'].remove(existing_enterprise)
            if existing_trunk:
                self.bridges_config['groups'].remove(existing_trunk)
            
            # Step 3: Create enterprise group (rallypoint side)
            print("\n--- Enterprise Group (Rallypoint Side) ---")
            enterprise_group = {
                'id': enterprise_group_id,
                'name': enterprise_group_id,
                'type': 1,  # Audio
                '_wizardCreated': {'type': 'tsm'}
            }
            
            # Rallypoint configuration
            print("\nRallypoint Configuration:")
            default_rp_addr = self.get_existing_rallypoint_address()
            while True:
                rp_addr_input = self._get_host_address_with_abort("Rallypoint address (hostname or IPv4)", default_rp_addr)
                rp_addr = rp_addr_input.strip() if rp_addr_input else ""
                if not rp_addr:
                    print("Error: Rallypoint address is required and cannot be empty.")
                    continue
                break
            
            rp_port = self._get_int_with_abort("Rallypoint port", 7443)
            enterprise_group['rallypoints'] = [{
                'host': {
                    'address': rp_addr,
                    'port': rp_port
                }
            }]
            
            print("\n--- Audio Configuration (Enterprise Group) ---")
            enterprise_group['txAudio'] = {}
            enterprise_encoder_id = self._get_audio_encoder_with_abort(25)  # Any encoder for enterprise side
            enterprise_group['txAudio']['encoder'] = enterprise_encoder_id
            enterprise_group['txAudio']['fdx'] = self._get_yes_no_with_abort("Full duplex?", False)
            enterprise_group['txAudio']['framingMs'] = self._get_int_with_abort("Framing (ms)", 60)
            enterprise_group['txAudio']['maxTxSecs'] = self._get_int_with_abort("Max transmit seconds", 60)
            enterprise_group['txAudio']['noHdrExt'] = self._get_yes_no_with_abort("No header extension?", False)
            
            # No transformations for enterprise group - standard Engage
            
            # Step 4: Create trunk group (multicast side)
            print("\n--- Trunk Group (Multicast Side) ---")
            trunk_group = {
                'id': trunk_group_id,
                'name': trunk_group_id,
                'type': 1,  # Audio
                '_wizardCreated': {'type': 'tsm'}
            }
            
            # Multicast configuration
            print("\nMulticast Configuration:")
            default_interface = self.get_existing_interface_name()
            interface_name = self._get_interface_name_with_abort(default_interface)
            trunk_group['interfaceName'] = interface_name
            
            # RX multicast/unicast address and port - validate combination is unique
            while True:
                rx_addr = self._get_ipv4_address_with_abort("RX multicast/unicast address")
                
                rx_port = self._get_int_with_abort("RX multicast port")
                if rx_port is None:
                    print("Error: RX multicast port is required and cannot be empty.")
                    continue
                
                if self.multicast_address_port_exists(rx_addr, rx_port):
                    print(f"Error: Multicast/unicast address+port combination '{rx_addr}:{rx_port}' is already used by another group.")
                    continue
                break
            
            trunk_group['rx'] = {'address': rx_addr, 'port': rx_port}
            
            # TX defaults to RX values
            tx_addr = self._get_ipv4_address_with_abort("TX multicast/unicast address", rx_addr)
            tx_port = self._get_int_with_abort("TX multicast port", rx_port)
            trunk_group['tx'] = {'address': tx_addr, 'port': tx_port}
            
            ttl = self._get_int_with_abort("TX TTL", 64)
            trunk_group['txOptions'] = {'ttl': ttl}
            
            print("\n--- Audio Configuration (Trunk Group) ---")
            trunk_group['txAudio'] = {}
            trunk_encoder_id = self._get_audio_encoder_for_radio_type_with_abort('tsm', None)  # TSM-compatible encoder for trunk
            if trunk_encoder_id is None:
                print("Error: No TSM-compatible encoders available.")
                self.safe_input("Press Enter to continue...")
                return
            trunk_group['txAudio']['encoder'] = trunk_encoder_id
            trunk_group['txAudio']['fdx'] = self._get_yes_no_with_abort("Full duplex?", False)
            trunk_group['txAudio']['framingMs'] = self._get_int_with_abort("Framing (ms)", 20)  # Trunk-side default is 20ms
            trunk_group['txAudio']['maxTxSecs'] = self._get_int_with_abort("Max transmit seconds", 60)
            trunk_group['txAudio']['noHdrExt'] = self._get_yes_no_with_abort("No header extension?", True)
            
            # Apply TSM transformations to trunk group
            if not self.configure_payload_transformations(trunk_group, trunk_encoder_id, 'tsm'):
                print(f"Warning: TSM transformations not available for encoder {trunk_encoder_id}")
            
            # Step 5: Create bridge
            bridge = {
                'id': bridge_id,
                'groups': [enterprise_group_id, trunk_group_id],
                '_wizardCreated': {'type': 'tsm'},
                'enabled': True
            }
            
            # Add bridge target output detail to both groups
            enterprise_group['bridgeTargetOutputDetail'] = {'mode': 2}  # bomMixedStream for Audio groups
            trunk_group['bridgeTargetOutputDetail'] = {'mode': 2}  # bomMixedStream for Audio groups
            
            # Add groups and bridge
            self.bridges_config['groups'].append(enterprise_group)
            self.bridges_config['groups'].append(trunk_group)
            self.bridges_config['bridges'].append(bridge)
            
            # Save configuration
            if self.save_bridges_config():
                print(f"\n✓ TSM Bridge '{bridge_id}' created successfully!")
                print(f"  Enterprise group: {enterprise_group_id} (Rallypoint with TSM)")
                print(f"  Trunk group: {trunk_group_id} (Multicast with TSM)")
                print(f"  Bridge connects: {enterprise_group_id} ↔ {trunk_group_id}")
                self.safe_input("\nPress Enter to continue...")
            else:
                print("\n⚠ Error: Failed to save configuration.")
                self.safe_input("Press Enter to continue...")
        except AbortOperationException:
            return
    
    def wizard_mpu_bridge_multicast_to_rallypoint(self):
        """Wizard to create an MPU bridge from multicast to rallypoint."""
        try:
            self.clear_screen()
            print("\n=== MPU Bridge Wizard (MPU Radio to Rallypoint) ===\n")
            print("This wizard will create:")
            print("  - An Audio group with rallypoint (enterprise side)")
            print("  - An Audio group with multicast for MPU radio (trunk side)")
            print("  - Automatic MPU payload transformations")
            print("  - A bridge connecting both groups")
            print()
            
            # Step 1: Get bridge name and generate ID
            bridge_name = self._get_input_with_abort("Bridge name (e.g., 'MPU Bridge 1')")
            if not bridge_name:
                print("Bridge name is required!")
                self.safe_input("Press Enter to continue...")
                return
            
            bridge_id = self.generate_bridge_id_from_name(bridge_name)
            
            # Step 2: Get stream ID for enterprise side
            stream_id = self._get_input_with_abort("Stream ID for enterprise side")
            if not stream_id:
                print("Stream ID is required!")
                self.safe_input("Press Enter to continue...")
                return
            
            enterprise_group_id = stream_id
            trunk_group_id = f"{stream_id}-trunk"
            
            # Check for existing groups
            existing_enterprise = next((g for g in self.bridges_config.get('groups', []) if g.get('id') == enterprise_group_id), None)
            existing_trunk = next((g for g in self.bridges_config.get('groups', []) if g.get('id') == trunk_group_id), None)
            
            if existing_enterprise or existing_trunk:
                print(f"\nWarning: Groups with IDs '{enterprise_group_id}' or '{trunk_group_id}' already exist.")
                if not self._get_yes_no_with_abort("Replace existing groups?", False):
                    self.safe_input("Press Enter to continue...")
                    return
            
            # Remove existing groups if they exist
            if existing_enterprise:
                self.bridges_config['groups'].remove(existing_enterprise)
            if existing_trunk:
                self.bridges_config['groups'].remove(existing_trunk)
            
            # Step 3: Create enterprise group (rallypoint side)
            print("\n--- Enterprise Group (Rallypoint Side) ---")
            enterprise_group = {
                'id': enterprise_group_id,
                'name': enterprise_group_id,
                'type': 1,  # Audio
                '_wizardCreated': {'type': 'mpu'}
            }
            
            # Rallypoint configuration
            print("\nRallypoint Configuration:")
            default_rp_addr = self.get_existing_rallypoint_address()
            while True:
                rp_addr_input = self._get_host_address_with_abort("Rallypoint address (hostname or IPv4)", default_rp_addr)
                rp_addr = rp_addr_input.strip() if rp_addr_input else ""
                if not rp_addr:
                    print("Error: Rallypoint address is required and cannot be empty.")
                    continue
                break
            
            rp_port = self._get_int_with_abort("Rallypoint port", 7443)
            enterprise_group['rallypoints'] = [{
                'host': {
                    'address': rp_addr,
                    'port': rp_port
                }
            }]
            
            print("\n--- Audio Configuration (Enterprise Group) ---")
            enterprise_group['txAudio'] = {}
            enterprise_encoder_id = self._get_audio_encoder_with_abort(25)  # Any encoder for enterprise side
            enterprise_group['txAudio']['encoder'] = enterprise_encoder_id
            enterprise_group['txAudio']['fdx'] = self._get_yes_no_with_abort("Full duplex?", False)
            enterprise_group['txAudio']['framingMs'] = self._get_int_with_abort("Framing (ms)", 60)
            enterprise_group['txAudio']['maxTxSecs'] = self._get_int_with_abort("Max transmit seconds", 60)
            enterprise_group['txAudio']['noHdrExt'] = self._get_yes_no_with_abort("No header extension?", False)
            
            # No transformations for enterprise group - standard Engage
            
            # Step 4: Create trunk group (multicast side)
            print("\n--- Trunk Group (Multicast Side) ---")
            trunk_group = {
                'id': trunk_group_id,
                'name': trunk_group_id,
                'type': 1,  # Audio
                '_wizardCreated': {'type': 'mpu'}
            }
            
            # Multicast configuration
            print("\nMulticast Configuration:")
            default_interface = self.get_existing_interface_name()
            interface_name = self._get_interface_name_with_abort(default_interface)
            trunk_group['interfaceName'] = interface_name
            
            # RX multicast/unicast address and port - validate combination is unique
            while True:
                rx_addr = self._get_ipv4_address_with_abort("RX multicast/unicast address")
                
                rx_port = self._get_int_with_abort("RX multicast port")
                if rx_port is None:
                    print("Error: RX multicast port is required and cannot be empty.")
                    continue
                
                if self.multicast_address_port_exists(rx_addr, rx_port):
                    print(f"Error: Multicast/unicast address+port combination '{rx_addr}:{rx_port}' is already used by another group.")
                    continue
                break
            
            trunk_group['rx'] = {'address': rx_addr, 'port': rx_port}
            
            # TX defaults to RX values
            tx_addr = self._get_ipv4_address_with_abort("TX multicast/unicast address", rx_addr)
            tx_port = self._get_int_with_abort("TX multicast port", rx_port)
            trunk_group['tx'] = {'address': tx_addr, 'port': tx_port}
            
            ttl = self._get_int_with_abort("TX TTL", 64)
            trunk_group['txOptions'] = {'ttl': ttl}
            
            print("\n--- Audio Configuration (Trunk Group) ---")
            trunk_group['txAudio'] = {}
            trunk_encoder_id = self._get_audio_encoder_for_radio_type_with_abort('mpu', None)  # MPU-compatible encoder for trunk
            if trunk_encoder_id is None:
                print("Error: No MPU-compatible encoders available.")
                self.safe_input("Press Enter to continue...")
                return
            trunk_group['txAudio']['encoder'] = trunk_encoder_id
            trunk_group['txAudio']['fdx'] = self._get_yes_no_with_abort("Full duplex?", False)
            trunk_group['txAudio']['framingMs'] = self._get_int_with_abort("Framing (ms)", 20)  # Trunk-side default is 20ms
            trunk_group['txAudio']['maxTxSecs'] = self._get_int_with_abort("Max transmit seconds", 60)
            trunk_group['txAudio']['noHdrExt'] = self._get_yes_no_with_abort("No header extension?", True)
            
            # Apply MPU transformations to trunk group
            if not self.configure_payload_transformations(trunk_group, trunk_encoder_id, 'mpu'):
                print(f"Warning: MPU transformations not available for encoder {trunk_encoder_id}")
            
            # Step 5: Create bridge
            bridge = {
                'id': bridge_id,
                'groups': [enterprise_group_id, trunk_group_id],
                '_wizardCreated': {'type': 'mpu'},
                'enabled': True
            }
            
            # Add bridge target output detail to both groups
            enterprise_group['bridgeTargetOutputDetail'] = {'mode': 2}  # bomMixedStream for Audio groups
            trunk_group['bridgeTargetOutputDetail'] = {'mode': 2}  # bomMixedStream for Audio groups
            
            # Add groups and bridge
            self.bridges_config['groups'].append(enterprise_group)
            self.bridges_config['groups'].append(trunk_group)
            self.bridges_config['bridges'].append(bridge)
            
            # Save configuration
            if self.save_bridges_config():
                print(f"\n✓ MPU Bridge '{bridge_id}' created successfully!")
                print(f"  Enterprise group: {enterprise_group_id} (Rallypoint with MPU)")
                print(f"  Trunk group: {trunk_group_id} (Multicast with MPU)")
                print(f"  Bridge connects: {enterprise_group_id} ↔ {trunk_group_id}")
                self.safe_input("\nPress Enter to continue...")
            else:
                print("\n⚠ Error: Failed to save configuration.")
                self.safe_input("Press Enter to continue...")
        except AbortOperationException:
            return
    
    def wizard_silvus_bridge_multicast_to_rallypoint(self):
        """Wizard to create a Silvus bridge from multicast to rallypoint."""
        try:
            self.clear_screen()
            print("\n=== Silvus Bridge Wizard (Silvus Radio to Rallypoint) ===\n")
            print("This wizard will create:")
            print("  - An Audio group with rallypoint (enterprise side)")
            print("  - An Audio group with multicast for Silvus radio (trunk side)")
            print("  - Automatic Silvus payload transformations")
            print("  - A bridge connecting both groups")
            print()
            
            # Step 1: Get bridge name and generate ID
            bridge_name = self._get_input_with_abort("Bridge name (e.g., 'Silvus Bridge 1')")
            if not bridge_name:
                print("Bridge name is required!")
                self.safe_input("Press Enter to continue...")
                return
            
            bridge_id = self.generate_bridge_id_from_name(bridge_name)
            
            # Step 2: Get stream ID for enterprise side
            stream_id = self._get_input_with_abort("Stream ID for enterprise side")
            if not stream_id:
                print("Stream ID is required!")
                self.safe_input("Press Enter to continue...")
                return
            
            enterprise_group_id = stream_id
            trunk_group_id = f"{stream_id}-trunk"
            
            # Check for existing groups
            existing_enterprise = next((g for g in self.bridges_config.get('groups', []) if g.get('id') == enterprise_group_id), None)
            existing_trunk = next((g for g in self.bridges_config.get('groups', []) if g.get('id') == trunk_group_id), None)
            
            if existing_enterprise or existing_trunk:
                print(f"\nWarning: Groups with IDs '{enterprise_group_id}' or '{trunk_group_id}' already exist.")
                if not self._get_yes_no_with_abort("Replace existing groups?", False):
                    self.safe_input("Press Enter to continue...")
                    return
            
            # Remove existing groups if they exist
            if existing_enterprise:
                self.bridges_config['groups'].remove(existing_enterprise)
            if existing_trunk:
                self.bridges_config['groups'].remove(existing_trunk)
            
            # Step 3: Create enterprise group (rallypoint side)
            print("\n--- Enterprise Group (Rallypoint Side) ---")
            enterprise_group = {
                'id': enterprise_group_id,
                'name': enterprise_group_id,
                'type': 1,  # Audio
                '_wizardCreated': {'type': 'silvus'}
            }
            
            # Rallypoint configuration
            print("\nRallypoint Configuration:")
            default_rp_addr = self.get_existing_rallypoint_address()
            while True:
                rp_addr_input = self._get_host_address_with_abort("Rallypoint address (hostname or IPv4)", default_rp_addr)
                rp_addr = rp_addr_input.strip() if rp_addr_input else ""
                if not rp_addr:
                    print("Error: Rallypoint address is required and cannot be empty.")
                    continue
                break
            
            rp_port = self._get_int_with_abort("Rallypoint port", 7443)
            enterprise_group['rallypoints'] = [{
                'host': {
                    'address': rp_addr,
                    'port': rp_port
                }
            }]
            
            print("\n--- Audio Configuration (Enterprise Group) ---")
            enterprise_group['txAudio'] = {}
            enterprise_encoder_id = self._get_audio_encoder_with_abort(25)  # Any encoder for enterprise side
            enterprise_group['txAudio']['encoder'] = enterprise_encoder_id
            enterprise_group['txAudio']['fdx'] = self._get_yes_no_with_abort("Full duplex?", False)
            enterprise_group['txAudio']['framingMs'] = self._get_int_with_abort("Framing (ms)", 60)
            enterprise_group['txAudio']['maxTxSecs'] = self._get_int_with_abort("Max transmit seconds", 60)
            enterprise_group['txAudio']['noHdrExt'] = self._get_yes_no_with_abort("No header extension?", False)
            
            # No transformations for enterprise group - standard Engage
            
            # Step 4: Create trunk group (multicast side)
            print("\n--- Trunk Group (Multicast Side) ---")
            trunk_group = {
                'id': trunk_group_id,
                'name': trunk_group_id,
                'type': 1,  # Audio
                '_wizardCreated': {'type': 'silvus'}
            }
            
            # Multicast configuration
            print("\nMulticast Configuration:")
            default_interface = self.get_existing_interface_name()
            interface_name = self._get_interface_name_with_abort(default_interface)
            trunk_group['interfaceName'] = interface_name
            
            # RX multicast/unicast address and port - validate combination is unique
            while True:
                rx_addr = self._get_ipv4_address_with_abort("RX multicast/unicast address")
                
                rx_port = self._get_int_with_abort("RX multicast port")
                if rx_port is None:
                    print("Error: RX multicast port is required and cannot be empty.")
                    continue
                
                if self.multicast_address_port_exists(rx_addr, rx_port):
                    print(f"Error: Multicast/unicast address+port combination '{rx_addr}:{rx_port}' is already used by another group.")
                    continue
                break
            
            trunk_group['rx'] = {'address': rx_addr, 'port': rx_port}
            
            # TX defaults to RX values
            tx_addr = self._get_ipv4_address_with_abort("TX multicast/unicast address", rx_addr)
            tx_port = self._get_int_with_abort("TX multicast port", rx_port)
            trunk_group['tx'] = {'address': tx_addr, 'port': tx_port}
            
            ttl = self._get_int_with_abort("TX TTL", 64)
            trunk_group['txOptions'] = {'ttl': ttl}
            
            print("\n--- Audio Configuration (Trunk Group) ---")
            trunk_group['txAudio'] = {}
            trunk_encoder_id = self._get_audio_encoder_for_radio_type_with_abort('silvus', None)  # Silvus-compatible encoder for trunk
            if trunk_encoder_id is None:
                print("Error: No Silvus-compatible encoders available.")
                self.safe_input("Press Enter to continue...")
                return
            trunk_group['txAudio']['encoder'] = trunk_encoder_id
            trunk_group['txAudio']['fdx'] = self._get_yes_no_with_abort("Full duplex?", False)
            trunk_group['txAudio']['framingMs'] = self._get_int_with_abort("Framing (ms)", 20)  # Trunk-side default is 20ms
            trunk_group['txAudio']['maxTxSecs'] = self._get_int_with_abort("Max transmit seconds", 60)
            trunk_group['txAudio']['noHdrExt'] = self._get_yes_no_with_abort("No header extension?", True)
            
            # Apply Silvus transformations to trunk group
            if not self.configure_payload_transformations(trunk_group, trunk_encoder_id, 'silvus'):
                print(f"Warning: Silvus transformations not available for encoder {trunk_encoder_id}")
            
            # Step 5: Create bridge
            bridge = {
                'id': bridge_id,
                'groups': [enterprise_group_id, trunk_group_id],
                '_wizardCreated': {'type': 'silvus'},
                'enabled': True
            }
            
            # Add bridge target output detail to both groups
            enterprise_group['bridgeTargetOutputDetail'] = {'mode': 2}  # bomMixedStream for Audio groups
            trunk_group['bridgeTargetOutputDetail'] = {'mode': 2}  # bomMixedStream for Audio groups
            
            # Add groups and bridge
            self.bridges_config['groups'].append(enterprise_group)
            self.bridges_config['groups'].append(trunk_group)
            self.bridges_config['bridges'].append(bridge)
            
            # Save configuration
            if self.save_bridges_config():
                print(f"\n✓ Silvus Bridge '{bridge_id}' created successfully!")
                print(f"  Enterprise group: {enterprise_group_id} (Rallypoint with Silvus)")
                print(f"  Trunk group: {trunk_group_id} (Multicast with Silvus)")
                print(f"  Bridge connects: {enterprise_group_id} ↔ {trunk_group_id}")
                self.safe_input("\nPress Enter to continue...")
            else:
                print("\n⚠ Error: Failed to save configuration.")
                self.safe_input("Press Enter to continue...")
        except AbortOperationException:
            return
    
    def bridges_config_menu(self):
        """Main application menu - Engage Bridging Service Configuration."""
        while True:
            self.clear_screen()
            # Display permission warnings first so they're always visible
            self.display_permission_warnings()
            print(f"\n{self.header(f'=== Engage Bridging Service Configuration v{VERSION} ===')}")
            print(f"{self.info('Copyright (c) 2025 Rally Tactical Systems Inc')}")
            print(f"Working on: {self.bridges_config_path}")
            # Calculate separator length to match header (accounting for color codes)
            header_text = f'=== Engage Bridging Service Configuration v{VERSION} ==='
            separator_length = len(header_text)
            print(f"{self.header('=' * separator_length)}")
            print()
            
            # Display bridges and their groups in table format (with wizard indicator and numbers)
            print(f"{self.header('Bridges:')}")
            self.display_bridges_table(show_numbers=True, show_wizard_indicator=True)
            
            # Display available wizards
            print(f"{self.info('Add bridge wizards:')}")
            print(f"{self.menu_option('1.')} Raw Bridge (Multicast to Rallypoint)")
            print(f"{self.menu_option('2.')} Audio Bridge (Multicast to Rallypoint)")
            print(f"{self.menu_option('3.')} TSM Bridge (TSM Radio to Rallypoint)")
            print(f"{self.menu_option('4.')} MPU Bridge (MPU Radio to Rallypoint)")
            print(f"{self.menu_option('5.')} Silvus Bridge (Silvus Radio to Rallypoint)")
            print()
            
            # Menu options
            print(f"{self.menu_option('6.')} {self.colored('Remove', Colors.RED)} (enter number)")
            print(f"{self.menu_option('7.')} Toggle Bridge Enabled (enter number)")
            print(f"{self.menu_option('8.')} Edit Bridges & Groups")
            print(f"{self.menu_option('9.')} EBS Service Configuration")
            print(f"{self.menu_option('q.')} Quit")
            
            choice = self.menu_input("\nSelect option: ").strip().lower()
            
            if choice == '1':
                self.wizard_raw_bridge_multicast_to_rallypoint()
            elif choice == '2':
                self.wizard_audio_bridge_multicast_to_rallypoint()
            elif choice == '3':
                self.wizard_tsm_bridge_multicast_to_rallypoint()
            elif choice == '4':
                self.wizard_mpu_bridge_multicast_to_rallypoint()
            elif choice == '5':
                self.wizard_silvus_bridge_multicast_to_rallypoint()
            elif choice == '6':
                self.handle_remove_bridge_by_number()
            elif choice == '7':
                self.toggle_bridge_enabled()
            elif choice == '8':
                self.edit_bridges_submenu()
            elif choice == '9':
                self.edit_main_config()
            elif choice == 'q':
                if self.handle_quit():
                    break
            else:
                print("Invalid option. Please try again.")
                self.safe_input("Press Enter to continue...")
    
    def add_bridge_interactive(self):
        """Walk user through adding a new bridge."""
        try:
            self.clear_screen()
            print("\n=== Add New Bridge ===\n")
            
            if 'bridges' not in self.bridges_config:
                self.bridges_config['bridges'] = []
            
            if 'groups' not in self.bridges_config or not self.bridges_config['groups']:
                print("No groups available. Please add groups first.")
                self.safe_input("Press Enter to continue...")
                return
            
            # Get bridge ID
            bridge_id = self._get_input_with_abort("Bridge ID")
            if not bridge_id:
                print("Bridge ID is required!")
                self.safe_input("Press Enter to continue...")
                return
            
            # Check if bridge already exists
            for existing in self.bridges_config['bridges']:
                if existing.get('id') == bridge_id:
                    if not self._get_yes_no_with_abort(f"Bridge '{bridge_id}' already exists. Replace it?", False):
                        self.safe_input("Press Enter to continue...")
                        return
                    self.bridges_config['bridges'].remove(existing)
                    break
            
            # Show available groups
            print("\nAvailable groups:")
            groups = self.bridges_config['groups']
            self.display_groups_table(groups, show_numbers=True)
            
            # Get group selections
            print("\nSelect groups to bridge (enter numbers separated by commas, e.g., 1,2,3)")
            selection = self.safe_input("Groups: ", detect_esc=True).strip()
            
            if selection == 'ESC':
                print()  # Move to next line before showing abort confirmation
                self._confirm_abort()
                return
            
            if not selection:
                print("No groups selected. Bridge not created.")
                self.safe_input("Press Enter to continue...")
                return
            
            try:
                indices = [int(x.strip()) - 1 for x in selection.split(',')]
                selected_groups = []
                for idx in indices:
                    if 0 <= idx < len(groups):
                        selected_groups.append(groups[idx].get('id'))
                    else:
                        print(f"Warning: Invalid group number {idx + 1}")
                
                if not selected_groups:
                    print("No valid groups selected. Bridge not created.")
                    self.safe_input("Press Enter to continue...")
                    return
                
                # Create the bridge
                bridge = {
                    'id': bridge_id,
                    'groups': selected_groups,
                    'enabled': True
                }
                
                self.bridges_config['bridges'].append(bridge)
                
                if self.save_bridges_config():
                    print(f"\n✓ Bridge '{bridge_id}' created successfully!")
                    print(f"  Bridging groups: {', '.join(selected_groups)}")
                    self.safe_input("Press Enter to continue...")
            except ValueError:
                print("Invalid input. Please enter numbers separated by commas.")
                self.safe_input("Press Enter to continue...")
        except AbortOperationException:
            return
    
    def handle_remove_bridge_by_number(self):
        """Handle removing a bridge by its display number (inline, no screen clearing)."""
        try:
            # Get valid number range
            bridges = self.bridges_config.get('bridges', [])
            if not bridges:
                print("No bridges configured.")
                self.safe_input("Press Enter to continue...")
                return
            
            min_num = 1
            max_num = len(bridges)
            
            try:
                number_str = self._get_input_with_abort(f"Enter bridge number to remove ({min_num}-{max_num}): ")
                if not number_str.strip():
                    return  # User cancelled
                    
                number = int(number_str)
                
                if number < min_num or number > max_num:
                    print(f"Error: Number must be between {min_num} and {max_num}")
                    self.safe_input("Press Enter to continue...")
                    return
                
                # Convert to 0-based index
                idx = number - 1
                bridge = bridges[idx]
                bridge_id = bridge.get('id', 'Unknown')
                
                # Confirm removal
                if not self._get_yes_no_with_abort(f"Are you sure you want to remove bridge '{bridge_id}'?", False):
                    return
                
                # Use the same removal logic as the unified interface
                self.remove_bridge_by_id(bridge_id)
                            
            except ValueError:
                print("Error: Please enter a valid number.")
                self.safe_input("Press Enter to continue...")
            except AbortOperationException:
                raise
        except AbortOperationException:
            return
        except Exception as e:
            print(f"Error removing bridge: {e}")
            self.safe_input("Press Enter to continue...")
    
    def toggle_bridge_enabled(self):
        """Toggle the enabled status of a bridge."""
        try:
            self.clear_screen()
            print("\n=== Toggle Bridge Enabled ===\n")
            
            if 'bridges' not in self.bridges_config or not self.bridges_config['bridges']:
                print("No bridges configured.")
                self.safe_input("Press Enter to continue...")
                return
            
            print("Available bridges:")
            self.display_bridges_table(show_numbers=True, show_wizard_indicator=True)
            
            choice = self.menu_input("Enter bridge number to toggle (or 'c' to cancel, Esc to go back): ").strip()
            
            if choice.lower() == 'c' or choice == 'b':
                return
            
            try:
                idx = int(choice) - 1
                if not (0 <= idx < len(self.bridges_config['bridges'])):
                    print("Invalid bridge number.")
                    self.safe_input("Press Enter to continue...")
                    return
                
                bridge = self.bridges_config['bridges'][idx]
                bridge_id = bridge.get('id', 'N/A')
                current_enabled = bridge.get('enabled', True)
                new_enabled = not current_enabled
                
                bridge['enabled'] = new_enabled
                
                if self.save_bridges_config():
                    status_str = "enabled" if new_enabled else "disabled"
                    print(f"\n✓ Bridge '{bridge_id}' {status_str} successfully!")
                    self.safe_input("Press Enter to continue...")
                else:
                    print("\n⚠ Error: Failed to save configuration.")
                    self.safe_input("Press Enter to continue...")
                            
            except ValueError:
                print("Error: Please enter a valid number.")
                self.safe_input("Press Enter to continue...")
            except AbortOperationException:
                raise
        except AbortOperationException:
            return
        except Exception as e:
            print(f"Error toggling bridge: {e}")
            self.safe_input("Press Enter to continue...")
    
    def edit_bridge_interactive(self):
        """Edit an existing bridge."""
        self.clear_screen()
        print("\n=== Edit Bridge ===\n")
        
        if 'bridges' not in self.bridges_config or not self.bridges_config['bridges']:
            print("No bridges configured.")
            self.safe_input("Press Enter to continue...")
            return
        
        print("Available bridges:")
        self.display_bridges_table(show_numbers=True)
        
        choice = self.menu_input("Enter bridge number to edit (or 'c' to cancel, Esc to go back): ").strip()
        
        if choice.lower() == 'c' or choice == 'b':
            return
        
        try:
            idx = int(choice) - 1
            if not (0 <= idx < len(self.bridges_config['bridges'])):
                print("Invalid bridge number.")
                self.safe_input("Press Enter to continue...")
                return
            
            bridge = self.bridges_config['bridges'][idx]
            original_id = bridge.get('id', '')
            original_groups = bridge.get('groups', []).copy()
            
            self.clear_screen()
            print(f"\n=== Editing Bridge: {original_id} ===\n")
            
            # Edit bridge ID
            bridge_id = self.get_input("Bridge ID", original_id)
            if not bridge_id:
                print("Bridge ID is required!")
                self.safe_input("Press Enter to continue...")
                return
            
            # If ID changed, check for conflicts
            if bridge_id != original_id:
                for existing in self.bridges_config['bridges']:
                    if existing.get('id') == bridge_id and existing != bridge:
                        print(f"Error: Bridge ID '{bridge_id}' already exists!")
                        self.safe_input("Press Enter to continue...")
                        return
            
            # Show current groups
            if original_groups:
                print(f"\nCurrent bridged groups: {', '.join(original_groups)}")
            
            # Show available groups
            if 'groups' not in self.bridges_config or not self.bridges_config['groups']:
                print("No groups available.")
                self.safe_input("Press Enter to continue...")
                return
            
            print("\nAvailable groups:")
            groups = self.bridges_config['groups']
            
            # Build a map of group IDs to their indices for pre-filling
            group_id_to_index = {}
            current_group_indices = []
            
            # Display table with status
            self.display_groups_table(groups, show_numbers=True, show_status=True, current_groups=original_groups)
            
            # Build indices for pre-filling
            for i, group in enumerate(groups, 1):
                group_id = group.get('id', '')
                group_id_to_index[group_id] = i
                if group_id in original_groups:
                    current_group_indices.append(str(i))
            
            # Pre-fill with current group indices
            default_selection = ','.join(current_group_indices) if current_group_indices else ""
            
            # Get group selections with custom prompt format
            print("\nSelect groups to bridge (enter numbers separated by commas, e.g., 1,2,3)")
            if default_selection:
                # Show format: Groups [1,2]: 1,2 
                # [1,2] shows previous groups, 1,2 is pre-filled in the input
                prompt = f"Groups [{default_selection}]: "
                
                # Pre-fill the input buffer if readline is available
                selection = ""
                if READLINE_AVAILABLE:
                    # Create a closure to capture the value
                    prefill_value = default_selection
                    def prefill_hook():
                        readline.insert_text(prefill_value)
                        # Remove the hook after it's been called once
                        readline.set_startup_hook(None)
                    readline.set_startup_hook(prefill_hook)
                    
                    try:
                        # Use regular input() to ensure readline hook works
                        selection = input(prompt).strip()
                    except KeyboardInterrupt:
                        # If Ctrl+C, handle it via safe_input (without pre-fill)
                        readline.set_startup_hook(None)  # Clear hook first
                        selection = self.safe_input(prompt).strip()
                    finally:
                        # Clean up readline hook
                        try:
                            readline.set_startup_hook(None)
                        except:
                            pass
                else:
                    # No readline, use safe_input
                    selection = self.safe_input(prompt).strip()
                
                if not selection:
                    # If empty, use the default
                    selection = default_selection
            else:
                selection = self.safe_input("Groups: ").strip()
            
            if selection:
                try:
                    indices = [int(x.strip()) - 1 for x in selection.split(',')]
                    selected_groups = []
                    for idx in indices:
                        if 0 <= idx < len(groups):
                            selected_groups.append(groups[idx].get('id'))
                        else:
                            print(f"Warning: Invalid group number {idx + 1}")
                    
                    if not selected_groups:
                        print("No valid groups selected. Keeping current groups.")
                        selected_groups = original_groups
                except ValueError:
                    print("Invalid input. Keeping current groups.")
                    selected_groups = original_groups
            else:
                selected_groups = original_groups
            
            # Update the bridge
            bridge['id'] = bridge_id
            bridge['groups'] = selected_groups
            
            if self.save_bridges_config():
                print(f"\n✓ Bridge '{bridge_id}' updated successfully!")
                print(f"  Bridging groups: {', '.join(selected_groups)}")
                self.safe_input("Press Enter to continue...")
        except ValueError:
            print("Invalid input.")
            self.safe_input("Press Enter to continue...")
    
    def main_config_needs_setup(self) -> bool:
        """Check if main config needs any setup/configuration."""
        # Check if EBS ID is missing or empty
        id_empty = not self.main_config.get('id') or self.main_config.get('id').strip() == ''
        
        # Check if license is missing or empty
        engine_policy = self.main_config.get('enginePolicy', {})
        licensing = engine_policy.get('licensing', {})
        license_empty = (
            not licensing.get('entitlement') or licensing.get('entitlement').strip() == '' or
            not licensing.get('key') or licensing.get('key').strip() == ''
        )
        
        # Check if featureset is missing or empty
        featureset = engine_policy.get('featureset', {})
        featureset_empty = (
            not featureset.get('signature') or featureset.get('signature').strip() == '' or
            not featureset.get('features') or len(featureset.get('features', [])) == 0
        )
        
        # If anything is missing, main config needs setup
        return id_empty or license_empty or featureset_empty
    
    def run(self):
        """Application entry point - starts with Bridges menu as top menu."""
        self.load_configs()
        
        # Check and initialize required fields BEFORE showing menu
        self.check_and_initialize_config()
        
        try:
            # Start directly with Bridges menu as the top-level menu
            self.bridges_config_menu()
        except KeyboardInterrupt:
            print("\n\n⚠ Interrupted by user (Ctrl+C)")
            
            # If no changes were made, simply exit
            if not self.changes_made:
                self.cleanup_backups()
                print("\nGoodbye!")
                sys.exit(0)
            
            # Changes were made, show options
            print("\nOptions:")
            print("1. Quit")
            print("2. Revert all changes and Quit")
            print("3. Continue editing")
            
            while True:
                try:
                    choice = self.safe_input("\nSelect option (1-3): ").strip()
                    if choice == '1':
                        # Exit - changes are already saved (they're saved immediately after each edit)
                        # Just clean up backups
                        self.cleanup_backups()
                        print("\nGoodbye!")
                        sys.exit(0)
                    elif choice == '2':
                        # Revert all changes and exit - restore from backup
                        self.restore_from_backup()
                        # restore_from_backup already cleans up backups
                        print("\nReverted all changes. Goodbye!")
                        sys.exit(0)
                    elif choice == '3':
                        # Continue the loop
                        break
                    else:
                        print("Invalid option. Please enter 1, 2, or 3.")
                except KeyboardInterrupt:
                    # If they press Ctrl+C again, just exit
                    print("\n\nExiting...")
                    sys.exit(0)
            # User chose to continue, loop will restart


def main():
    # Check for help flag
    if len(sys.argv) > 1 and sys.argv[1] in ('-h', '--help'):
        show_usage()
        sys.exit(0)
    
    # Default filenames
    default_main_filename = "engagebridged_conf.json"
    default_bridges_filename = "bridges.json"
    
    # Determine paths based on arguments
    if len(sys.argv) == 1:
        # No arguments: use current directory with default filenames
        main_config_path = default_main_filename
        bridges_config_path = default_bridges_filename
    elif len(sys.argv) == 2:
        # One argument: could be a directory or a file
        arg = sys.argv[1]
        arg_path = Path(arg)
        
        if arg_path.is_dir():
            # It's a directory: use default filenames in that directory
            main_config_path = arg_path / default_main_filename
            bridges_config_path = arg_path / default_bridges_filename
        elif arg_path.is_file():
            # It's a file: use it as main config, look for bridges.json in same directory
            main_config_path = arg_path
            bridges_config_path = arg_path.parent / default_bridges_filename
        else:
            # Assume it's a file path (might not exist yet)
            main_config_path = arg_path
            bridges_config_path = arg_path.parent / default_bridges_filename
    else:
        # Two arguments: treat as two file paths
        main_config_path = Path(sys.argv[1])
        bridges_config_path = Path(sys.argv[2])
    
    # Convert to strings for EBSConfigTool
    tool = EBSConfigTool(str(main_config_path), str(bridges_config_path))
    tool.run()


if __name__ == '__main__':
    main()

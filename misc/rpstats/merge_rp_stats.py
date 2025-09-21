#!/usr/bin/env python3
#
# Rallypoint Stream Statistics CSV Merger
# Copyright (c) 2020 Rally Tactical Systems, Inc.
#

"""
This script processes Rallypoint stream statistics CSV files, merges them,
and outputs a chronologically ordered CSV to stdout.

Usage:
    python3 merge_rp_stats.py <path_spec_with_wildcards>

Examples:
    python3 merge_rp_stats.py "/path/to/RP-BILLING-01_*_stream_stats.csv"
    python3 merge_rp_stats.py "/data/exports/*stream_stats.csv"
    python3 merge_rp_stats.py "/tmp/RP-*.csv"

Features:
    - Processes files in sequence order (000001, 000002, etc.)
    - Merges all CSV files chronologically
    - Adds metadata columns (sequenceNumber, instanceId, sourceFile)
    - Handles missing or corrupted files gracefully
    - Outputs to stdout for piping to other tools
"""

import os
import sys
import csv
import glob
import re
from datetime import datetime
from typing import List, Tuple, Dict


def parse_filename(filename: str) -> Tuple[str, int, str]:
    """
    Parse Rallypoint CSV filename to extract components.
    
    Expected format: RP-BILLING-01_da0f1f46c96247fcac6f31853dfbefdd_00000X_stream_stats.csv
    
    Returns:
        Tuple of (instance_id, sequence_number, full_path)
    """
    basename = os.path.basename(filename)
    
    # Extract the sequence number from the filename
    match = re.search(r'_(\d{6})_stream_stats\.csv$', basename)
    if not match:
        raise ValueError(f"Invalid filename format: {basename}")
    
    sequence_number = int(match.group(1))
    
    # Extract instance ID (the long hex string)
    instance_match = re.search(r'RP-BILLING-01_([a-f0-9]{32})_', basename)
    if not instance_match:
        raise ValueError(f"Could not extract instance ID from: {basename}")
    
    instance_id = instance_match.group(1)
    
    return instance_id, sequence_number, filename


def read_csv_file(filepath: str) -> List[Dict]:
    """
    Read a CSV file and return list of dictionaries.
    
    Returns:
        List of dictionaries representing CSV rows
    """
    rows = []
    try:
        with open(filepath, 'r', newline='', encoding='utf-8') as csvfile:
            reader = csv.DictReader(csvfile)
            for row in reader:
                # Convert timestamp to integer for sorting
                if 'exportTimestampMs' in row:
                    row['exportTimestampMs'] = int(row['exportTimestampMs'])
                rows.append(row)
    except Exception as e:
        print(f"Error reading {filepath}: {e}", file=sys.stderr)
        return []
    
    return rows


def process_stream_stats_files(path_spec: str, verbose: bool = False) -> None:
    """
    Process Rallypoint stream statistics files and output merged CSV to stdout.
    
    Args:
        path_spec: Path specification with wildcards for CSV files
        verbose: If True, output progress information to stderr
    """
    # Find all matching CSV files using the provided path spec
    csv_files = glob.glob(path_spec)
    
    if not csv_files:
        if verbose:
            print(f"No matching CSV files found for pattern: {path_spec}", file=sys.stderr)
        return
    
    # Parse filenames and sort by sequence number
    file_info = []
    for csv_file in csv_files:
        try:
            instance_id, seq_num, full_path = parse_filename(csv_file)
            file_info.append((seq_num, instance_id, full_path))
        except ValueError as e:
            if verbose:
                print(f"Skipping invalid file {csv_file}: {e}", file=sys.stderr)
            continue
    
    if not file_info:
        if verbose:
            print("No valid CSV files found", file=sys.stderr)
        return
    
    # Sort by sequence number
    file_info.sort(key=lambda x: x[0])
    
    if verbose:
        print(f"Processing {len(file_info)} CSV files...", file=sys.stderr)
    
    # Read and merge all CSV data
    all_rows = []
    fieldnames = None
    
    for seq_num, instance_id, filepath in file_info:
        rows = read_csv_file(filepath)
        
        if not rows:
            if verbose:
                print(f"No data found in {filepath}", file=sys.stderr)
            continue
        
        # Set fieldnames from first file
        if fieldnames is None:
            fieldnames = list(rows[0].keys())
        
        # Add sequence number and instance ID to each row
        for row in rows:
            row['sequenceNumber'] = seq_num
            row['instanceId'] = instance_id
            row['sourceFile'] = os.path.basename(filepath)
        
        all_rows.extend(rows)
        if verbose:
            print(f"Processed {os.path.basename(filepath)}: {len(rows)} rows", file=sys.stderr)
    
    if not all_rows:
        if verbose:
            print("No data to process", file=sys.stderr)
        return
    
    # Sort all rows chronologically by timestamp
    all_rows.sort(key=lambda x: x['exportTimestampMs'])
    
    # Add additional fieldnames for the output
    output_fieldnames = ['sequenceNumber', 'instanceId', 'sourceFile'] + fieldnames
    
    # Write merged CSV to stdout
    writer = csv.DictWriter(sys.stdout, fieldnames=output_fieldnames)
    writer.writeheader()
    writer.writerows(all_rows)
    
    # Print summary to stderr only if verbose
    if verbose:
        print(f"\nSummary:", file=sys.stderr)
        print(f"  Total files processed: {len(file_info)}", file=sys.stderr)
        print(f"  Total rows: {len(all_rows)}", file=sys.stderr)
        
        # Count unique streams/groups
        unique_streams = set(row['id'] for row in all_rows)
        print(f"  Unique streams/groups: {len(unique_streams)}", file=sys.stderr)
        
        # Time range
        if all_rows:
            first_timestamp = all_rows[0]['exportTimestampMs']
            last_timestamp = all_rows[-1]['exportTimestampMs']
            first_dt = datetime.fromtimestamp(first_timestamp / 1000)
            last_dt = datetime.fromtimestamp(last_timestamp / 1000)
            print(f"  Time range: {first_dt} to {last_dt}", file=sys.stderr)
            print(f"  Duration: {last_timestamp - first_timestamp} ms", file=sys.stderr)


def main():
    """Main function with command line argument parsing."""
    if len(sys.argv) < 2 or len(sys.argv) > 3:
        print("Usage: python3 merge_rp_stats.py <path_spec_with_wildcards> [--verbose]", file=sys.stderr)
        print("", file=sys.stderr)
        print("Examples:", file=sys.stderr)
        print("  python3 merge_rp_stats.py \"/path/to/RP-BILLING-01_*_stream_stats.csv\"", file=sys.stderr)
        print("  python3 merge_rp_stats.py \"/data/exports/*stream_stats.csv\" --verbose", file=sys.stderr)
        print("  python3 merge_rp_stats.py \"/tmp/RP-*.csv\"", file=sys.stderr)
        sys.exit(1)
    
    path_spec = sys.argv[1]
    verbose = len(sys.argv) == 3 and sys.argv[2] == "--verbose"
    
    try:
        process_stream_stats_files(path_spec, verbose)
    except Exception as e:
        print(f"Error processing files: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
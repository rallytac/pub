#!/bin/bash

# Color codes
RED="\033[0;31m"
GREEN="\033[0;32m"
YELLOW="\033[0;33m"
CYAN="\033[0;36m"
RESET="\033[0m"

# Default settings
COLOR=true
FIPS_CHECK=false
SUMMARY=false

# Function to apply colors if enabled
colorize() {
    if [[ "$COLOR" == "true" ]]; then
        echo -e "$1$2$RESET"
    else
        echo "$2"
    fi
}

# Function to parse date for Linux or macOS
parse_date() {
    local input_date="$1"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        date -j -f "%b %e %H:%M:%S %Y %Z" "$input_date" +%s
    else
        date -d "$input_date" +%s
    fi
}

# Function to check if the certificate can act as a CA
check_ca_usage() {
    if echo "$CERT_CONTENT" | openssl x509 -noout -text | grep -q "CA:TRUE"; then
        if [[ "$SUMMARY" == "true" ]]; then
            echo "ca_usage:yes"
        else
            colorize "$GREEN" "CA Usage: Certificate can act as a CA."
        fi
    else
        if [[ "$SUMMARY" == "true" ]]; then
            echo "ca_usage:no"
        else
            colorize "$RED" "CA Usage: Certificate cannot act as a CA."
        fi
    fi
}

# Function to check validity dates
check_validity() {
    local start_date=$(echo "$CERT_CONTENT" | openssl x509 -noout -startdate | cut -d= -f2)
    local end_date=$(echo "$CERT_CONTENT" | openssl x509 -noout -enddate | cut -d= -f2)
    local start_ts=$(parse_date "$start_date")
    local end_ts=$(parse_date "$end_date")
    local now_ts=$(date +%s)

    if [[ "$SUMMARY" == "true" ]]; then
        echo "start_date:$start_date"
        echo "end_date:$end_date"
        if [[ $now_ts -lt $start_ts ]]; then
            echo "validity:not_yet_valid"
        elif [[ $now_ts -gt $end_ts ]]; then
            echo "validity:expired"
        else
            echo "validity:valid"
        fi
    else
        colorize "$CYAN" "Validity Period:"
        echo "  Start Date: $start_date"
        echo "  End Date:   $end_date"
        if [[ $now_ts -lt $start_ts ]]; then
            colorize "$YELLOW" "  Status: Certificate is not yet valid."
        elif [[ $now_ts -gt $end_ts ]]; then
            colorize "$RED" "  Status: Certificate has expired."
        else
            colorize "$GREEN" "  Status: Certificate is currently valid."
        fi
    fi
}

# Function to check the signing algorithm
check_algorithm() {
    local algorithm=$(echo "$CERT_CONTENT" | openssl x509 -noout -text | grep "Signature Algorithm" | head -n 1 | awk '{print $3}')
    if [[ "$SUMMARY" == "true" ]]; then
        echo "signature_algorithm:$algorithm"
        if [[ $algorithm == "sha1" || $algorithm == "md5" ]]; then
            echo "algorithm_status:weak"
        else
            echo "algorithm_status:strong"
        fi
    else
        colorize "$CYAN" "Signature Algorithm: $algorithm"
        if [[ $algorithm == "sha1" || $algorithm == "md5" ]]; then
            colorize "$RED" "  Warning: Weak or deprecated algorithm detected!"
        else
            colorize "$GREEN" "  Status: Algorithm is strong."
        fi
    fi
}

# Function to check for FIPS-approved algorithms
check_fips_approval() {
    if [[ "$FIPS_CHECK" != "true" ]]; then
        return
    fi

    local fips_approved=true
    local disallowed_algorithms=("sha1" "md5" "dsa")

    if [[ "$SUMMARY" == "true" ]]; then
        for algo in "${disallowed_algorithms[@]}"; do
            if echo "$CERT_CONTENT" | openssl x509 -noout -text | grep -qi "$algo"; then
                echo "fips_status:non_fips"
                return
            fi
        done
        echo "fips_status:fips_approved"
    else
        colorize "$CYAN" "FIPS Approval Check:"
        for algo in "${disallowed_algorithms[@]}"; do
            if echo "$CERT_CONTENT" | openssl x509 -noout -text | grep -qi "$algo"; then
                colorize "$RED" "  Disallowed algorithm detected: $algo"
                fips_approved=false
            fi
        done
        if [[ "$fips_approved" == "true" ]]; then
            colorize "$GREEN" "  Status: All algorithms are FIPS-approved."
        else
            colorize "$RED" "  Status: Certificate uses non-FIPS-approved algorithms."
        fi
    fi
}

# Function to check expiration warnings
check_expiration_warning() {
    local end_date=$(echo "$CERT_CONTENT" | openssl x509 -noout -enddate | cut -d= -f2)
    local end_ts=$(parse_date "$end_date")
    local now_ts=$(date +%s)
    local warning_days=30
    local warning_ts=$((now_ts + warning_days * 86400))

    if [[ "$SUMMARY" == "true" ]]; then
        if [[ $end_ts -le $warning_ts ]]; then
            echo "expiration_warning:yes"
        else
            echo "expiration_warning:no"
        fi
    else
        if [[ $end_ts -le $warning_ts ]]; then
            colorize "$YELLOW" "Expiration Warning: Certificate will expire within $warning_days days!"
        else
            colorize "$GREEN" "Expiration Warning: Certificate is not close to expiration."
        fi
    fi
}

# Function to display usage information
show_usage() {
    echo "Usage: $0 [--check-fips] [--no-color] [--summary] <certificate.pem> or pipe certificate content."
    echo "Options:"
    echo "  --check-fips   Enable FIPS validation."
    echo "  --no-color     Disable color output."
    echo "  --summary      Produce summarized output suitable for parsing."
    exit 1
}

# Parse command-line arguments
while [[ "$#" -gt 0 ]]; do
    case "$1" in
        --check-fips)
            FIPS_CHECK=true
            ;;
        --no-color)
            COLOR=false
            ;;
        --summary)
            SUMMARY=true
            ;;
        -*)
            echo "Unknown option: $1"
            show_usage
            ;;
        *)
            CERT_FILE="$1"
            ;;
    esac
    shift
done

# Ensure certificate input is provided
if [[ -z "$CERT_FILE" && -t 0 ]]; then
    show_usage
fi

# Determine input source (file or pipe)
if [[ -t 0 ]]; then
    # Input is a file
    if [[ ! -f "$CERT_FILE" ]]; then
        echo "Error: Certificate file '$CERT_FILE' not found."
        exit 1
    fi
    CERT_CONTENT=$(cat "$CERT_FILE")
else
    # Input is from a pipe
    CERT_CONTENT=$(cat)
fi

if [[ "$SUMMARY" != "true" ]]; then
    colorize "$CYAN" "Checking certificate..."
    echo "---------------------------------"
fi

check_ca_usage
check_validity
check_algorithm
check_fips_approval
check_expiration_warning

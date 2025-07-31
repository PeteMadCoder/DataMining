#!/bin/bash

# --- get_cpu_info.sh ---

# Check if lscpu is available
if ! command -v lscpu &> /dev/null; then
    echo "Error: 'lscpu' command not found. This script requires lscpu."
    exit 1
fi

# Get CPU information using lscpu and parse with grep and awk
# We are interested in Cores per socket and Sockets, or just Cores if available directly
# lscpu | grep -E '^Thread|^Core|^Socket|^CPU\(' is the command you found

echo "Detecting CPU configuration..."
CPU_INFO=$(lscpu)

# Try to get Cores per socket and number of sockets first
CORES_PER_SOCKET=$(echo "$CPU_INFO" | grep -E '^Core\(s\) per socket:' | awk '{print $4}')
SOCKETS=$(echo "$CPU_INFO" | grep -E '^Socket\(s\):' | awk '{print $2}')

# Try to get total Cores directly (might be present)
TOTAL_CORES=$(echo "$CPU_INFO" | grep -E '^CPU\(s\):' | awk '{print $2}')

# Handle potential non-numeric values (e.g., "8" or "8 (4 online)")
# Extract the first number found
if [[ $TOTAL_CORES =~ ^([0-9]+) ]]; then
    TOTAL_CORES=${BASH_REMATCH[1]}
fi
if [[ $CORES_PER_SOCKET =~ ^([0-9]+) ]]; then
    CORES_PER_SOCKET=${BASH_REMATCH[1]}
fi
if [[ $SOCKETS =~ ^([0-9]+) ]]; then
    SOCKETS=${BASH_REMATCH[1]}
fi


# Determine the number of logical CPUs (Threads)
LOGICAL_CPUS=$(echo "$CPU_INFO" | grep -E '^CPU\(s\):' | awk '{print $2}')
if [[ $LOGICAL_CPUS =~ ^([0-9]+) ]]; then
    LOGICAL_CPUS=${BASH_REMATCH[1]}
fi

# Calculate physical cores if we have cores per socket and sockets
PHYSICAL_CORES=""
if [[ -n "$CORES_PER_SOCKET" && -n "$SOCKETS" ]]; then
    PHYSICAL_CORES=$(( CORES_PER_SOCKET * SOCKETS ))
fi

# Decide which core count to use for recommendation (prefer physical if available)
RECOMMENDED_CORES=""
if [[ -n "$PHYSICAL_CORES" ]]; then
    RECOMMENDED_CORES=$PHYSICAL_CORES
    echo "Found $PHYSICAL_CORES physical CPU core(s) ($CORES_PER_SOCKET core(s) per socket * $SOCKETS socket(s))."
elif [[ -n "$TOTAL_CORES" ]]; then
    RECOMMENDED_CORES=$TOTAL_CORES
    echo "Found $TOTAL_CORES total CPU core(s) (from CPU(s) line)."
else
    # Fallback if specific core info is hard to parse
    echo "Could not determine physical cores precisely."
    if [[ -n "$LOGICAL_CPUS" ]]; then
        RECOMMENDED_CORES=$LOGICAL_CPUS
        echo "Falling back to $LOGICAL_CPUS logical CPU(s) (threads) for estimation."
    else
        echo "Error: Could not determine CPU count from lscpu output."
        exit 1
    fi
fi

# Display Logical CPUs (Threads) if different from cores
if [[ -n "$LOGICAL_CPUS" && "$LOGICAL_CPUS" != "$RECOMMENDED_CORES" ]]; then
    echo "Found $LOGICAL_CPUS logical CPU(s) (threads) in total."
fi

# --- Recommendation Logic ---
# For CPU-bound tasks like HTML processing, physical cores are often a good starting point.
# Using all logical CPUs (including hyperthreads) might lead to context switching overhead.
# A common recommendation is the number of physical cores, or sometimes physical cores + a small buffer (e.g., +1 or +2).
# Let's recommend physical cores, but cap the suggestion if it's very high.

RECOMMENDED_THREADS=$RECOMMENDED_CORES

# Simple cap for very high core counts (optional, adjust as needed)
# if [ "$RECOMMENDED_THREADS" -gt 16 ]; then
#     RECOMMENDED_THREADS=16
#     echo "Note: High core count detected. Capping recommendation at $RECOMMENDED_THREADS for manageability."
# fi

echo ""
echo "--- Thread Recommendation for DataMiner Processing ---"
echo "For CPU-bound processing tasks (like HTML parsing),"
echo "a good starting point is often the number of physical CPU cores."
echo ""
echo "Recommended number of processing threads: $RECOMMENDED_THREADS"
echo ""
echo "This is a conservative baseline, meaning that it's possible to be more eficient"
echo "with more threads than those recommended"
echo ""
echo "You can use this with DataMiner like so:"
echo "./DataMiner --process ./output --processor-type wikipedia --processing-threads $RECOMMENDED_THREADS ..."
echo "--------------------------------------------------------"

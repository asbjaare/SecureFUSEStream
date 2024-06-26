#!/bin/bash
# Define source and destination directories
SRC="/home/aaa040/SecureFUSEStream/bench/"
DST="/home/aaa040/SecureFUSEStream/fuse/mountpoint/"

# Get a list of images
IMAGES=$(ls $SRC)

# Benchmark start time
START=$(date +%s)

# Copy images
for img in $IMAGES; do
	cp "${SRC}/${img}" "${DST}/"
	echo "Sleeping for 2 sec"
	sleep 5
done

# Benchmark end time
END=$(date +%s)

# Calculate and display the time taken
echo "Time taken: $((END - START)) seconds"

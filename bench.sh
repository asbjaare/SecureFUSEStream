#!/bin/bash
# Define source and destination directories
SRC="/path/to/source/images"
DST="/path/to/mounted/fuse/fs"

# Get a list of images
IMAGES=$(ls $SRC)

# Benchmark start time
START=$(date +%s)

# Copy images
for img in $IMAGES; do
	cp "${SRC}/${img}" "${DST}/"
done

# Benchmark end time
END=$(date +%s)

# Calculate and display the time taken
echo "Time taken: $((END - START)) seconds"

#!/bin/bash

# Check for correct usage
if [ "$#" -ne 2 ]; then
	echo "Usage: $0 <source image path> <destination directory>"
	exit 1
fi

# Assign arguments to variables
source_image=$1
destination_dir=$2

# Check if the source image exists
if [ ! -f "$source_image" ]; then
	echo "Error: Source image does not exist."
	exit 1
fi

# Check if destination directory exists, if not, create it
if [ ! -d "$destination_dir" ]; then
	echo "Destination directory does not exist, creating it..."
	mkdir -p "$destination_dir"
fi

# Loop to create copies
for i in $(seq 1 1000); do
	cp "$source_image" "${destination_dir}/image_copy_$i.bmp"
done

echo "1000 copies of '$source_image' have been created in '$destination_dir'."

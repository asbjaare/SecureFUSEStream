#!/bin/bash
# Copy TA and CA into jetson remote via ssh

if [ $# -eq 0 ]; then
	echo "Usage: $0 <username>"
	exit 1
fi

# Copy binaries to remote
scp ta/236268e6-a7a4-4bcd-97c6-451ebd802beb.ta $1@jetagx1.cs.uit.no:
scp host/video_tee $1@jetagx1.cs.uit.no:

# Move binaries to correct locations (TODO: Needs sudo)
# ssh oty000@jetagx1.cs.uit.no

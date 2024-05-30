#!/bin/bash
# Copy TA and CA into jetson remote via ssh

if [ $# -eq 0 ]; then
	echo "Usage: $0 <username>"
	exit 1
fi

# Copy binaries to remote
scp ta/f4e750bb-1437-4fbf-8785-8d3580c34994.ta $1@jetagx1.cs.uit.no:
scp host/optee_example_secure_storage $1@jetagx1.cs.uit.no:

# Move binaries to correct locations (TODO: Needs sudo)
# ssh oty000@jetagx1.cs.uit.no

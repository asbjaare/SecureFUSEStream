#!/bin/bash
# Copy TA and CA into jetson remote via ssh

# Copy binaries to remote
scp ta/236268e6-a7a4-4bcd-97c6-451ebd802beb.ta oty000@jetagx1.cs.uit.no:
scp host/video_tee oty000@jetagx1.cs.uit.no:

# Move binaries to correct locations (TODO: Needs sudo)
# ssh oty000@jetagx1.cs.uit.no

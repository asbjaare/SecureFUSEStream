#!/bin/bash

sudo fusermount -u mountpoint

cargo build --release

rm fuse

mv target/release/fuse .

rm -rf mnt && mkdir mnt
rm -rf mountpoint && mkdir mountpoint

sudo ./fuse mnt/ mountpoint/

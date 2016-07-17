#!/usr/bin/env bash
#
# This is intended only for use by the siteupdate.sh script
# and can run concurrently with parts of siteupdate.sh
#
echo "xfernmpwpts.sh: Creating nmpwptstoxfer.tar"
tar cf nmpwptstoxfer.tar $1
echo "xfernmpwpts.sh: Bzipping nmpwptstoxfer.tar"
bzip2 -9f nmpwptstoxfer.tar
echo "xfernmpwpts.sh: Transfering nmpwptstoxfer.tar.bz2"
scp nmpwptstoxfer.tar.bz2 blizzard.teresco.org:/tmp
echo "xfernmpwpts.sh: Launching command to bunzip and extract nmpwptstoxfer.tar.bz2 on blizzard"
ssh blizzard.teresco.org "cd /home/www/tm; bzcat /tmp/nmpwptstoxfer.tar.bz2 | tar xpf -"
echo "xfernmpwpts.sh: complete"

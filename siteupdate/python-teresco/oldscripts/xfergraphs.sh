#!/usr/bin/env bash
#
# This is intended only for use by the siteupdate.sh script
# and can run concurrently with parts of siteupdate.sh
#
echo "xfergraphs.sh: Creating graphstoxfer.tar"
tar cf graphstoxfer.tar $1/*.tmg $1/*.nmp
echo "xfergraphs.sh: Bzipping graphstoxfer.tar"
bzip2 -9f graphstoxfer.tar
echo "xfergraphs.sh: Transfering graphstoxfer.tar.bz2"
scp graphstoxfer.tar.bz2 blizzard.teresco.org:/tmp
echo "xfergraphs.sh: Launching command to bunzip and extract graphstoxfer.tar.bz2 on blizzard"
ssh blizzard.teresco.org "cd /home/www/tm; bzcat /tmp/graphstoxfer.tar.bz2 | tar xpf -"
echo "xfergraphs.sh: Launching command to create zip graph archive on blizzard"
ssh blizzard.teresco.org "cd /home/www/tm; zip -q graphs.zip graphs/*.tmg"
echo "xfergraphs.sh: complete"

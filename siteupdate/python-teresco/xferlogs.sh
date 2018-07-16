#!/usr/bin/env bash
#
# This is intended only for use by the siteupdate.sh script
# and can run concurrently with parts of siteupdate.sh
#
echo "xferlogs.sh: Creating logstoxfer.tar"
tar cf logstoxfer.tar $1/*.log $1/users/*.log $2/*.csv $3/*.tmg $1/*.nmp
echo "xferlogs.sh: Bzipping logstoxfer.tar"
bzip2 -9f logstoxfer.tar
echo "xferlogs.sh: Transfering logstoxfer.tar.bz2"
scp logstoxfer.tar.bz2 blizzard.teresco.org:/tmp
echo "xferlogs.sh: Launching command to bunzip and extract logstoxfer.tar.bz2 on blizzard"
ssh blizzard.teresco.org "cd /home/www/tm; /bin/rm -rf ${1} ${2}; bzcat /tmp/logstoxfer.tar.bz2 | tar xpf -"
echo "xferlogs.sh: Launching command to create zip graph archive on blizzard"
ssh blizzard.teresco.org "cd /home/www/tm; zip -q graphs.zip ${3}/*.tmg"
echo "xferlogs.sh: complete"

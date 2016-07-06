#!/usr/bin/env bash
#
# This is intended only for use by the siteupdate.sh script
# and can run concurrently with parts of siteupdate.sh
#
echo "xferlogs.sh: Creating logstoxfer.tar"
tar cf logstoxfer.tar $1/*.log $2/*.csv $3/*.{gra,tmg,php}
echo "xferlogs.sh: Bzipping logstoxfer.tar"
bzip2 -9f logstoxfer.tar
echo "xferlogs.sh: Transfering logstoxfer.tar.bz2"
scp logstoxfer.tar.bz2 blizzard.teresco.org:/tmp
echo "xferlogs.sh: Launching command to bunzip and extract logstoxfer.tar.bz2 on blizzard"
ssh blizzard.teresco.org "cd /home/www/tm; bzcat /tmp/logstoxfer.tar.bz2 | tar xpf -"
echo "xferlogs.sh: complete"

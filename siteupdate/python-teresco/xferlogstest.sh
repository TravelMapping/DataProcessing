#!/usr/bin/env bash
#
# This is intended only for use by the siteupdate.sh script
# and can run concurrently with parts of siteupdate.sh
#
echo "xferlogstest.sh: Creating logstoxfer.tar"
tar cf logstoxfer.tar $1/*.log $2/*.csv $3/*.tmg $3/*nmp
echo "xferlogstest.sh: Bzipping logstoxfer.tar"
bzip2 -9f logstoxfer.tar
echo "xferlogstest.sh: Transfering logstoxfer.tar.bz2"
scp logstoxfer.tar.bz2 blizzard.teresco.org:/tmp
echo "xferlogstest.sh: Launching command to bunzip and extract logstoxfer.tar.bz2 on blizzard"
ssh blizzard.teresco.org "cd /home/www/tmtest; bzcat /tmp/logstoxfer.tar.bz2 | tar xpf -"
echo "xferlogstest.sh: complete"

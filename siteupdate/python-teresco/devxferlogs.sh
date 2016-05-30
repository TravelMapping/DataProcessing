#!/usr/bin/env bash
#
# This is intended only for use by the devsiteupdate.sh script
# and can run concurrently with parts of devsiteupdate.sh
#
echo "devxferlogs.sh: Creating logstodevxfer.tar"
tar cf logstodevxfer.tar $1/*.log $2/*.csv
echo "devxferlogs.sh: Bzipping logstodevxfer.tar"
bzip2 -9f logstodevxfer.tar
echo "devxferlogs.sh: Transfering logstodevxfer.tar.bz2"
scp logstodevxfer.tar.bz2 blizzard.teresco.org:/tmp
echo "devxferlogs.sh: Launching command to bunzip and extract logstodevxfer.tar.bz2 on blizzard"
ssh blizzard.teresco.org "cd /home/www/tm; bzcat /tmp/logstodevxfer.tar.bz2 | tar xpf -"
echo "devxferlogs.sh: complete"

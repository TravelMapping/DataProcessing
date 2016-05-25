#!/usr/bin/env bash
#
set -e
read_data=1
date
if [ $# -eq 1 ]; then
  if [ "$1" == "--noread" ]; then
     read_data=0
  fi
fi
if [ "$read_data" == "1" ]; then
  echo "siteupdate.sh: launching read_data.py"
  ./read_data.py | tee read_data.log 2>&1
else
  echo "siteupdate.sh: SKIPPING read_data.py"
fi
echo "siteupdate.sh: Bzipping TravelMapping.sql file"
bzip2 -9f TravelMapping.sql
echo "siteupdate.sh: Transferring TravelMapping.sql.bz2 to blizzard"
scp TravelMapping.sql.bz2 blizzard.teresco.org:/tmp
echo "siteupdate.sh: launching xferlogs.sh"
sh xferlogs.sh &
echo "siteupdate.sh: sending bunzip to run on blizzard"
ssh blizzard.teresco.org bunzip2 -f /tmp/TravelMapping.sql.bz2
echo "siteupdate.sh: sending mysql update to run on blizzard"
ssh blizzard.teresco.org "mysql --defaults-group-suffix=tmapadmin -u travmapadmin TravelMapping < /tmp/TravelMapping.sql"
echo "siteupdate.sh: complete"
echo "siteupdate.sh: sending email notification"
mailx -s "Travel Mapping Site Update Complete" travelmapping-siteupdates@teresco.org <<EOF
A Travel Mapping site update has just successfully completed.
The complete log is available at http://tm.teresco.org/logs/read_data.log .
Please report any problems to travmap@teresco.org .
EOF
date

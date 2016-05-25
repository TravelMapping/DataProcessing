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
  echo "devsiteupdate.sh: launching read_data.py"
  ./read_data.py -d TravelMappingDev -i active,preview
else
  echo "devsiteupdate.sh: SKIPPING read_data.py"
fi
echo "devsiteupdate.sh: Bzipping TravelMappingDev.sql file"
bzip2 -9f TravelMappingDev.sql
echo "devsiteupdate.sh: Transferring TravelMappingDev.sql.bz2 to blizzard"
scp TravelMappingDev.sql.bz2 blizzard.teresco.org:/tmp
echo "devsiteupdate.sh: launching devxferlogs.sh"
sh devxferlogs.sh &
echo "devsiteupdate.sh: sending bunzip to run on blizzard"
ssh blizzard.teresco.org bunzip2 -f /tmp/TravelMappingDev.sql.bz2
echo "devsiteupdate.sh: sending mysql update to run on blizzard"
ssh blizzard.teresco.org "mysql --defaults-group-suffix=tmapadmin -u travmapadmin TravelMappingDev < /tmp/TravelMappingDev.sql"
echo "devsiteupdate.sh: complete"
echo "devsiteupdate.sh: sending email notification"
mailx -s "Travel Mapping Site Update (with preview systems) Complete" travelmapping-siteupdates@teresco.org <<EOF
A Travel Mapping site update that includes the preview systems
has just successfully completed.
The complete log is available at http://tm.teresco.org/devlogs/read_data.log .
Please report any problems to travmap@teresco.org .
EOF
date

#!/usr/bin/env bash
#
set -e
read_data=1
logdir=devlogs
statdir=devstats
mkdir -p $logdir $statdir
date
if [ $# -eq 1 ]; then
  if [ "$1" == "--noread" ]; then
     read_data=0
  fi
fi
if [ "$read_data" == "1" ]; then
  echo "devsiteupdate.sh: launching siteupdate.py"
  ./siteupdate.py -s cypa-only.csv -k -d TravelMappingDev -i active,preview -l $logdir -c $statdir | tee $logdir/devsiteupdate.log 2>&1
else
  echo "devsiteupdate.sh: SKIPPING siteupdate.py"
fi
echo "devsiteupdate.sh: Bzipping TravelMappingDev.sql file"
bzip2 -9f TravelMappingDev.sql
echo "devsiteupdate.sh: Transferring TravelMappingDev.sql.bz2 to blizzard"
scp TravelMappingDev.sql.bz2 blizzard.teresco.org:/tmp
echo "devsiteupdate.sh: launching devxferlogs.sh"
sh devxferlogs.sh $logdir $statdir &
echo "devsiteupdate.sh: sending bunzip to run on blizzard"
ssh blizzard.teresco.org bunzip2 -f /tmp/TravelMappingDev.sql.bz2
echo "devsiteupdate.sh: sending mysql update to run on blizzard"
ssh blizzard.teresco.org "mysql --defaults-group-suffix=tmapadmin -u travmapadmin TravelMappingDev < /tmp/TravelMappingDev.sql"
echo "devsiteupdate.sh: complete"
#echo "devsiteupdate.sh: sending email notification"
#mailx -s "Travel Mapping Site Update (with preview systems) Complete" travelmapping-siteupdates@teresco.org <<EOF
#A Travel Mapping site update that includes the preview systems
#has just successfully completed.
#The complete log is available at http://tm.teresco.org/devlogs/siteupdate.log .
#Please report any problems to travmap@teresco.org .
#EOF
date

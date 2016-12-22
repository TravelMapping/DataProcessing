#!/usr/bin/env bash
#
set -e
read_data=1
logdir=testlogs
statdir=teststats
graphdir=testgraphs
nmpmerged=nmp_merged
mkdir -p $logdir $statdir $graphdir
date
if [ $# -eq 1 ]; then
  if [ "$1" == "--noread" ]; then
     read_data=0
  fi
fi
if [ "$read_data" == "1" ]; then
  echo "testsiteupdate.sh: launching siteupdate.py"
  #./siteupdate.py -w ../../../../HighwayData -d TravelMappingTest -s smallsystems.csv -u a_few_lists -l $logdir -c $statdir | tee $logdir/testsiteupdate.log 2>&1
  #./siteupdate.py -k -w ../../../../HighwayData -s systems.csv -d TravelMappingTest -g $graphdir -u . -l $logdir -c $statdir | tee $logdir/testsiteupdate.log 2>&1
  ./siteupdate.py -k -s cypa-only.csv -d TravelMappingTest -g $graphdir -l $logdir -c $statdir | tee $logdir/testsiteupdate.log 2>&1
  #./siteupdate.py -w ../../../../HighwayData -d TravelMappingTest -u ../../../../UserData/list_files -l $logdir -c $statdir | tee $logdir/testsiteupdate.log 2>&1
else
  echo "testsiteupdate.sh: SKIPPING siteupdate.py"
fi
echo "testsiteupdate.sh: Bzipping TravelMappingTest.sql file"
bzip2 -9f TravelMappingTest.sql
echo "testsiteupdate.sh: Transferring TravelMappingTest.sql.bz2 to blizzard"
scp TravelMappingTest.sql.bz2 blizzard.teresco.org:/tmp
echo "testsiteupdate.sh: launching xferlogs.sh"
sh xferlogstest.sh $logdir $statdir $graphdir &
echo "testsiteupdate.sh: sending bunzip to run on blizzard"
ssh blizzard.teresco.org bunzip2 -f /tmp/TravelMappingTest.sql.bz2
echo "testsiteupdate.sh: sending mysql update to run on blizzard"
ssh blizzard.teresco.org "mysql --defaults-group-suffix=tmapadmin -u travmapadmin TravelMappingTest < /tmp/TravelMappingTest.sql"
echo "testsiteupdate.sh: complete"
#echo "siteupdate.sh: sending email notification"
#mailx -s "Travel Mapping Site Update Complete" travelmapping-siteupdates@teresco.org <<EOF
#A Travel Mapping site update has just successfully completed.
#The complete log is available at http://tm.teresco.org/logs/siteupdate.log .
#Please report any problems to travmap@teresco.org .
#EOF
date

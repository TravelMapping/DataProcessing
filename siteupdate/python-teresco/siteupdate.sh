#!/usr/bin/env bash
#
set -e
read_data=1
transfer=1
logdir=logs
statdir=stats
graphdir=graphdata
nmpmerged=nmp_merged
graphflag=
mkdir -p $logdir/users $statdir $graphdir
date
if [ $# -eq 1 ]; then
  if [ "$1" == "--noread" ]; then
     read_data=0
  fi
  if [ "$1" == "--nographs" ]; then
      graphflag="-k"
  fi
  if [ "$1" == "--noxfer" ]; then
      transfer=0
  fi
fi
if [ "$read_data" == "1" ]; then
  echo "siteupdate.sh: launching siteupdate.py"
  echo "siteupdate.sh: cleaning $logdir and $statdir"
  /bin/rm -f $logdir/*.log $statdir/*.csv  
  # Add -k to prevent generation of new version of graphs
  # Remove -k to generate new version of graphs
  PYTHONIOENCODING='utf-8' ./siteupdate.py $graphflag -l $logdir -c $statdir -g $graphdir -n $nmpmerged | tee $logdir/siteupdate.log 2>&1 || exit 1
else
  echo "siteupdate.sh: SKIPPING siteupdate.py"
fi
date
if [ "$transfer" == "0" ]; then
    echo "siteupdate.sh: SKIPPING file transfers and DB update"
    exit 0
fi
echo "siteupdate.sh: Bzipping TravelMapping.sql file"
bzip2 -9f TravelMapping.sql
echo "siteupdate.sh: Transferring TravelMapping.sql.bz2 to blizzard"
scp TravelMapping.sql.bz2 blizzard.teresco.org:/tmp
echo "siteupdate.sh: launching xferlogs.sh"
sh xferlogs.sh $logdir $statdir $graphdir &
echo "siteupdate.sh: launching xfernmpwpts.sh"
sh xfernmpwpts.sh $nmpmerged &
echo "siteupdate.sh: sending bunzip to run on blizzard"
ssh blizzard.teresco.org bunzip2 -f /tmp/TravelMapping.sql.bz2
echo "siteupdate.sh: sending mysql update to run on blizzard"
date
ssh blizzard.teresco.org "touch /home/www/tm/dbupdating; mysql --defaults-group-suffix=tmapadmin -u travmapadmin TravelMapping < /tmp/TravelMapping.sql; /bin/rm /home/www/tm/dbupdating"
echo "siteupdate.sh: complete"
echo "siteupdate.sh: sending email notification"
mailx -s "Travel Mapping Site Update Complete" travelmapping-siteupdates@teresco.org <<EOF
A Travel Mapping site update has just successfully completed.
The complete log is available at http://tm.teresco.org/logs/siteupdate.log .
Please report any problems to travmap@teresco.org .
EOF
date

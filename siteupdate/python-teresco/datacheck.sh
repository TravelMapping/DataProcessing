#!/usr/bin/env bash
#
# script to run check-only site update on local data
#
# Jim Teresco, Sun May 20 16:27:18 EDT 2018
#
set -e
install=1
pull=1
if [ -d "$HOME/HighwayData" ]; then
    tmbase=$HOME
elif [ -d "$HOME/travelmapping/HighwayData" ]; then
    tmbase=$HOME/travelmapping
elif [ -d "$HOME/TravelMapping/HighwayData" ]; then
    tmbase=$HOME/TravelMapping
elif [ -d "$HOME/tm/HighwayData" ]; then
    tmbase=$HOME/tm
else
    echo "$0: could not find HighwayData repository"
    exit 1
fi
logdir=logs
statdir=stats
date
# process command line args
for arg in "$@"; do
    if [ "$arg" == "--nopull" ]; then
	pull=0
    fi
    shift
done
if [ "$pull" == "1" ]; then
      echo "$0: updating TM repositories"
      (cd $tmbase/HighwayData; git pull)
      (cd $tmbase/UserData; git pull)
fi

echo "$0: creating directories"
mkdir -p $logdir/users $statdir

echo "$0: launching siteupdate.py"
PYTHONIOENCODING='utf-8' ./siteupdate.py -e -l $logdir -c $statdir | tee $logdir/siteupdate.log 2>&1 || exit 1
date
echo "$0: complete"

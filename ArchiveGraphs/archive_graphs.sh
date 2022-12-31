#!/usr/bin/env bash
#
# script to run to take the current set of METAL graphs and
# save them as an archive
#
# Jim Teresco, Tue Jun 28 18:25:28 EDT 2022
#
set -e
install=1
pull=1
execdir=`pwd`
tmbase=$HOME/travelmapping
tmwebbase=/fast/www/tm
datestr=`date '+%Y-%m-%d'`
graphdir=$HOME/temp/graphdata
graphdir=$tmwebbase/graphdata
# check for required command line args:
#
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 archivename description"
    exit 1
fi
export HIGHWAY_DATA=$tmbase/HighwayData
echo "Creating graph archive set with date stamp $datestr"
echo "Archive name $1, description: $2"
archivedir=$tmwebbase/grapharchives/$1
if [ -d $archivedir ]; then
    echo "Archive directory $archivedir exists, exiting"
    exit 1
fi
echo "Creating archive directory $archivedir"
mkdir $archivedir
echo "Copying existing graphs from $graphdir to $archivedir"
cp $graphdir/*.tmg $archivedir

echo "Gathering repo head info"
hwydatavers=`(cd $tmbase/HighwayData; git show -s | head -n 1 | cut -f2 -d' ')`
userdatavers=`(cd $tmbase/UserData; git show -s | head -n 1 | cut -f2 -d' ')`
dataprocvers=`git show -s | head -n 1 | cut -f2 -d' '`
echo "Highway Data: $hwydatavers"
echo "User Data: $userdatavers"
echo "Data Processing: $dataprocvers"

echo "running archivegraphs to create intersection-only graphs and compute stats."
echo "Command line is:"
echo "./archivegraphs $1 \"$2\" $datestr $hwydatavers $userdatavers $dataprocvers $archivedir"
./archivegraphs $1 "\"$2\"" $datestr $hwydatavers $userdatavers $dataprocvers $archivedir
echo "Adding archive entries to DB"
mysql --defaults-group-suffix=tmapadmin -u travmapadmin TravelMapping < $archivedir/$1.sql
mysql --defaults-group-suffix=tmapadmin -u travmapadmin TravelMappingCopy < $archivedir/$1.sql

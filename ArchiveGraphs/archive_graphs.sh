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
datestr=`date '+%Y-%m-%d'`
# check for required command line args:
#
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 archivename description"
    exit 1
fi

# check default locations of TM repositories
tmbasedir=$HOME
if [[ -d $HOME/TravelMapping ]]; then
    tmbasedir=$HOME/TravelMapping
elif [[ -d $HOME/travelmapping ]]; then
    tmbasedir=$HOME/travelmapping
elif [[ -d $HOME/tm ]]; then
    tmbasedir=$HOME/tm
fi

# check default locations of TM web server files
tmwebdir=/fast/www/tm
if [[ -d /var/www/html ]]; then
    tmwebdir=/var/www/html
elif [[ -d /opt/local/www/apache2/html ]]; then
    tmwebdir=/opt/local/www/apache2/html
fi

# make sure we have a valid location for tmbasedir and it has the two
# needed repositories, and that they appear to be valid repositories
if [[ ! -d $tmbasedir ]]; then
    echo "TravelMapping repository base directory $tmbasedir not found"
    exit 1
else
    if [[ ! -d $tmbasedir/HighwayData ]]; then
	echo "TravelMapping/HighwayData repository not found in $tmbasedir"
	exit 1
    elif [[ ! -d $tmbasedir/HighwayData/.git ]]; then
	echo "TravelMapping/HighwayData in $tmbasedir does not appear to be a git repository"
	exit 1
    fi
    if [[ ! -d $tmbasedir/UserData ]]; then
	echo "TravelMapping/UserData repository not found in $tmbasedir"
	exit 1
    elif [[ ! -d $tmbasedir/UserData/.git ]]; then
	echo "TravelMapping/UserData in $tmbasedir does not appear to be a git repository"
	exit 1
    fi	
fi

if [[ ! -d $tmwebdir ]]; then
    echo "Web directory $tmwebdir does not exist"
    exit 1
fi
if [[ ! -f $tmwebdir/participate.php ]]; then
    echo "Web directory $tmwebdir does not appear to contain a TM web instance"
    exit 1
fi

graphdir=$tmwebdir/graphdata

export HIGHWAY_DATA=$tmbasedir/HighwayData
echo "Creating graph archive set with date stamp $datestr"
echo "Archive name $1, description: $2"
archivedir=$tmwebdir/grapharchives/$1
if [ -d $archivedir ]; then
    echo "Archive directory $archivedir exists, exiting"
    exit 1
fi
echo "Creating archive directory $archivedir"
mkdir $archivedir
echo "Copying existing graphs from $graphdir to $archivedir"
cp $graphdir/*.tmg $archivedir

echo "Gathering repo head info"
hwydatavers=`(cd $tmbasedir/HighwayData; git show -s | head -n 1 | cut -f2 -d' ')`
userdatavers=`(cd $tmbasedir/UserData; git show -s | head -n 1 | cut -f2 -d' ')`
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

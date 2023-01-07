#!/usr/bin/env bash
#
# TM common site update script
#
# Based on language, OS, and other specific scripts
#
# Jim Teresco, Sat Dec 31 22:59:48 EST 2022
#

function usage {
    echo "Usage: $0 [--help] [--nographs] [--noinstall] [--nopull] [--use-python] [--remote server] [--numthreads nt]"
}

set -e
date

# lots of defaults
install=1
pull=1
execdir=`pwd`
tmbase=$HOME/travelmapping
tmwebbase=/fast/www/tm
tmpdir=/home/tmp/tm
datestr=`date '+%Y-%m-%d@%H:%M:%S'`
logdir=logs
statdir=stats
graphdir=graphdata
grapharchives=grapharchives
nmpmdir=nmp_merged
graphflag=
language=cpp
remote=0
numthreads=1

# process command line args
nextitem=
for arg in "$@"; do
    if [[ "$nextitem" == remote ]]; then
	nextitem=
	remote=$arg
    elif [[ "$nextitem" == numthreads ]]; then
	nextitem=
	numthreads=$arg
    elif [[ "$arg" == --help ]]; then
	usage
	exit 0
    elif [[ "$arg" == --nographs ]]; then
	# -k to siteupdate supresses graph generation
	graphflag="-k"
    elif [[ "$arg" == --noinstall ]]; then
	install=0
    elif [[ "$arg" == --nopull ]]; then
	pull=0
    elif [[ "$arg" == --use-python ]]; then
	language=python
    elif [[ "$arg" == --remote ]]; then
	nextitem=remote
    elif [[ "$arg" == --numthreads ]]; then
	nextitem=numthreads
    else
	echo "Unknown argument $arg"
	usage
	exit 1
    fi
    shift
done

# check for any missing nextitem
if [[ "$nextitem" != "" ]]; then
    echo "Missing argument for $nextitem"
    usage
    exit 1
fi

# no threads with python allowed
if [[ "$language" == python && "$numthreads" != "1" ]]; then
    echo "Python is not compatible with multiple threads"
    exit 1
fi

# no remote support, yet
if [[ "$remote" != "0" ]]; then
    echo "Remote updates not yet supported"
    exit 1
fi

# check for valid language option
if [[ "$language" != python && "$language" != cpp ]]; then
    echo "Language $language not supported, only python and cpp"
    exit 1
fi

if [[ "$pull" == "1" ]]; then
      echo "$0: updating TM repositories"
      (cd $tmbase/HighwayData; git pull)
      (cd $tmbase/UserData; git pull)
fi

echo "$0: creating directories"
mkdir -p $datestr/$logdir/users $datestr/$statdir $datestr/$nmpmdir $datestr/$logdir/nmpbyregion
if [[ "$graphflag" != "-k" ]]; then
    mkdir -p $datestr/$graphdir
fi

echo "$0: gathering repo head info"
echo HighwayData '@' `(cd $tmbase/HighwayData; git show -s | head -n 1 | cut -f2 -d' ')` | tee $datestr/$logdir/siteupdate.log
echo UserData '@' `(cd $tmbase/UserData; git show -s | head -n 1 | cut -f2 -d' ')` | tee -a $datestr/$logdir/siteupdate.log
echo DataProcessing '@' `git show -s | head -n 1 | cut -f2 -d' '` | tee -a $datestr/$logdir/siteupdate.log

echo "$0: creating listupdates.txt"
cd $tmbase/UserData/list_files
for u in *; do echo $u `git log -n 1 --pretty=%ci $u`; done | tee $execdir/listupdates.txt
cd -

# pick our language, number of threads for the command
if [[ "$language" == python ]]; then
    if hash python3 2>/dev/null; then
	siteupdate="python3 python-teresco/siteupdate.py"
    else
	echo "No python3 command found."
	exit 1
    fi
elif [[ "$numthreads" != "1" ]]; then
    if [ -x ./cplusplus/siteupdate ]; then
	siteupdate="./cplusplus/siteupdate -t $numthreads"
    else
	echo "./cplusplus/siteupdate program not found."
	exit 1
    fi
else
    if [ -x ./cplusplus/siteupdateST ]; then
	siteupdate="./cplusplus/siteupdateST"
    else
	echo "./cplusplus/siteupdateST program not found."
	exit 1
    fi
fi
   
echo "$0: launching $siteupdate"
$siteupdate -d TravelMapping-$datestr $graphflag -l $datestr/$logdir -c $datestr/$statdir -g $datestr/$graphdir -n $datestr/$nmpmdir | tee -a $datestr/$logdir/siteupdate.log 2>&1 || exit 1
date

echo "$0: deleting listupdates.txt"
rm $execdir/listupdates.txt

if [ -x ../../nmpfilter/nmpbyregion ]; then
    echo "$0: running nmpbyregion"
    ../../nmpfilter/nmpbyregion $datestr/$logdir/tm-master.nmp $datestr/$logdir/nmpbyregion/
else
    echo "$0: SKIPPING nmpbyregion (../../nmpfilter/nmpbyregion not executable)"
fi

if [ "$install" == "0" ]; then
    echo "$0: SKIPPING file copies and DB update"
    exit 0
fi

if [ "$graphflag" != "-k" ]; then
    echo "$0: creating zip archive of all graphs"
    cd $datestr/$graphdir
    zip -q graphs.zip *.tmg
    cd -
fi

echo "$0: switching to DB copy"
ln -sf $tmwebbase/lib/tm.conf.updating $tmwebbase/lib/tm.conf
touch $tmwebbase/dbupdating

echo "$0: loading primary DB"
date
mysql --defaults-group-suffix=tmapadmin -u travmapadmin TravelMapping < TravelMapping-$datestr.sql
if [ -d $tmwebbase/$grapharchives ]; then
    for archive in $tmwebbase/$grapharchives/*/*.sql; do
	mysql --defaults-group-suffix=tmapadmin -u travmapadmin TravelMapping < $archive
    done
fi
/bin/rm $tmwebbase/dbupdating
echo "$0: switching to primary DB"
date
ln -sf $tmwebbase/lib/tm.conf.standard $tmwebbase/lib/tm.conf

echo "$0: installing logs, stats, nmp_merged, graphs, archiving old contents in $tmpdir/$datestr"
mkdir -p $tmpdir/$datestr
mv $tmwebbase/$logdir $tmpdir/$datestr
mv $datestr/$logdir $tmwebbase
mv $tmwebbase/$statdir $tmpdir/$datestr
mv $datestr/$statdir $tmwebbase
mv $tmwebbase/$nmpmdir $tmpdir/$datestr
mv $datestr/$nmpmdir $tmwebbase
if [ "$graphflag" != "-k" ]; then
    mv $tmwebbase/$graphdir $tmpdir/$datestr
    mv $datestr/$graphdir $tmwebbase
fi
rmdir $datestr

echo "$0: loading DB copy"
mysql --defaults-group-suffix=tmapadmin -u travmapadmin TravelMappingCopy < TravelMapping-$datestr.sql
if [ -d $tmwebbase/$grapharchives ]; then
    for archive in $tmwebbase/$grapharchives/*/*.sql; do
	mysql --defaults-group-suffix=tmapadmin -u travmapadmin TravelMappingCopy < $archive
    done
fi
echo "$0: moving sql file to archive"
mv TravelMapping-$datestr.sql $tmpdir
echo "$0: sending email notification"
mailx -s "Travel Mapping Site Update Complete" travelmapping-siteupdates@teresco.org <<EOF
A Travel Mapping site update has just successfully completed.
The complete log is available at https://travelmapping.net/logs/siteupdate.log .
Please report any problems to travmap@teresco.org .
EOF
echo "$0: complete"
date

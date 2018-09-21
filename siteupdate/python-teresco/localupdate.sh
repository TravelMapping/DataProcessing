#!/usr/bin/env bash
#
# script to run a site update on the same system as the web front end
#
# Jim Teresco, Tue May 15 15:57:28 EDT 2018
#
set -e
install=1
pull=1
tmbase=$HOME/travelmapping
tmwebbase=/home/www/tm
tmpdir=/home/tmp/tm
datestr=`date '+%Y-%m-%d@%H:%M:%S'`
logdir=logs
statdir=stats
graphdir=graphdata
nmpmdir=nmp_merged
graphflag=
date
# process command line args
for arg in "$@"; do
    if [ "$arg" == "--nographs" ]; then
	# -k to siteupdate.py supresses graph generation
	graphflag="-k"
    fi
    if [ "$arg" == "--noinstall" ]; then
	install=0
    fi
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
mkdir -p $datestr/$logdir/users $datestr/$statdir $datestr/$nmpmdir $datestr/$logdir/nmpbyregion
if [ "$graphflag" != "-k" ]; then
    mkdir -p $datestr/$graphdir
fi

echo "$0: launching siteupdate.py"
PYTHONIOENCODING='utf-8' ./siteupdate.py -d TravelMapping-$datestr $graphflag -l $datestr/$logdir -c $datestr/$statdir -g $datestr/$graphdir -n $datestr/$nmpmdir | tee $datestr/$logdir/siteupdate.log 2>&1 || exit 1
date

if [ -x ../../nmpfilter/nmpfilter ]; then
    echo "$0: running nmpfilter"
    ../../nmpfilter/nmpfilter $tmbase/HighwayData/hwy_data  $datestr/$logdir/tm-master.nmp $datestr/$logdir/nmpbyregion/
    echo "$0: creating zip archive of all nmp files created by nmpfilter"
    cd $datestr/$logdir/nmpbyregion
    zip -q nmpbyregion.zip *.nmp
    cd -
else
    echo "$0: SKIPPING nmpfilter (../../nmpfilter/nmpfilter not executable)"
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
echo "$0: switching to DB copy"
ln -sf $tmwebbase/lib/tm.conf.updating $tmwebbase/lib/tm.conf
touch $tmwebbase/dbupdating
echo "$0: loading primary DB"
date
mysql --defaults-group-suffix=tmapadmin -u travmapadmin TravelMapping < TravelMapping-$datestr.sql
/bin/rm $tmwebbase/dbupdating
echo "$0: switching to primary DB"
date
ln -sf $tmwebbase/lib/tm.conf.standard $tmwebbase/lib/tm.conf
echo "$0: loading DB copy"
mysql --defaults-group-suffix=tmapadmin -u travmapadmin TravelMappingCopy < TravelMapping-$datestr.sql
echo "$0: moving sql file to archive"
mv TravelMapping-$datestr.sql $tmpdir
echo "$0: sending email notification"
mailx -s "Travel Mapping Site Update Complete" travelmapping-siteupdates@teresco.org <<EOF
A Travel Mapping site update has just successfully completed.
The complete log is available at http://travelmapping.net/logs/siteupdate.log .
Please report any problems to travmap@teresco.org .
EOF
echo "$0: complete"
date

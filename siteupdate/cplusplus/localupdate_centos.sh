#!/usr/bin/env bash
#
# script to run a site update on the same system as the web front end
#
# Jim Teresco, Tue May 15 15:57:28 EDT 2018
#
set -e
install=1
pull=1
execdir=`pwd`
tmbase=$HOME/tm
tmwebbase=/var/www/html
tmpdir=$HOME/tmp/tm
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
	# -k to siteupdate supresses graph generation
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
if [ "$install" == "1" ]; then
    echo "$0: switching to DB copy"
    ln -sf $tmwebbase/lib/tm.conf.updating $tmwebbase/lib/tm.conf
    touch $tmwebbase/dbupdating
    echo "**********************************************************************"
    echo "**********************************************************************"
    echo "*                                                                    *"
    echo "* CHECKING FOR USER SLEEP MYSQL PROCESSES USING SHOW PROCESSLIST;    *"
    echo "* REMOVE ANY ENTRIES BEFORE THE SITE UPDATE SCRIPT FINISHES TO AVOID *"
    echo "* A POSSIBLE HANG DURING INGESTION OF THE NEW .sql FILE.             *"
    echo "*                                                                    *"
    echo "**********************************************************************"
    echo "**********************************************************************"
    echo "show processlist;" | mysql --defaults-group-suffix=travmap -u travmap
else
    echo "$0: SKIPPING switch to DB copy"
fi

echo "$0: gathering repo head info"
echo HighwayData '@' `(cd $tmbase/HighwayData; git show -s | head -n 1 | cut -f2 -d' ')` | tee $datestr/$logdir/siteupdate.log
echo UserData '@' `(cd $tmbase/UserData; git show -s | head -n 1 | cut -f2 -d' ')` | tee -a $datestr/$logdir/siteupdate.log
echo DataProcessing '@' `git show -s | head -n 1 | cut -f2 -d' '` | tee -a $datestr/$logdir/siteupdate.log

echo "$0: creating .time files"
cd $tmbase/UserData/time_files
for t in `ls ../list_files/*.list | sed -r 's~../list_files/(.*).list~\1.time~'`; do make $t; done
cd -

echo "$0: launching siteupdate"
./siteupdate -t 16 -d TravelMapping-$datestr $graphflag -l $datestr/$logdir -c $datestr/$statdir -g $datestr/$graphdir -n $datestr/$nmpmdir -u $tmbase/UserData/list_files -w $tmbase/HighwayData | tee -a $datestr/$logdir/siteupdate.log 2>&1 || exit 1
date

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

echo "$0: loading primary DB"
date
mysql --defaults-group-suffix=travmapadmin -u travmapadmin TravelMapping < TravelMapping-$datestr.sql
/bin/rm $tmwebbase/dbupdating
echo "$0: switching to primary DB"
date
ln -sf $tmwebbase/lib/tm.conf.standard $tmwebbase/lib/tm.conf

echo "$0: installing logs, stats, nmp_merged, graphs, archiving old contents in $tmpdir/$datestr"
mkdir -p $tmpdir/$datestr
mv $tmwebbase/$logdir $tmpdir/$datestr
mv $datestr/$logdir $tmwebbase
sudo chcon -R --type=httpd_sys_content_t $tmwebbase/$logdir
mv $tmwebbase/$statdir $tmpdir/$datestr
mv $datestr/$statdir $tmwebbase
sudo chcon -R --type=httpd_sys_content_t $tmwebbase/$statdir
mv $tmwebbase/$nmpmdir $tmpdir/$datestr
mv $datestr/$nmpmdir $tmwebbase
sudo chcon -R --type=httpd_sys_content_t $tmwebbase/$nmpmdir
if [ "$graphflag" != "-k" ]; then
    mv $tmwebbase/$graphdir $tmpdir/$datestr
    mv $datestr/$graphdir $tmwebbase
    sudo chcon -R --type=httpd_sys_content_t $tmwebbase/$graphdir
fi
rmdir $datestr

echo "$0: loading DB copy"
mysql --defaults-group-suffix=travmapadmin -u travmapadmin TravelMappingCopy < TravelMapping-$datestr.sql
echo "$0: moving sql file to archive"
mv TravelMapping-$datestr.sql $tmpdir
echo "$0: complete"
date

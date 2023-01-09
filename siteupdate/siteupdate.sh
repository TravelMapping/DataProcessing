#!/usr/bin/env bash
#
# TM common site update script
#
# Based on language-, OS-, and other specific scripts
#
# Jim Teresco, Sat Dec 31 22:59:48 EST 2022
#

function usage {
    echo "Usage: $0 [--help] [--nographs] [--noinstall] [--nopull] [--use-python] [--remote server] [--numthreads nt] [--tmbasedir dir] [--workdir dir] [--archivedir dir] [--webdir dir]"
}

set -e
date
datestr=`date '+%Y-%m-%d@%H:%M:%S'`

# start with some defaults, can override with command-line parameters
install=1
pull=1
graphflag=
language=cpp
remote=0
numthreads=1
workdir=.

# check default locations of TM repositories, can be overridden
# by the --tmbasedir command-line parameter
tmbasedir=$HOME/travelmapping
if [[ -d $HOME/tm ]]; then
    tmbasedir=$HOME/tm
fi

# We need GNU make, so use "gmake" if there is one, and if not, hope
# that "make" is a GNU version (this is only known to be a likely problem
# with FreeBSD, where "make" is BSD make).
if hash gmake 2>/dev/null; then
    make=gmake
else
    make=make
fi

# centos sometimes needs a "chcon" command for permissions, we'll run the
# null command "false" otherwise
chcon=false
if hash chcon 2>/dev/null; then
    chcon="sudo chcon -R --type=httpd_sys_content_t"
fi
# check default locations of TM web server files
# NOTE: should check on remote server when remote install
tmwebdir=/fast/www/tm
if [[ -d /var/www/html ]]; then
    tmwebdir=/var/www/html
fi

# check default locations for TM archives
tmarchivedir=/home/tmp/tm
if [[ -d $HOME/tmp/tm ]]; then
    tmarchivedir=$HOME/tmp/tm
fi

logdir=logs
statdir=stats
graphdir=graphdata
grapharchives=grapharchives
nmpmdir=nmp_merged

# process command line args
nextitem=
for arg in "$@"; do
    if [[ "$nextitem" == remote ]]; then
	nextitem=
	remote=$arg
    elif [[ "$nextitem" == numthreads ]]; then
	nextitem=
	numthreads=$arg
    elif [[ "$nextitem" == tmbasedir ]]; then
	nextitem=
	tmbasedir=$arg
    elif [[ "$nextitem" == workdir ]]; then
	nextitem=
	workdir=$arg
    elif [[ "$nextitem" == archivedir ]]; then
	nextitem=
	tmarchivedir=$arg
    elif [[ "$nextitem" == webdir ]]; then
	nextitem=
	tmwebdir=$arg
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
    elif [[ "$arg" == --tmbasedir ]]; then
	nextitem=tmbasedir
    elif [[ "$arg" == --workdir ]]; then
	nextitem=workdir
    elif [[ "$arg" == --archivedir ]]; then
	nextitem=archivedir
    elif [[ "$arg" == --webdir ]]; then
	nextitem=webdir
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

# find the right site update program based on the selected language
# and number of threads for the command
if [[ "$language" == python ]]; then
    if hash python3 2>/dev/null; then
	siteupdate="python3 python-teresco/siteupdate.py"
    else
	echo "No python3 command found."
	exit 1
    fi
elif [[ "$numthreads" != "1" ]]; then
    if [[ -x ./cplusplus/siteupdate ]]; then
	siteupdate="./cplusplus/siteupdate -t $numthreads"
    else
	echo "./cplusplus/siteupdate program not found."
	exit 1
    fi
else
    if [[ -x ./cplusplus/siteupdateST ]]; then
	siteupdate="./cplusplus/siteupdateST"
    else
	echo "./cplusplus/siteupdateST program not found."
	exit 1
    fi
fi

# make sure our work directory exists
if [[ ! -d $workdir ]]; then
    echo "Work directory $workdir does not exist, creating"
    mkdir $workdir
    if [[ ! -d $workdir ]]; then
	echo "Could not create work directory $workdir"
	exit 1
    fi
fi

# if local, make sure the tmwebdir directory exists
if [[ "$remote" == "0" ]]; then
    if [[ ! -d $tmwebdir ]]; then
	echo "Web directory $tmwebdir does not exist"
	exit 1
    fi
    if [[ ! -f $tmwebdir/participate.php ]]; then
	echo "Web directory $tmwebdir does not appear to contain a TM web instance"
	exit 1
    fi
fi

# if local, make sure the archive directory exists
if [[ "$remote" == "0" ]]; then
    if [[ ! -d $tmarchivedir ]]; then
	echo "Archive directory $tmarchivedir does not exist"
	exit 1
    fi
fi

# set up directory for this update in work directory
indir=$workdir/$datestr

# Update repositories unless not updating
if [[ "$pull" == "1" ]]; then
      echo "$0: updating TM repositories"
      (cd $tmbasedir/HighwayData; git pull)
      (cd $tmbasedir/UserData; git pull)
fi

echo "$0: creating directories"
mkdir -p $indir/$logdir/users $indir/$statdir $indir/$nmpmdir $indir/$logdir/nmpbyregion
if [[ "$graphflag" != "-k" ]]; then
    mkdir -p $indir/$graphdir
fi

echo "$0: gathering repo head info"
echo HighwayData '@' `(cd $tmbasedir/HighwayData; git show -s | head -n 1 | cut -f2 -d' ')` | tee $indir/$logdir/siteupdate.log
echo UserData '@' `(cd $tmbasedir/UserData; git show -s | head -n 1 | cut -f2 -d' ')` | tee -a $indir/$logdir/siteupdate.log
echo DataProcessing '@' `git show -s | head -n 1 | cut -f2 -d' '` | tee -a $indir/$logdir/siteupdate.log

echo "$0: creating .time files"
cd $tmbasedir/UserData/time_files
for t in `ls ../list_files/*.list | sed -r 's~../list_files/(.*).list~\1.time~'`; do $make $t; done
cd -
  
echo "$0: launching $siteupdate"
$siteupdate -d TravelMapping-$datestr $graphflag -l $indir/$logdir -c $indir/$statdir -g $indir/$graphdir -n $indir/$nmpmdir | tee -a $indir/$logdir/siteupdate.log 2>&1 || exit 1
date

if [[ -x ../nmpfilter/nmpbyregion ]]; then
    echo "$0: running nmpbyregion"
    ../nmpfilter/nmpbyregion $indir/$logdir/tm-master.nmp $indir/$logdir/nmpbyregion/
else
    echo "$0: SKIPPING nmpbyregion (no ../nmpfilter/nmpbyregion executable)"
fi

if [[ "$install" == "0" ]]; then
    echo "$0: SKIPPING file copies and DB update"
    exit 0
fi

if ! hash mysql 2>/dev/null; then
    echo "$0: No mysql program, SKIPPING file copies and DB update"
    exit 1
fi

if [[ "$graphflag" != "-k" ]]; then
    echo "$0: creating zip archive of all graphs"
    cd $indir/$graphdir
    zip -q graphs.zip *.tmg
    cd -
fi

echo "$0: switching to DB copy"
ln -sf $tmwebdir/lib/tm.conf.updating $tmwebdir/lib/tm.conf
touch $tmwebdir/dbupdating

echo "$0: loading primary DB"
date
mysql --defaults-group-suffix=tmapadmin -u travmapadmin TravelMapping < TravelMapping-$datestr.sql
if [[ -d $tmwebdir/$grapharchives ]]; then
    for archive in $tmwebdir/$grapharchives/*/*.sql; do
	mysql --defaults-group-suffix=tmapadmin -u travmapadmin TravelMapping < $archive
    done
fi
/bin/rm $tmwebdir/dbupdating
echo "$0: switching to primary DB"
date
ln -sf $tmwebdir/lib/tm.conf.standard $tmwebdir/lib/tm.conf

# NOTE: doesn't this install previous data in a directory with this update's
# date stamp?!
echo "$0: installing logs, stats, nmp_merged, graphs, archiving old contents in $tmarchivedir/$datestr"
mkdir -p $tmarchivedir/$datestr
mv $tmwebdir/$logdir $tmarchivedir/$datestr
mv $indir/$logdir $tmwebdir
$chcon $tmwebdir/$logdir
mv $tmwebdir/$statdir $tmarchivedir/$datestr
mv $indir/$statdir $tmwebdir
$chcon $tmwebdir/$statdir
mv $tmwebdir/$nmpmdir $tmarchivedir/$datestr
mv $instr/$nmpmdir $tmwebdir
if [[ "$graphflag" != "-k" ]]; then
    mv $tmwebdir/$graphdir $tmarchivedir/$datestr
    mv $indir/$graphdir $tmwebdir
    $chcon $tmwebdir/$graphdir
fi
rmdir $indir

echo "$0: loading DB copy"
mysql --defaults-group-suffix=tmapadmin -u travmapadmin TravelMappingCopy < TravelMapping-$datestr.sql
if [[ -d $tmwebdir/$grapharchives ]]; then
    for archive in $tmwebdir/$grapharchives/*/*.sql; do
	mysql --defaults-group-suffix=tmapadmin -u travmapadmin TravelMappingCopy < $archive
    done
fi
echo "$0: moving sql file to archive"
mv TravelMapping-$datestr.sql $tmarchivedir
if hash mailx 2>/dev/null; then
    echo "$0: sending email notification"
    mailx -s "Travel Mapping Site Update Complete" travelmapping-siteupdates@teresco.org <<EOF
A Travel Mapping site update has just successfully completed.
The complete log is available at https://travelmapping.net/logs/siteupdate.log .
Please report any problems to travmap@teresco.org .
EOF
else
    echo "$0: no mailx program, skipping email notification"
fi
echo "$0: complete"
date

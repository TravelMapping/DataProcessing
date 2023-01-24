#!/usr/bin/env bash
#
# TM common site update/datacheck script
#
# Based on language-, OS-, and other specific scripts
#
# Jim Teresco, Sat Dec 31 22:59:48 EST 2022 (started)
#

function usage {
    echo "Usage: $0 [--help] [--nographs] [--noinstall] [--nopull] [--mail] [--use-python] [--remote server] [--numthreads nt] [--tmbasedir dir] [--workdir dir] [--webdir dir]"
}

set -e
date
datestr=`date '+%Y-%m-%d@%H:%M:%S'`

# start with some defaults, can override with command-line parameters
install=1
mail=0
pull=1
graphflag=
errorcheck=
language=cpp
remote=0
numthreads=1
workdir=.
chcon=0
makesiteupdate=0
compress=0
compressflag=

# If running as datacheck, we change some defaults.
# Running from the common install?
if [[ "$0" == "/fast/tm/datacheck" ]]; then
    echo "$0: Running as common datacheck, setting appropriate defaults"
    install=0
    numthreads=4
    graphflag="-k"
    # -e flag runs site update in check-only mode
    errorcheck="-e"
fi

if [[ "$0" == "./datacheck.sh" ]]; then
    echo "$0: Running as datacheck, setting appropriate defaults"
    install=0
    numthreads=4
    graphflag="-k"
    # -e flag runs site update in check-only mode
    errorcheck="-e"
    # only in datacheck mode do we automatically make the latest
    # C++ site update (if using C++)
    makesiteupdate=1
fi

# check default locations of TM repositories, can be overridden
# by the --tmbasedir command-line parameter
tmbasedir=$HOME
if [[ -d $HOME/TravelMapping ]]; then
    tmbasedir=$HOME/TravelMapping
elif [[ -d $HOME/travelmapping ]]; then
    tmbasedir=$HOME/travelmapping
elif [[ -d $HOME/tm ]]; then
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

# Find a program to create .bz2, .gz, or .Z files
if hash bzip2 2>/dev/null; then
    compress=bzip2
    compressflag=-9
elif
    hash gzip 2>/dev/null; then
    compress=gzip
    compressflag=-9
elif
    hash compress 2>/dev/null; then
    compress=compress
fi

# If SELinux is installed and enabled, web install needs a "chcon"
# command
if hash getenforce 2>/dev/null; then
    geout=`getenforce`
    if [[ "$geout" != "Disabled" ]]; then
	chcon=1
    fi
fi
# check default locations of TM web server files
# NOTE: should check on remote server when remote install
tmwebdir=/fast/www/tm
if [[ -d /var/www/html ]]; then
    tmwebdir=/var/www/html
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
    elif [[ "$nextitem" == webdir ]]; then
	nextitem=
	tmwebdir=$arg
    elif [[ "$arg" == --help ]]; then
	usage
	exit 0
    elif [[ "$arg" == --graphs ]]; then
	graphflag=
    elif [[ "$arg" == --nographs ]]; then
	# -k to siteupdate supresses graph generation
	graphflag="-k"
    elif [[ "$arg" == --noinstall ]]; then
	install=0
    elif [[ "$arg" == --nopull ]]; then
	pull=0
    elif [[ "$arg" == --mail ]]; then
	mail=1
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

# make sure we have mailx if mail=1
if [[ "$mail" == "1" ]]; then
    if ! hash mailx 2>/dev/null; then
	mail=0
	echo "Ignoring --mail option, no mailx program found"
    fi
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
# and number of threads for the command, but first see if
# this is the common datacheck
if [[ "$0" == "/fast/tm/datacheck" ]]; then
    siteupdate="/fast/tm/DataProcessing/siteupdate/cplusplus/siteupdate -t $numthreads"
elif [[ "$language" == python ]]; then
    if hash python3 2>/dev/null; then
	PYTHONIOENCODING='utf-8'
	siteupdate="python3 python-teresco/siteupdate.py"
    else
	echo "No python3 command found."
	exit 1
    fi
else # C++
    if [[ "$makesiteupdate" == "1" ]]; then
	echo "$0: compiling the latest siteupdate program"
	cd cplusplus
	$make
	cd -
    fi
    if [[ "$numthreads" != "1" ]]; then
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

# if local and installing, make sure the tmwebdir directory exists
if [[ "$remote" == "0" && "$install" == "1" ]]; then
    if [[ ! -d $tmwebdir ]]; then
	echo "Web directory $tmwebdir does not exist"
	exit 1
    fi
    if [[ ! -f $tmwebdir/participate.php ]]; then
	echo "Web directory $tmwebdir does not appear to contain a TM web instance"
	exit 1
    fi
fi

indir=$workdir/$datestr
# for datacheck, no need for date-specific indir
if [[ "$0" == "./datacheck.sh" ]]; then
   indir=$workdir
fi
   
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
$siteupdate $errorcheck -d TravelMapping-$datestr $graphflag -l $indir/$logdir -c $indir/$statdir -g $indir/$graphdir -n $indir/$nmpmdir -w $tmbasedir/HighwayData -u $tmbasedir/UserData/list_files | tee -a $indir/$logdir/siteupdate.log 2>&1 || exit 1
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
    echo "$0: creating zip archive of all graphs (backgrounding)"
    cd $indir/$graphdir
    zip -q graphs.zip *.tmg &
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

instdir=$tmwebdir/updates/$datestr
echo "$0: installing logs, stats, nmp_merged, graphs in $instdir"
mkdir -p $instdir
mv $indir/$logdir $indir/$statdir $indir/$nmpmdir $instdir
if [[ "$chcon" == "1" ]]; then
    sudo chcon -R --type=httpd_sys_content_t $instdir/$logdir $instdir/$statdir $instdir/$nmpmdir
fi
if [[ "$graphflag" != "-k" ]]; then
    mv $indir/$graphdir $instdir
    if [[ "$chcon" == "1" ]]; then
	sudo chcon -R --type=httpd_sys_content_t $instdir/$graphdir
    fi
fi
rmdir $indir
echo "$0: linking current to $instdir"
cd $tmwebdir
rm -f current
ln -sf updates/$datestr current
cd -

echo "$0: loading DB copy"
mysql --defaults-group-suffix=tmapadmin -u travmapadmin TravelMappingCopy < TravelMapping-$datestr.sql
if [[ -d $tmwebdir/$grapharchives ]]; then
    for archive in $tmwebdir/$grapharchives/*/*.sql; do
	mysql --defaults-group-suffix=tmapadmin -u travmapadmin TravelMappingCopy < $archive
    done
fi
echo "$0: moving sql file to $instdir"
mv TravelMapping-$datestr.sql $instdir

if [[ "$compress" != "0" ]]; then
    echo "$0: Compressing TravelMapping-$datestr.sql with $compress $compressflag (backgrounded)"
    $compress $compressflag $instdir/TravelMapping-$datestr.sql &
fi

if [[ "$mail" == "1" ]]; then
    echo "$0: sending email notification"
    mailx -s "Travel Mapping Site Update Complete" travelmapping-siteupdates@teresco.org <<EOF
A Travel Mapping site update has just successfully completed.
The complete log is available at https://travelmapping.net/logs/siteupdate.log .
Please report any problems to travmap@teresco.org .
EOF
fi

echo "$0: complete"
date

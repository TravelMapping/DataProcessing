This document describes how to use the site update code on the Travel Mapping server to test your changes to highway data before submitting a pull request. The idea is to speed the process of getting your changes and updates into the site by finding things like missing or misnamed files, missing/erroneous/duplicate csv entries, a WPT file with malformed lines, mismatches between `.csv` and `_con.csv` files, misspelled CSV entries or file names, etc, etc, etc.  By testing before submitting the pull request, problems could be caught by those making the changes rather than having to wait for the next "official" site update to find out.

### Software Requirements

All you will need is an ssh client to connect to the FreeBSD server (currently noreaster.teresco.org) that is used by this project.  If you are on Windows, the most popular option is likely [PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/).  If you are on a Mac, your Terminal application already has what you need in the command-line ssh client.  Same for other Unix-like environments (Linux, FreeBSD, etc.).

### Obtaining an account and logging in

Request an account by email to travmap@teresco.org.  We will select a username and you can set a password when you first log in.  You will also be given a port number to specify with your Secure Shell (ssh) connection.  We run `sshd` on a nonstandard port to enhance security, so please don't publicize it.

#### For Windows users

To connect with PuTTY from Windows, run the "PuTTY" program (not pscp or WinSCP).  You will create a new connection and enter `noreaster.teresco.org` in the host name, and the port number that you will be given with your account information.  You will be prompted for your username and password.  

#### For Mac, Linux, and other Unix-like system users

From a Mac Terminal (which you can launch by searching for it by name with Spotlight) or other Unix-like command-line environment, you will connect with

```
ssh -l username -p portnum noreaster.teresco.org
```

Where you would replace "username" with your assigned username, and "portnum" with the number you will be given with your account information.

### First time setup

The first time you connect, you will need to clone the appropriate GitHub repositories into your account.  You will need a clone of the `HighwayData` that contains the files you wish to check and a clone of the `UserData` repository.  It is no longer necessary to clone the `DataProcessing` repository.  Most often, you will want your own fork of the first, and the master version of the latter, as this process is intended to help test changes to highway data.  We'll assume that's the case here, and will use "`jcool`" as the GitHub account into which you have forked the `HighwayData` repository.

Once you are logged in, you will see a prompt something like this:

```
[jcool@noreaster ~]$ 
```

That is your Unix command prompt, and by default it is in a "shell" program called Bash.  Basically, a shell is a way for you to issue commands directly to the operating system.  Our first commands will clone the needed repositories from GitHub.  Again, this assumes a GitHub username of "`jcool`".  Type each of these, in turn, at that `$` prompt.  You may be prompted for a GitHub username and password.  Any output from successful or unsuccessful commands will appear in your terminal, followed by a new `$` prompt.

```
git clone https://github.com/jcool/HighwayData.git
git clone https://github.com/TravelMapping/UserData.git
```

Don't forget to replace "jcool" with your GitHub username into which you have forked the `HighwayData` repository.

If all were successful, you should now have copies of each of the repositories in your account on the server.

### Running the site update code in "highway data check mode"

The steps above are only needed on the initial setup.  Everything from here on is what you'll do every time you want to perform a data check before issuing a pull request to bring your changes into the master.

A data check is completed by running the site update program in "highway data check mode".  Since for checking highway data updates involves only a subset of the tasks needed to complete an actual site update, a script has been provided that will run this program with appropriate parameters, and it will make sure your `HighwayData` and `UserData` repositories are up to date as well.

To run it, you will enter the following at your $ prompt:

```
/fast/tm/datacheck
```

The process that launches can take as long as a few minutes.  If the program runs to completion without reporting errors and you see

```
!!! DATA CHECK SUCCESSFUL !!!
```

a few lines from the end of your output, you should be ok to proceed with a pull request with your highway data changes. If not, you have things to fix.  The process will create some files in directories named `logs` and `stats` in your directory.  Most of the time you can ignore these.

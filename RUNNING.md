This document describes how to use the site update code on the Travel Mapping server to test your changes to highway data before submitting a pull request. The idea is to speed the process of getting your changes and updates into the site by finding things like missing or misnamed files, missing/erroneous/duplicate csv entries, a WPT file with malformed lines, mismatches between .csv and _con.csv files, misspelled CSV entries or file names, etc, etc, etc.  By testing before submitting the pull request, problems could be caught by those making the changes rather than having to wait for the next "official" site update to find out.

### Software Requirements

All you will need is an ssh client to connect to the FreeBSD server (currently noreaster.teresco.org) that is used by this project.  If you are on Windows, the most popular option is likely [PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/).  If you are on a Mac, your Terminal application already has what you need in the command-line ssh client.  Same for other Unix-like environments (Linux, FreeBSD, etc.).

### Obtaining an account and logging in

Request an account by email to travmap@teresco.org.  We will select a username and you can set a password when you first log in.  To connect with PuTTY from Windows, you will create a new connection and enter "noreaster.teresco.org" in the host name, and the port number that you will be given with your account information (we run sshd on a nonstandard port to enhance security).  You will be prompted for your username and password.  From a Mac Terminal or other Unix-like command-line environment, you will connect with

```
ssh -l username -p portnum noreaster.teresco.org
```

Where you would replace "username" with your assigned username, and "portnum" with the number you are given with your account information.

### First time setup

The first time you connect, you will need to clone the appropriate GitHub repositories into your account.  You will need the HighwayData repository, the UserData repository, and the DataProcessing repository.  Most often, you will want your own fork of the first, and the master versions of the latter two, as this process is intended to help test changes to highway data.  We'll assume that's the case here, and will use "jcool" as the GitHub account into which you have forked the HighwayData repository.

Once you are logged in, you will see a prompt something like this:

```
[jcool@noreaster ~]$ 
```

That is your Unix command prompt, and by default it is in a "shell" program called Bash.  Basically, a shell is a way for you to issue commands directly to the operating system.  Our first commands will clone the needed repositories from GitHub.  Again, this assumes a GitHub username of "jcool".  Type each of these, in turn, at that $ prompt.  You may be prompted for a GitHub username and password.  Any output from successful or unsuccessful commands will appear in your terminal, followed by a new $ prompt.

```
git clone https://github.com/jcool/HighwayData.git
git clone https://github.com/TravelMapping/UserData.git
git clone https://github.com/TravelMapping/DataProcessing.git
```

Don't forget to replace "jcool" with your GitHub username into which you have forked the HighwayData repository.

If all were successful, you should now have copies of each of the repositories in your account on the server.

### Running the site update code

At this time, your best bet is to run the full site update program, even though you don't need everything it does. https://github.com/TravelMapping/DataProcessing/issues/73 has been created as a reminder to simplify the process for this purpose.

To do this, you will enter the following at your $ prompt:

```
cd ~/DataProcessing/siteupdate/python-teresco
python3 siteupdate.py -k
```

The process that launches will likely run for several minutes.  The "-k" omits graph generation, which is one of the slowest parts of the process.  You can leave it out if you want to generate graphs.

If the program runs to completion without reporting errors, you are likely in good shape to make your pull request with your highway data changes.  If not, you have things to fix.

### Updating before subsequent runs

When you make changes to your data on GitHub, you will need to update your clone on noreaster to match before you run the site update process.  It's also a good idea to make sure the user data and data processing clones are up-to-date as well.  To do this, you will issue these commands:

```
cd ~/DataProcessing
git pull
cd ~/UserData
git pull
cd ~/HighwayData
git pull
```

At this point, you can run the site update program as described above.

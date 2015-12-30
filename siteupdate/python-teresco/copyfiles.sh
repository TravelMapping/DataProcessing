#!/usr/bin/env bash
#
bzip2 -9f siteupdate.sql ; scp siteupdate.sql.bz2 blizzard.teresco.org: ; scp *.log blizzard.teresco.org:public_html/travelmapping/logs

#include "crawl_hwy_data.h"
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>

void crawl_hwy_data(std::string path, std::unordered_set<std::string> &all_wpt_files, std::unordered_set<std::string> &splitsystems, std::string &splitregion, bool get_ss)
{	DIR *dir;
	dirent *ent;
	struct stat buf;
	if ((dir = opendir (path.data())) != NULL)
	{	while ((ent = readdir (dir)) != NULL)
		{	std::string entry = (path + "/") + ent->d_name;
			stat(entry.data(), &buf);
			if (S_ISDIR(buf.st_mode))
			{    if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..") && strcmp(ent->d_name, "_boundaries"))
			     {	if (get_ss)
				{	splitsystems.insert(ent->d_name);
					splitsystems.insert(std::string(ent->d_name)+'r');
				}
				crawl_hwy_data(entry, all_wpt_files, splitsystems, splitregion, splitregion==ent->d_name);
			     }
			}
			else if (entry.substr(entry.size()-4) == ".wpt")
				all_wpt_files.insert(entry);
		}
		closedir(dir);
	}
}

#include "crawl_rte_data.h"
#include "../classes/Args/Args.h"
#include "../classes/Route/Route.h"
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>

void crawl_rte_data(std::string path, std::unordered_set<std::string> &splitsystems, bool get_ss)
{	DIR *dir;
	dirent *ent;
	struct stat buf;
	if ((dir = opendir (path.data())) != NULL)
	{	while ((ent = readdir (dir)) != NULL)
		{	std::string entry = (path + "/") + ent->d_name;
			stat(entry.data(), &buf);
			if (S_ISDIR(buf.st_mode))
			{   if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..") && strcmp(ent->d_name, "_boundaries"))
			    {	if (get_ss)
				{	splitsystems.insert(ent->d_name);
					splitsystems.insert(ent->d_name+Args::splitregionapp);
				}
				crawl_rte_data(entry, splitsystems, Args::splitregion==ent->d_name);
			    }
			}
			else if (entry.substr(entry.size()-4) == ".wpt")
				Route::all_wpt_files.insert(entry);
		}
		closedir(dir);
	}
}

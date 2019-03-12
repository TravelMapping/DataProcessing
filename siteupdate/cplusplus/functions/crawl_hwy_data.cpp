void crawl_hwy_data(std::string path, std::unordered_set<std::string> &all_wpt_files)
{	DIR *dir;
	dirent *ent;
	struct stat buf;
	if ((dir = opendir (path.data())) != NULL)
	{	while ((ent = readdir (dir)) != NULL)
		{	std::string entry = (path + "/") + ent->d_name;
			stat(entry.data(), &buf);
			if (S_ISDIR(buf.st_mode))
			{    if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..") && strcmp(ent->d_name, "_boundaries"))
				crawl_hwy_data(entry, all_wpt_files);
			}
			else if (entry.substr(entry.size()-4) == ".wpt")
				all_wpt_files.insert(entry);
		}
		closedir(dir);
	}
}

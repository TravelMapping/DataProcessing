// Check for reversed border labels
// DE491@DE/PA&PA491@PA/DE						would become	DE491/PA491@PA/DE
// NB2@QC/NB&TCHMai@QC/NB&A-85Not@NB/QC&TCHMai@QC/NB			would become	NB2/TCHMai/A-85Not/@QC/NB
// US12@IL/IN&US20@IL/IN&US41@IN/IL&US12@IL/IN&US20@IL/IN&US41@IN/IL	would become	US12/US20/US41@IL/IN

const char *slash = strchr(label.data(), '/');
if (slash)
{	std::string reverse = (slash+1)+('/'+label.substr(0, slash-label.data()));
	unsigned int matches = 1;
	// ap_coloc[0]->label *IS* label, so no need to check that
	for (unsigned int i = 1; i < ap_coloc.size(); i++)
	  if (ap_coloc[i]->label == label || ap_coloc[i]->label == reverse)
	    matches++;
	  else break;
	if (matches == ap_coloc.size())
	{	std::vector<std::string> routes;
		for (Waypoint *w : ap_coloc)
			if (!list_contains(&routes, w->route->list_entry_name()))
				routes.push_back(w->route->list_entry_name());

		std::string newname = routes[0];
		for (unsigned int i = 1; i < routes.size(); i++)
			newname += '/' + routes[i];
		newname += '@' + label;
		log.push_back("Reversed_border_labels: " + name + " -> " + newname);
		return newname;
	}
}

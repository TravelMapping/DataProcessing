// straightforward 2-route intersection with matching labels
// NY30@US20&US20@NY30 would become NY30/US20
// or
// 2-route intersection with one or both labels having directional
// suffixes but otherwise matching route
// US24@CO21_N&CO21@US24_E would become US24_E/CO21_N

if (ap_coloc.size() == 2)
{	// check both refs independently, because datachecks are involved
	bool one_ref_zero = ap_coloc[1]->label_references_route(ap_coloc[0]->route);
	bool zero_ref_one = ap_coloc[0]->label_references_route(ap_coloc[1]->route);
	if (one_ref_zero && zero_ref_one)
	{	std::string newname = ap_coloc[1]->label+'/'+ap_coloc[0]->label;
		// if this is taken or if name_no_abbrev()s match, attempt to add in abbrevs if there's point in doing so
		if (ap_coloc[0]->route->abbrev.size() || ap_coloc[1]->route->abbrev.size())
		{    g->set_mtx[newname.back()].lock();
		     bool taken = g->vertex_names[newname.back()].find(newname) != g->vertex_names[newname.back()].end();
		     g->set_mtx[newname.back()].unlock();
		     if (taken || ap_coloc[0]->route->name_no_abbrev() == ap_coloc[1]->route->name_no_abbrev())
		     {	const char *u0 = strchr(ap_coloc[0]->label.data(), '_');
			const char *u1 = strchr(ap_coloc[1]->label.data(), '_');
			std::string newname = (ap_coloc[0]->route->list_entry_name() + (u1 ? u1 : "")) + "/"
					    + (ap_coloc[1]->route->list_entry_name() + (u0 ? u0 : ""));
			std::string message = "Straightforward_intersection: " + name + " -> " + newname;
			if (taken) message += " (" + ap_coloc[1]->label+'/'+ap_coloc[0]->label + " already taken)";
			g->namelog(std::move(message));
			return newname;
		     }
		}
		g->namelog("Straightforward_intersection: " + name + " -> " + newname);
		return newname;
	}
}

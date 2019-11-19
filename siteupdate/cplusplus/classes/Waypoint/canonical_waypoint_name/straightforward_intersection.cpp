// straightforward 2-route intersection with matching labels
// NY30@US20&US20@NY30 would become NY30/US20
// or
// 2-route intersection with one or both labels having directional
// suffixes but otherwise matching route
// US24@CO21_N&CO21@US24_E would become US24_E/CO21_N

if (ap_coloc.size() == 2)
{	std::string w0_list_entry = ap_coloc[0]->route->list_entry_name();
	std::string w1_list_entry = ap_coloc[1]->route->list_entry_name();
	std::string w0_label = ap_coloc[0]->label;
	std::string w1_label = ap_coloc[1]->label;
	if (	(w0_list_entry == w1_label or w1_label.find(w0_list_entry + '_') == 0)
	     &&	(w1_list_entry == w0_label or w0_label.find(w1_list_entry + '_') == 0)
	   )
	   {	log.push_back("Straightforward intersection: " + name + " -> " + w1_label + "/" + w0_label);
		return w1_label + "/" + w0_label;
	   }
}

// system graphs - only a few interesting systems; many aren't useful on their own
file.open(Args::datapath+"/graphs/systemgraphs.csv");
getline(file, line);  // ignore header line

while (getline(file, line))
{	for (h = HighwaySystem::syslist.data; h < sys_end; h++)
	  if (h->systemname == line)
	  {	GraphListEntry::add_group(
			h->systemname + "-system",
			h->systemname + " (" + h->fullname + ")",
			's', nullptr, new list<HighwaySystem*>(1, h), nullptr);
				      // deleted @ end of HighwayGraph::write_subgraphs_tmg
		break;
	  }
	if (h == sys_end)
		el.add_error("unrecognized system code "+line+" in systemgraphs.csv");
}
file.close();
graph_types.push_back({"system", "Routes Within a Single Highway System",
		       "These graphs contain the routes within a single highway system and are not restricted by region."});

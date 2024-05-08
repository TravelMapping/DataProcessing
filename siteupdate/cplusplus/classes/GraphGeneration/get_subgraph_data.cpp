// Find a set of vertices from the graph, optionally
// restricted by region or system or placeradius area.
if (g->placeradius)
	g->placeradius->vertices(mv, qt, this, *g, threadnum);
else	if (g->regions)
	{ if (g->systems)
	  {	// iterate via region & check for a system_match
		for (Region *r : *g->regions)
		  for (auto& vw : r->vertices)
		    if (!subgraph_contains(vw.first, threadnum) && vw.second->system_match(g->systems))
		    {	mv.push_back(vw.first);
			add_to_subgraph(vw.first, threadnum);
		    }
	  }
	  else	for (Region *r : *g->regions)
		  for (auto& vw : r->vertices)
		    if (!subgraph_contains(vw.first, threadnum))
		    {	mv.push_back(vw.first);
			add_to_subgraph(vw.first, threadnum);
		    }
	} else	for (HighwaySystem *h : *g->systems)
		  for (HGVertex* v : h->vertices)
		    if (!subgraph_contains(v, threadnum))
		    {	mv.push_back(v);
			add_to_subgraph(v, threadnum);
		    }

// initialize written booleans
for (HGVertex *v : mv)
{	for (HGEdge* e : v->incident_s_edges) e->written[threadnum] = 0;
	for (HGEdge* e : v->incident_c_edges) e->written[threadnum] = 0;
	for (HGEdge* e : v->incident_t_edges) e->written[threadnum] = 0;
}

// Compute sets of edges for subgraphs, optionally
// restricted by region or system or placeradius.
// Keep a count of collapsed & traveled vertices as we go.
#define AREA (!g->placeradius || subgraph_contains(e->vertex1, threadnum) && subgraph_contains(e->vertex2, threadnum))
#define REGION (!g->regions || contains(*g->regions, e->segment->route->region))
for (HGVertex *v : mv)
{	switch (v->visibility) // fall-thru is a Good Thing!
	{   case 2:
		cv_count++;
		for (HGEdge *e : v->incident_c_edges)
		  if (!(e->written[threadnum] & HGEdge::collapsed) && AREA && REGION && e->segment->system_match(g->systems))
		  {	mce.push_back(e);
			e->written[threadnum] |= HGEdge::collapsed;
		  }
	    case 1:
		tv_count++;
		for (HGEdge *e : v->incident_t_edges)
		  if (!(e->written[threadnum] & HGEdge::traveled) && AREA && REGION && e->segment->system_match(g->systems))
		  {	mte.push_back(e);
			e->written[threadnum] |= HGEdge::traveled;
			traveler_set.fast_union(e->segment->clinched_by);
		  }
	    default:
		for (HGEdge *e : v->incident_s_edges)
		  if (!(e->written[threadnum] & HGEdge::simple) && AREA && REGION && e->segment->system_match(g->systems))
		  {	mse.push_back(e);
			e->written[threadnum] |= HGEdge::simple;
		  }
	}   clear_vbit(v, threadnum);
}
#undef AREA
#undef REGION

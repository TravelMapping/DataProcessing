// Find sets of vertices & edges from the graph, optionally
// restricted by region or system or placeradius area.
auto pr = [&]()
{	TMBitset<HGVertex*, uint64_t> pr_mv(vertices.data(), vertices.size());
	TMBitset<HGEdge*,   uint64_t> pr_me(edges.data, edges.size);
	g->placeradius->matching_ve(pr_mv, pr_me, qt);
	pr_mv.shrink_to_fit();
	pr_me.shrink_to_fit();
	mv &= pr_mv;
	me &= pr_me;
};
if (g->regions)
     {	auto &r = *g->regions;
	mv = r[0]->vertices;
	me = r[0]->edges;
	for (size_t i = 1; i < r.size(); ++i)
	{	mv |= r[i]->vertices;
		me |= r[i]->edges;
	}
	if (g->systems)
	{	auto &h = *g->systems;
		auto sys_mv(h[0]->vertices);
		auto sys_me(h[0]->edges);
		for (size_t i = 1; i < h.size(); ++i)
		{	sys_mv |= h[i]->vertices;
			sys_me |= h[i]->edges;
		}
		mv &= sys_mv;
		me &= sys_me;
	}
	if (g->placeradius) pr();
     }
else if (g->systems)
     {	auto &h = *g->systems;
	mv = h[0]->vertices;
	me = h[0]->edges;
	for (size_t i = 1; i < h.size(); ++i)
	{	mv |= h[i]->vertices;
		me |= h[i]->edges;
	}
	if (g->placeradius) pr();
     }
else {	// We know there's a PlaceRadius, as no GraphListEntry
	// is created for invalid fullcustom.csv data
	mv.alloc(vertices.data(), vertices.size());
	me.alloc(edges.data, edges.size);
	g->placeradius->matching_ve(mv, me, qt);
	mv.shrink_to_fit();
	me.shrink_to_fit();
     }

// count vertices & initialize vertex numbers
for (HGVertex* v : mv)
{	int* vnum = HGVertex::vnums+(v-vertices.data())*3;
	switch (v->visibility) // fall-thru is a Good Thing!
	{	case 2:	 vnum[1] = cv_count++;
		case 1:	 vnum[2] = tv_count++;
		default: vnum[0] = sv_count++;
	}
}

// count edges & create traveler set
for (HGEdge* e : me)
{	if (e->format & HGEdge::simple)
		se_count++;
	if (e->format & HGEdge::collapsed)
		ce_count++;
	if (e->format & HGEdge::traveled)
	{	te_count++;
		traveler_set.fast_union(e->segment->clinched_by);
	}
}

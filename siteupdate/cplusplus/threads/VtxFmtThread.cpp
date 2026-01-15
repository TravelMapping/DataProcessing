void VtxFmtThread(unsigned int id, std::vector<HGVertex>* vertices)
{	for (auto v = vertices->begin()+id, end = vertices->end(); v < end; v += Args::numthreads)
		v->format_coordstr();
}

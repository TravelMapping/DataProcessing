/*
  Create a graph archive set and corresponding stats for METAL
  graph data.

  For each graph set created from the current TM data, compute
  additional stats about existing simple, collapsed, and traveled, and
  create an intersection-only graph from the collapsed and compure its
  stats as well.  The program creates a .sql file (with its name
  matching the archive name specified in argv[1]) with the additions
  to the graphArchive and graphArchiveSets for these graphs.  This
  should be then loaded into the TravelMapping and TravelMappingCopy
  DBs on the TM/METAL server.

  The program expects the environment variable HIGHWAY_DATA to be the
  path to the TravelMapping/HighwayData repository where the list of
  systems, regions, countries, and continents, as well as the custom
  graphs are defined.

  Jim Teresco, Summer 2022
*/

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

#include "tmggraph.h"
#include "tmg_graph_intersections_only.h"

#define MAX_PATH_LEN 4096

void usage(char *progname) {

  fprintf(stderr, "Usage: %s archive_name description datestr hwy_vers user_vers dataproc_vers write_dir\n", progname);
  exit(1);
}

/*
 * compute info and write the SQL line for one specific graph
 *
 * Returns 0 on success, -1 on failure.
 */
int process_one_graph(tmg_graph *g, char *filename, char *descr, char *category,
		  FILE *sqlfp) {

  printf("Processing %s (%s) category %s, %d waypoints, %d connections\n",
	 filename, descr, category, g->num_vertices, g->num_edges);
  return 0;
}

/* process the graphs for a particular set, reading the collapsed
 * (default), traveled, and simple, generating an intersection-only
 * from the collapsed, and computing and writing to the .sql file all
 * of the info about each one.
 *
 * Returns 0 on success, -1 on failure.
 */
int process_graph_set(char *filename, char *descr, char *category,
		      char *outdir, FILE *sqlfp) {

  printf("Processing set: %s (%s) category %s\n", filename, descr, category);

  /* start with the collapsed graph */
  char filepath[MAX_PATH_LEN];
  sprintf(filepath, "%s/%s.tmg", outdir, filename);
  tmg_graph *g = tmg_load_graph(filepath);
  if (!g) {
    return -1;
  }
  int retval = process_one_graph(g, filename, descr, category, sqlfp);

  /* if there was a problem, we stop here */
  if (retval == -1) {
    tmg_graph_destroy(g);
    return -1;
  }

  /* while we have the collapsed graph, create the intersection-only
   * version and process that */
  tmg_graph_intersections_only(g);

  /* write it */
  sprintf(filepath, "%s/%s-intonly.tmg", outdir, filename);
  FILE *ifp = fopen(filepath, "wt");
  if (!ifp) {
    fprintf(stderr, "Could not open file %s for writing\n", filepath);
    tmg_graph_destroy(g);
    return -1;
  }
  tmg_file_writer(g, ifp);
  fclose(ifp);
  
  sprintf(filepath, "%s-intonly", filename);
  retval = process_one_graph(g, filepath, descr, category, sqlfp);
  
  tmg_graph_destroy(g);
  if (retval == -1) return -1;

  /* simple */

  /* traveled */
  
  return 0;
}

int main(int argc, char *argv[]) {

  /* check for required command-line parameters:

     argv[1]: name of archive
     argv[2]: longer description of the archive set
     argv[3]: SQL-format date string YYYY-MM-DD
     argv[4]: TravelMapping/HighwayData repository version
     argv[5]: TravelMapping/UserData repository version
     argv[6]: TravelMapping/DataProcessing repository version
     argv[7]: location to find existing graphs and write
     intersection-only graphs and .sql, which must be a writable
     directory
  */
  if (argc != 8) {
    fprintf(stderr, "Error: incorrect number of command-line parameters.\n");
    usage(argv[0]);
  }

  char *archive_name = argv[1];
  char *description = argv[2];
  char *hwy_vers = argv[3];
  char *user_vers = argv[4];
  char *dataproc_vers = argv[5];
  /* should check here for a proper date format */
  char *datestamp = argv[6];
  char *outdir = argv[7];

  /* try to open the directory */
  DIR *d = opendir(outdir);
  if (d == NULL) {
    fprintf(stderr, "Could not open output directory %s\n", outdir);
    usage(argv[0]);
  }
  closedir(d);

  /* check for required environment variable */

  char *hwy_data = getenv("HIGHWAY_DATA");
  if (hwy_data == NULL) {
    fprintf(stderr, "The HIGHWAY_DATA environment variable must be set to the location of the TravelMapping/HighwayData repository.\n");
    exit(1);
  }
  d = opendir(hwy_data);
  if (d == NULL) {
    fprintf(stderr, "Could not open highway data directory %s\n", hwy_data);
    usage(argv[0]);
  }
  closedir(d);

  /* create the .sql file */
  char sqlfilename[MAX_PATH_LEN];
  sprintf(sqlfilename, "%s/%s.sql", outdir, archive_name);
  FILE *sqlfp = fopen(sqlfilename, "wt");
  if (sqlfp == NULL) {
    fprintf(stderr, "Could not open file %s for writing.\n", sqlfilename);
    perror(NULL);
    exit(1);
  }

  printf("New graph archive set %s (%s) datestamp %s\n", archive_name,
	 description, datestamp);
  printf("Creating graph archive SQL file %s\n", sqlfilename);
  printf("Graphs created by TM siteupdate are in and output will go to %s\n", outdir);
  printf("Graphs based on HighwayData rev %s in %s\n", hwy_vers, hwy_data);
  printf("and UserData rev %s, DataProcessing rev %s\n", user_vers,
	 dataproc_vers);

  /* new graphArchiveSets DB table entry for this archive set */
  fprintf(sqlfp, "INSERT INTO graphArchiveSets VALUES\n");
  fprintf(sqlfp, "('%s','%s','%s','%s','%s','%s');", archive_name, description,
	  datestamp, hwy_vers, user_vers, dataproc_vers);

  /* start processing graphs */

  printf("Processing a subset of region graphs\n");
  process_graph_set("RI-region", "Rhode Island", "region", outdir, sqlfp);
  process_graph_set("DE-region", "Delaware", "region", outdir, sqlfp);

  printf("Processing tm-master graphs\n");
  process_graph_set("tm-master", "All Travel Mapping Data", "master",
  		    outdir, sqlfp);
  
  fclose(sqlfp);
  
  return 0;
}


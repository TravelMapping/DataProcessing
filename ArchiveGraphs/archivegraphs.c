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
#include <string.h>
#include <unistd.h>

#include "stringfuncs.h"
#include "tmggraph.h"
#include "tmg_conn.h"
#include "tmg_graph_intersections_only.h"
#include "tmg_stats.h"

#define MAX_PATH_LEN 4096
#define MAX_REGION_CODE_LEN 8
#define MAX_REGION_NAME_LEN 128
#define MAX_COUNTRY_CODE_LEN 4
#define MAX_REGION_TYPE_LEN 32
#define MAX_BASE_GRAPH_NAME_LEN 64
#define MAX_GRAPH_DESCR_LEN 192
#define MAX_SYSTEM_CODE_LEN 12
#define MAX_LAT_LNG_LEN 10
#define MAX_RADIUS_LEN 5

/* utility function to read up to the given character from the 
   file into the string */
void read_to_char(FILE *fp, char *s, char c) {

  int index = 0;
  char ch;
  while (!feof(fp) && (ch = getc(fp)) != c) {
    if (ch > 127) continue;
    s[index++] = ch;
  }
  s[index] = '\0';
}

/* utility function to skip to the end of a line of input from a file */
void skip_to_eol(FILE *fp) {

  char ch;
  while (!feof(fp) && (ch = getc(fp)) != '\n');
}

void usage(char *progname) {

  fprintf(stderr, "Usage: %s archive_name description datestr hwy_vers user_vers dataproc_vers write_dir\n", progname);
  exit(1);
}

/*
 * compute info and write the SQL line for one specific graph
 *
 * Returns 0 on success, -1 on failure.
 */
int process_one_graph(tmg_graph *g, char *filename, char *descr,
		      char *category, char *archive_name, FILE *sqlfp) {

  char tmg_filename[MAX_PATH_LEN];
  printf("Processing %s (%s) category %s, %d waypoints, %d connections\n",
	 filename, descr, category, g->num_vertices, g->num_edges);

  /* compute some vertex stats */
  tmg_vertex_stats vs;
  tmg_graph_vertex_stats(g, &vs);

  /* an aspect ratio, width/height */
  double aspect_ratio;

  tmg_latlng center_west, center_east, center_north, center_south;
  center_west.lat = (g->vertices[vs.north]->w.coords.lat +
		     g->vertices[vs.south]->w.coords.lat) / 2;
  center_west.lng = g->vertices[vs.west]->w.coords.lng;
  center_east.lat = (g->vertices[vs.north]->w.coords.lat +
		     g->vertices[vs.south]->w.coords.lat) / 2;
  center_east.lng = g->vertices[vs.east]->w.coords.lng;
  center_north.lat = g->vertices[vs.north]->w.coords.lat;
  center_north.lng = (g->vertices[vs.west]->w.coords.lng +
		     g->vertices[vs.east]->w.coords.lng) / 2;
  center_south.lat = g->vertices[vs.south]->w.coords.lat;
  center_south.lng = (g->vertices[vs.west]->w.coords.lng +
		     g->vertices[vs.east]->w.coords.lng) / 2;

  // need points at different latitudes and longitudes
  if (center_west.lng != center_east.lng &&
      center_north.lat != center_south.lat) {
    aspect_ratio =
      tmg_distance_latlng(&center_west, &center_east) /
      tmg_distance_latlng(&center_north, &center_south);
  }
  else {
    aspect_ratio = 1;
  }
  
  // connectivity
  int num_parts;
  int *part_sizes;
  tmg_conn(g, &num_parts, &part_sizes);
  free(part_sizes);
  
  sprintf(tmg_filename, "%s.tmg", filename);
  fprintf(sqlfp, "('%s','%s','%d','%d','%d','%s','%s','%s','%d','%.4f','%4f','%d')\n",
	  tmg_filename, descr, g->num_vertices, g->num_edges, g->num_travelers,
	  tmg_format_names[g->format], category, archive_name,
	  vs.highest_degree, vs.average_degree, aspect_ratio, num_parts);
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
		      char *archive_name, char *outdir, FILE *sqlfp) {

  printf("Processing set: %s (%s) category %s\n", filename, descr, category);

  /* start with the collapsed graph */
  char filepath[MAX_PATH_LEN];
  sprintf(filepath, "%s/%s.tmg", outdir, filename);
  tmg_graph *g = tmg_load_graph(filepath);
  if (!g) {
    /* this will occur, for example, when a region in regions.csv
       does not have any TM-mapped highways */
    return -1;
  }

  fprintf(sqlfp, "INSERT INTO graphArchives VALUES\n");

  int retval = process_one_graph(g, filename, descr, category, archive_name,
				 sqlfp);

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
  fprintf(sqlfp, ",");
  retval = process_one_graph(g, filepath, descr, category, archive_name,
			     sqlfp);
  
  tmg_graph_destroy(g);
  if (retval == -1) return -1;

  /* simple */
  sprintf(filepath, "%s/%s-simple.tmg", outdir, filename);
  g = tmg_load_graph(filepath);
  if (!g) {
    fprintf(stderr, "Simple format graph not found!\n");
    fprintf(sqlfp, ";\n");
    return -1;
  }
  fprintf(sqlfp, ",");
  sprintf(filepath, "%s-simple", filename);
  retval = process_one_graph(g, filepath, descr, category, archive_name,
			     sqlfp);
  
  tmg_graph_destroy(g);
  if (retval == -1) return -1;

  /* traveled */
  sprintf(filepath, "%s/%s-traveled.tmg", outdir, filename);
  g = tmg_load_graph(filepath);
  if (!g) {
    fprintf(stderr, "Traveled format graph not found!\n");
    fprintf(sqlfp, ";\n");
    return -1;
  }
  fprintf(sqlfp, ",");
  sprintf(filepath, "%s-traveled", filename);
  retval = process_one_graph(g, filepath, descr, category, archive_name,
			     sqlfp);
  
  tmg_graph_destroy(g);
  if (retval == -1) return -1;

  fprintf(sqlfp, ";\n");
  
  return 0;
}

int main(int argc, char *argv[]) {

  /* check for required command-line parameters:

     argv[1]: name of archive
     argv[2]: longer description of the archive set, must be in double quotes
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

  if (argv[2][0] != '"') {
    fprintf(stderr, "Error: description must be specified in double quotes.\n");
    usage(argv[0]);
  }

  char *archive_name = argv[1];
  char *description = argv[2];
  // strip off double quotes from description
  description++;
  description[strlen(description)-1] = '\0';
  
  /* should check here for a proper date format */
  char *datestamp = argv[3];
  char *hwy_vers = argv[4];
  char *user_vers = argv[5];
  char *dataproc_vers = argv[6];
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
  char filenamebuf[MAX_PATH_LEN];
  sprintf(filenamebuf, "%s/%s.sql", outdir, archive_name);
  FILE *sqlfp = fopen(filenamebuf, "wt");
  if (sqlfp == NULL) {
    fprintf(stderr, "Could not open file %s for writing.\n", filenamebuf);
    perror(NULL);
    exit(1);
  }

  printf("New graph archive set %s (%s) datestamp %s\n", archive_name,
	 description, datestamp);
  printf("Creating graph archive SQL file %s\n", filenamebuf);
  printf("Graphs created by TM siteupdate are in and output will go to %s\n", outdir);
  printf("Graphs based on HighwayData rev %s in %s\n", hwy_vers, hwy_data);
  printf("and UserData rev %s, DataProcessing rev %s\n", user_vers,
	 dataproc_vers);

  /* new graphArchiveSets DB table entry for this archive set */
  fprintf(sqlfp, "INSERT INTO graphArchiveSets VALUES\n");
  fprintf(sqlfp, "('%s','%s','%s','%s','%s','%s');\n", archive_name, description,
	  datestamp, hwy_vers, user_vers, dataproc_vers);

  /* start processing graphs */

  printf("Processing region graphs\n");
  /* use regions.csv for the list of possibilities */
  sprintf(filenamebuf, "%s/regions.csv", hwy_data);
  FILE *rfp = fopen(filenamebuf, "rt");
  if (!rfp) {
    fprintf(stderr, "Could not open regions file %s\n", filenamebuf);
    fclose(rfp);
    exit(1);
  }

  /* buffers for csv fields */
  char code[MAX_REGION_CODE_LEN];
  char name[MAX_REGION_NAME_LEN];
  char country[MAX_COUNTRY_CODE_LEN];
  char continent[MAX_COUNTRY_CODE_LEN];
  char region_type[MAX_REGION_TYPE_LEN];

  /* buffers for additional fields */
  char graph_basefilename[MAX_BASE_GRAPH_NAME_LEN];
  char graph_descr[MAX_GRAPH_DESCR_LEN];
  
  /* read and skip the header line */
  skip_to_eol(rfp);
  /* read remaining lines, know we're done when we get an empty code field */
  while (1) {
    read_to_char(rfp, code, ';');
    if (strlen(code) < 2) break;
    read_to_char(rfp, name, ';');
    read_to_char(rfp, country, ';');
    read_to_char(rfp, continent, ';');
    read_to_char(rfp, region_type, '\n');
    sprintf(graph_basefilename, "%s-region", code);
    sprintf(graph_descr, "%s (%s)", name, region_type);
    process_graph_set(graph_basefilename, graph_descr, "region", archive_name,
    		      outdir, sqlfp);
  }
  fclose(rfp);

  
  printf("Processing system graphs\n");
  /* use systemgraphs.csv for the list of possibilities */
  sprintf(filenamebuf, "%s/graphs/systemgraphs.csv", hwy_data);
  FILE *sfp = fopen(filenamebuf, "rt");
  if (!sfp) {
    fprintf(stderr, "Could not open systemgraphs file %s\n", filenamebuf);
    fclose(sfp);
    exit(1);
  }

  // read the list of system graphs
  char syscode[MAX_SYSTEM_CODE_LEN];
  struct stringlist *systems_to_process_list = new_stringlist();
  while (fscanf(sfp,"%s", syscode) == 1) {
    stringlist_append(systems_to_process_list, strdup(syscode));
  }
  fclose(sfp);

  // now have a NULL-terminated array of system names
  char **systems_to_process = stringlist_getarray(systems_to_process_list);

  // go through systems.csv to find the info about matching systems
  sprintf(filenamebuf, "%s/systems.csv", hwy_data);
  sfp = fopen(filenamebuf, "rt");
  if (!sfp) {
    fprintf(stderr, "Could not open systems.csv file %s\n", filenamebuf);
    fclose(sfp);
    exit(1);
  }

  /* read and skip the header line */
  skip_to_eol(sfp);
  /* read remaining lines, know we're done when we get an empty code field */
  while (1) {
    read_to_char(sfp, syscode, ';');
    if (strlen(syscode) < 2) break;
    // see if this system is in our list
    int i = 0;
    while (systems_to_process[i] != NULL) {
      if (strcmp(systems_to_process[i++], syscode) == 0) {
	// we have a match, process this one
	read_to_char(sfp, country, ';');
	read_to_char(sfp, name, ';');
	sprintf(graph_basefilename, "%s-system", syscode);
	// in case we have any single quotes in the string
	double_up_single_quotes(name);
	process_graph_set(graph_basefilename, name, "system", archive_name,
			  outdir, sqlfp);
	break;
      }
    }
    skip_to_eol(sfp);
  }
  
  fclose(sfp);
  
  // clean up system to process list
  stringlist_free(systems_to_process_list);
  int i = 0;
  while (systems_to_process[i] != NULL) {
    free(systems_to_process[i++]);
  }
  free(systems_to_process);

  // area graphs
  printf("Processing area graphs\n");
  /* use areagraphs.csv for the list of graphs of this group */
  sprintf(filenamebuf, "%s/graphs/areagraphs.csv", hwy_data);
  FILE *afp = fopen(filenamebuf, "rt");
  if (!afp) {
    fprintf(stderr, "Could not open areagraphs.csv file %s\n", filenamebuf);
    fclose(afp);
    exit(1);
  }

  char latorlng[MAX_LAT_LNG_LEN];
  char radius[MAX_RADIUS_LEN];
  
  /* read and skip the header line */
  skip_to_eol(afp);
  /* read remaining lines, know we're done when we get an empty description
     field */
  while (1) {
    read_to_char(afp, name, ';');
    if (strlen(name) < 2) break;
    read_to_char(afp, code, ';');
    // skip lat and lng
    read_to_char(afp, latorlng, ';');
    read_to_char(afp, latorlng, ';');
    // radius, we need
    read_to_char(afp, radius, '\n');
    sprintf(graph_basefilename, "%s%s-area", code, radius);
    sprintf(graph_descr, "%s (%s mi radius)", name, radius);
    process_graph_set(graph_basefilename, graph_descr, "area", archive_name,
		      outdir, sqlfp);
  }

  fclose(afp);

  // multi-region graphs
  printf("Processing multiregion graphs\n");
  /* use multiregion.csv for the list of graphs of this group */
  sprintf(filenamebuf, "%s/graphs/multiregion.csv", hwy_data);
  FILE *mfp = fopen(filenamebuf, "rt");
  if (!mfp) {
    fprintf(stderr, "Could not open multiregion.csv file %s\n", filenamebuf);
    fclose(mfp);
    exit(1);
  }
  
  /* read and skip the header line */
  skip_to_eol(mfp);
  /* read remaining lines, know we're done when we get an empty description
     field */
  while (1) {
    read_to_char(mfp, graph_descr, ';');
    if (strlen(graph_descr) < 2) break;
    read_to_char(mfp, graph_basefilename, ';');
    // don't need the list of regions
    skip_to_eol(mfp);
    process_graph_set(graph_basefilename, graph_descr, "multiregion", archive_name, outdir, sqlfp);
  }

  fclose(mfp);

  // multi-system graphs
  printf("Processing multisystem graphs\n");
  /* use multisystem.csv for the list of graphs of this group */
  sprintf(filenamebuf, "%s/graphs/multisystem.csv", hwy_data);
  mfp = fopen(filenamebuf, "rt");
  if (!mfp) {
    fprintf(stderr, "Could not open multisystem.csv file %s\n", filenamebuf);
    fclose(mfp);
    exit(1);
  }
  
  /* read and skip the header line */
  skip_to_eol(mfp);
  /* read remaining lines, know we're done when we get an empty description
     field */
  while (1) {
    read_to_char(mfp, graph_descr, ';');
    if (strlen(graph_descr) < 2) break;
    read_to_char(mfp, graph_basefilename, ';');
    // don't need the list of systems
    skip_to_eol(mfp);
    process_graph_set(graph_basefilename, graph_descr, "multisystem", archive_name, outdir, sqlfp);
  }

  fclose(mfp);

  // full custom graphs
  printf("Processing full custom graphs\n");
  /* use fullcustom.csv for the list of graphs of this group */
  sprintf(filenamebuf, "%s/graphs/fullcustom.csv", hwy_data);
  mfp = fopen(filenamebuf, "rt");
  if (!mfp) {
    fprintf(stderr, "Could not open fullcustom.csv file %s\n", filenamebuf);
    fclose(mfp);
    exit(1);
  }
  
  /* read and skip the header line */
  skip_to_eol(mfp);
  /* read remaining lines, know we're done when we get an empty description
     field */
  while (1) {
    read_to_char(mfp, graph_descr, ';');
    if (strlen(graph_descr) < 2) break;
    read_to_char(mfp, graph_basefilename, ';');
    // don't need the remaining fields
    skip_to_eol(mfp);
    process_graph_set(graph_basefilename, graph_descr, "fullcustom", archive_name, outdir, sqlfp);
  }

  fclose(mfp);
    
  printf("Processing tm-master graphs\n");
  process_graph_set("tm-master", "All Travel Mapping Data", "master",
    		    archive_name, outdir, sqlfp);
  
  fclose(sqlfp);
  
  return 0;
}


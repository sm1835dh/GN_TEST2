/***********************************************************************
 ** Gracenote GOET Tables library                                     **
 **                                                                   **
 ** Copyright (c) Gracenote, 2013                                     **
 ***********************************************************************/
#include "goet_tables.h"
#include "gmd_json.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#define DEFAULT_MOOD_SCORE 0

#define GT_MAXLOGLINE 2048
#define GT_MAXFILENAME	1024
#define GT_MAXFILELINE	2048
#define GT_DATA_DELIM   "\t"

#define ID_SIZE_INIT 1024
#define ID_SIZE_GROW 1024


typedef struct {
    int index;
    char b_use_recording;
    char gmd_recording_id[GMD_GLOBAL_ID_SIZE];
    char gmd_song_id[GMD_GLOBAL_ID_SIZE];
    char gmd_release_year[GT_MAX_SIZE_RELEASEYEAR];
} gmd_recording_data_t;

typedef struct {
    unsigned int numerical_id;
    char gmd_id[GMD_GLOBAL_ID_SIZE];
    char gmd_release_year[GT_MAX_SIZE_RELEASEYEAR];
    char gmd_name[GT_MAX_SIZE_INPUTSTR];
} gmd_preferred_edition_t;

typedef struct {
    char gmd_edition_id[GMD_GLOBAL_ID_SIZE];
    char gmd_master_id[GMD_GLOBAL_ID_SIZE];
} gmd_edition_master_t;

typedef struct {
    char gmd_master_id[GMD_GLOBAL_ID_SIZE];
    char gmd_release_year[GT_MAX_SIZE_RELEASEYEAR];
    char gmd_name[GT_MAX_SIZE_INPUTSTR];
} gmd_master_data_t;

typedef struct {
    char gmd_recording_id[GMD_GLOBAL_ID_SIZE];
    char gmd_edition_id[GMD_GLOBAL_ID_SIZE];
    int release_type;
} gmd_rec_id_2_pref_edition_id_t;

typedef struct {
	char gmd_id[GMD_GLOBAL_ID_SIZE];
    int numerical_id;
} gmd_id_mapping_t;

static void			(*gt_logger)(char *) = NULL;
static char			*gt_file_artist_category = NULL;
static char			*gt_file_artist_list = NULL;
static char			*gt_file_artist_popularity = NULL;
static char			*gt_file_category = NULL;
static char			*gt_file_correlate = NULL;
static char			*gt_file_track_category = NULL;
static char			*gt_file_track_list = NULL;
static char			*gt_file_track_popularity = NULL;
static char			*gt_file_hierarchy_type = NULL;
static char			*gt_file_hierarchy_node = NULL;
static char			*gt_file_hierarchy_node_language = NULL;
static char			*gt_file_descriptor_to_hierarchy_node = NULL;
static char			*gt_file_language = NULL;
static char			*gt_file_album_edition = NULL;
static char			*gt_file_album_master = NULL;
static char			*gt_file_song = NULL;

static gt_artist_t		*gt_artists = NULL;
static size_t			gt_artists_cnt = 0;
static size_t			gt_artists_size = 0;

static gt_artist_t      **gt_artists_by_id = NULL;

static gt_track_t		*gt_tracks = NULL;
static size_t			gt_tracks_cnt = 0;
static size_t			gt_tracks_size = 0;

static size_t			gt_artgoet_allcnt = -1;
static size_t			gt_artname_allcnt = 0;
static size_t			gt_artpop_allcnt = 0;

static size_t			gt_trkgoet_allcnt = -1;
static size_t			gt_trkmdata_allcnt = 0;
static size_t			gt_trkpop_allcnt = 0;

static gt_goet_t		*gt_goets = NULL;
static size_t			gt_goets_cnt = 0;
static size_t			gt_goets_size = 0;

static size_t			gt_goetgoet_allcnt = 0;
static gt_goet_t		*gt_goetgoet_lastused = NULL;

static gt_goetcorr_index_t	gt_gci;

static int			gt_rank_unit;

static gt_lang_t		*gt_langs = NULL;
static size_t			gt_langs_cnt = 0;
static size_t			gt_langs_size = 0;

static gt_hier_t		*gt_hiers = NULL;
static size_t			gt_hiers_cnt = 0;
static size_t			gt_hiers_size = 0;

static gt_hier_node_t		*gt_hier_nodes = NULL;
static size_t			gt_hier_nodes_cnt = 0;
static size_t			gt_hier_nodes_size = 0;
static size_t			gt_hier_nodes_alllocstr = 0;

static size_t			gt_goet_to_hier_node_allcnt = 0;

static gt_column_override_t   *gt_column_override_list = NULL;
static size_t               gt_column_override_cnt = 0;

int add_goet(unsigned, int, char *);
int add_artist_goet(unsigned, unsigned, int, int);
int add_artist_name(unsigned, char *);
int add_artist_popularity(unsigned, unsigned);
int add_artist_track(gt_artist_t *, gt_track_t *);
int add_track_goet(unsigned, unsigned, int, int);
int add_track_mdata(unsigned, unsigned, char *, char *, unsigned, char *, int);
int add_track_goet_and_mdata(unsigned int id,
                             unsigned int artid,
                             char *artname,
                             char *name,
                             unsigned int albid,
                             char *albname,
                             int year,
                             gt_goetcorr_t *goets,
                             int cnt_goets);
int add_track_popularity(unsigned, unsigned);
int add_goet_goet(unsigned, unsigned, int, int);
int gt_search_track_by_similarity_prim(gt_artist_t *, gt_track_t *, int, int,
    int, gt_track_t **, int *);
int goetcorr_index_add(unsigned, unsigned, int);
int goetcorr_index_get(unsigned, unsigned);
int artist_redo_weights(gt_artist_t *);
int gt_add_track_links();
int add_hier(unsigned, char *);
int add_hier_node(unsigned, char *, unsigned, unsigned, unsigned);
int add_hier_node_locstr(unsigned, unsigned, char *);
int gt_add_hier_node_links();
int add_language(unsigned, char *, char *);
int add_goet_to_hier_node(unsigned, unsigned);

static int gt_load_table_artist_goets();
static int gt_load_table_artist_names();
static int gt_load_table_artist_popularity();
static int gt_load_table_goets();
static int gt_load_table_goet_correlations();
static int gt_load_table_recording_goets();
static int gt_load_table_recording_names();
static int gt_load_table_recording_popularities();
static int gt_load_table_hierarchies();
static int gt_load_table_hierarchy_nodes();
static int gt_load_table_descriptor_hierarchy_nodes();
static int gt_load_table_languages();
static int gt_load_table_hierarchy_localizations();

static int gmd_load_table_artists();
static int gmd_load_table_goets();
static int gmd_load_table_goet_correlations();
static int gmd_load_preferred_editions(gmd_preferred_edition_t **gmd_preferred_edition_list_out, int *cnt_preferred_editions_out);
static int gmd_load_one_edition_per_master(gmd_preferred_edition_t **gmd_preferred_edition_list_out, int *cnt_preferred_editions_out);
static int gmd_load_edition_recordings_data(gmd_preferred_edition_t *gmd_preferred_edition_list, int cnt_preferred_editions, gmd_rec_id_2_pref_edition_id_t **gmd_rec_2_ed_out, int *cnt_rec_2_ed_out);
static int gmd_load_recording_data(gmd_recording_data_t *gmd_recordings_list,
                                   int cnt_recordings,
                                   gmd_rec_id_2_pref_edition_id_t *rec2editions,
                                   int cnt_rec2editions,
                                   gmd_preferred_edition_t *editions_list,
                                   int cnt_editions,
                                   int do_one_recording_per_song);

static int gmd_load_recordings_to_use(gmd_recording_data_t **gmd_recordings_list_out,
                             int *cnt_recordings_out,
                             gmd_rec_id_2_pref_edition_id_t *rec2editions,
                             int cnt_rec2editions,
                             gmd_preferred_edition_t *editions_list,
                             int cnt_editions,
                             int do_one_recording_per_song);

static int gt_get_header_from_column(int, int, char *);
static int gt_get_column_index(int, int, char **, int);

static int gt_get_column_override(int, int, char *);

static int gt_init_artists_by_id_list();
static int comp_partist_by_id(const void *arg1, const void *arg2); /*array of pointer to artist*/

static int cmp_goetcorr_by_weight_type(const void *arg1, const void *arg2);

static unsigned int add_gmd_id(char *id);
static int initialize_sorted_id_list();
static int cmp_gmd_id_mapping(const void *arg1, const void *arg2);

static int cmp_gmd_recording_data(const void *arg1, const void *arg2);
static int cmp_gmd_preferred_editions(const void *arg1, const void *arg2);
static int cmp_gmd_rec_2_preferred_edition_id_wout_release_type(const void *arg1, const void *arg2);
static int cmp_gmd_rec_2_preferred_edition_id_w_release_type(const void *arg1, const void *arg2);
static int cmp_gmd_recording_index(const void *arg1, const void *arg2);
static int cmp_edition_master(const void *arg1, const void *arg2);
static int cmp_master_data(const void *arg1, const void *arg2);

static int get_sort_rank_from_release_type(int release_type);

gmd_id_mapping_t *id_to_gmd = NULL;
gmd_id_mapping_t *id_to_gmd_sort_by_string = NULL;

unsigned int id_count = 0;
unsigned int id_size = 0;

void
gt_log(char *fmt, ...)
{
	char buf[GT_MAXLOGLINE];
	va_list args;

	if(gt_logger == NULL)
		return;

	memset(buf, 0, GT_MAXLOGLINE);
	va_start(args, fmt);
	vsnprintf(buf, GT_MAXLOGLINE - 1, fmt, args);

	gt_logger(buf);

    	va_end(args);
}

static int
gettype(char *type)
{
	if(!strcmp(type, "GS"))
		return GT_TYPE_GENRE;
	else
	if(!strcmp(type, "OR"))
		return GT_TYPE_ORIGIN;
	else
	if(!strcmp(type, "ER"))
		return GT_TYPE_ERA;
	else
	if(!strcmp(type, "AT"))
		return GT_TYPE_ATYPE;
	else
	if(!strcmp(type, "MD"))
		return GT_TYPE_MOOD;
	else
	if(!strcmp(type, "TMM"))
		return GT_TYPE_TEMPO_M;
    else
    if(!strcmp(type, "LN"))
        return GT_TYPE_LANG;
	else
		return -1;
}



int
gt_init()
{
	int	size;
	char init_mapping[GMD_GLOBAL_ID_SIZE] = {0};

	gt_logger = NULL;

	size = GT_GOETCORRIDX_BUCKETS * sizeof(gt_goetcorr_ent_t *);
	gt_gci.gi_buckets = malloc(size);

	if(gt_gci.gi_buckets == NULL) {
		gt_log("Couldn't allocate GOET correlate index!");
		return ENOMEM;
	}

	memset(gt_gci.gi_buckets, 0, size);
	gt_gci.gi_maxdepth = 0;

	gt_rank_unit = 1;

	id_size = ID_SIZE_INIT;
	id_to_gmd = malloc(ID_SIZE_INIT * sizeof(gmd_id_mapping_t));
	if (id_to_gmd == NULL)
		return ENOMEM;
	add_gmd_id(init_mapping);

	return 0;
}

int
gt_uninit()
{
	int			i, j;
	gt_artist_t		*art;
	gt_goet_t		*goet;
	gt_track_t		*trk;
	gt_goetcorr_ent_t	*ent;
	gt_goetcorr_ent_t	*next;

	if(gt_artists) {
		for(i = 0; i < gt_artists_cnt; ++i) {
			art = &gt_artists[i];
			if(art->ga_name) {
				free(art->ga_name);
				art->ga_name = NULL;
			}
			if(art->ga_name_norm) {
				free(art->ga_name_norm);
				art->ga_name_norm = NULL;
			}
			if(art->ga_goetcorr) {
				free(art->ga_goetcorr);
				art->ga_goetcorr = NULL;
			}
			if(art->ga_tracks) {
				free(art->ga_tracks);
				art->ga_tracks = NULL;
			}
		}

		free(gt_artists);
		gt_artists = NULL;
	}

	if(gt_goets) {
		for(i = 0; i < gt_goets_cnt; ++i) {
			goet = &gt_goets[i];
			if(goet->gg_name) {
				free(goet->gg_name);
				goet->gg_name = NULL;
			}
			if(goet->gg_goetcorr) {
				free(goet->gg_goetcorr);
				goet->gg_goetcorr = NULL;
			}
			if (goet->gg_hier_node_ids) {
				free(goet->gg_hier_node_ids);
				goet->gg_hier_node_ids = NULL;
			}
		}

		free(gt_goets);
		gt_goets = NULL;
	}

	if(gt_tracks) {
		for(i = 0; i < gt_tracks_cnt; ++i) {
			trk = &gt_tracks[i];
			if(trk->gt_goetcorr) {
				free(trk->gt_goetcorr);
				trk->gt_goetcorr = NULL;
			}
			if(trk->gt_name) {
				free(trk->gt_name);
				trk->gt_name = NULL;
			}
			if(trk->gt_albname) {
				free(trk->gt_albname);
				trk->gt_albname = NULL;
			}
			if(trk->gt_artname) {
				free(trk->gt_artname);
				trk->gt_artname = NULL;
			}
		}

		free(gt_tracks);
		gt_tracks = NULL;
	}

	for(i = 0; i < GT_GOETCORRIDX_BUCKETS; ++i) {
		ent = gt_gci.gi_buckets[i];
		while(ent) {
			next = ent->ge_next;
			free(ent);
			ent = next;
		}
	}
	free(gt_gci.gi_buckets);
	gt_gci.gi_buckets = NULL;

	if(gt_file_artist_category) {
		free(gt_file_artist_category);
		gt_file_artist_category = 0;
	}
	if(gt_file_artist_list) {
		free(gt_file_artist_list);
		gt_file_artist_list = 0;
	}
	if(gt_file_artist_popularity) {
		free(gt_file_artist_popularity);
		gt_file_artist_popularity = 0;
	}
	if(gt_file_category) {
		free(gt_file_category);
		gt_file_category = 0;
	}
	if(gt_file_correlate) {
		free(gt_file_correlate);
		gt_file_correlate = 0;
	}
	if(gt_file_track_category) {
		free(gt_file_track_category);
		gt_file_track_category = 0;
	}
	if(gt_file_track_list) {
		free(gt_file_track_list);
		gt_file_track_list = 0;
	}
	if(gt_file_track_popularity) {
		free(gt_file_track_popularity);
		gt_file_track_popularity = 0;
	}
	if(gt_file_hierarchy_type) {
		free(gt_file_hierarchy_type);
		gt_file_hierarchy_type = 0;
	}
	if(gt_file_hierarchy_node) {
		free(gt_file_hierarchy_node);
		gt_file_hierarchy_node = 0;
	}
	if(gt_file_hierarchy_node_language) {
		free(gt_file_hierarchy_node_language);
		gt_file_hierarchy_node_language = 0;
	}
	if(gt_file_descriptor_to_hierarchy_node) {
		free(gt_file_descriptor_to_hierarchy_node);
		gt_file_descriptor_to_hierarchy_node = 0;
	}
	if(gt_file_language) {
		free(gt_file_language);
		gt_file_language = 0;
	}

    if (gt_artists_by_id)
    {
        free(gt_artists_by_id);
        gt_artists_by_id = NULL;
    }

	if (gt_hiers) {
		for (i = 0; i < gt_hiers_cnt; ++i) {
			free(gt_hiers[i].gh_name);
		}
		free(gt_hiers);
		gt_hiers = NULL;
	}

	if (gt_hier_nodes) {
		for (i = 0; i < gt_hier_nodes_cnt; ++i) {
			free(gt_hier_nodes[i].gn_name);

			if (gt_hier_nodes[i].gn_goet_ids) {
				free(gt_hier_nodes[i].gn_goet_ids);
			}

			for (j = 0; j < gt_hier_nodes[i].gn_locs_cnt; ++j) {
				free(gt_hier_nodes[i].gn_locs[j].go_str);
			}
			free(gt_hier_nodes[i].gn_locs);
		}

		free(gt_hier_nodes);
		gt_hier_nodes = NULL;
	}

	if (gt_langs) {
		for (i = 0; i < gt_langs_cnt; ++i) {
			free(gt_langs[i].gl_iso);
			free(gt_langs[i].gl_name);
		}

		free(gt_langs);
		gt_langs = NULL;
	}

	if (gt_column_override_list) {
		free(gt_column_override_list);
		gt_column_override_list = NULL;
	}

	if (id_to_gmd != NULL) {
		free(id_to_gmd);
		id_to_gmd = NULL;
	}

	if (id_to_gmd_sort_by_string != NULL) {
		free(id_to_gmd_sort_by_string);
		id_to_gmd_sort_by_string = NULL;
	}

    gt_goetgoet_lastused = NULL;

	return 0;
}


int
gt_set_logger(void (*logfunc)(char *))
{
	gt_logger = logfunc;
	return 0;
}

static int
cmp_artist_by_id(const void *arg1, const void *arg2)
{
	gt_artist_t *art1;
	gt_artist_t *art2;

	art1 = (gt_artist_t *) arg1;
	art2 = (gt_artist_t *) arg2;

	return art1->ga_id - art2->ga_id;
}


static int
cmp_artist_by_name(const void *arg1, const void *arg2)
{
	gt_artist_t *art1;
	gt_artist_t *art2;

	art1 = (gt_artist_t *) arg1;
	art2 = (gt_artist_t *) arg2;

	if(art1->ga_name_searchstart == NULL) {
		if(art2->ga_name_searchstart == NULL)
			return 0;
		return 1;
	} else
	if(art2->ga_name_searchstart == NULL) {
		return -1;
	}


	return strcmp(art1->ga_name_searchstart, art2->ga_name_searchstart);
}


static int
cmp_artist_by_pop(const void *arg1, const void *arg2)
{
	gt_artist_t *art1;
	gt_artist_t *art2;

	art1 = (gt_artist_t *) arg1;
	art2 = (gt_artist_t *) arg2;

	return art2->ga_pop - art1->ga_pop;
}


static int
cmp_artist_by_score(const void *arg1, const void *arg2)
{
	gt_artist_t	*art1;
	gt_artist_t	*art2;
	int		diff;

	art1 = (gt_artist_t *) arg1;
	art2 = (gt_artist_t *) arg2;

	diff = art2->ga_match_score - art1->ga_match_score;
	if(diff != 0)
		return diff;

	return art2->ga_pop - art1->ga_pop;
}


static int
cmp_artist_by_name_sub(const void *arg1, const void *arg2)
{
	/* Compares names, but allows the first string (the key in the text
	 * search) to be an anchored substring of the second.  */
	gt_artist_t *art1;
	gt_artist_t *art2;

	art1 = (gt_artist_t *) arg1;
	art2 = (gt_artist_t *) arg2;

	if(art1->ga_name_searchstart == NULL) {
		if(art2->ga_name_searchstart == NULL)
			return 0;
		return 1;
	} else
	if(art2->ga_name_searchstart == NULL) {
		return -1;
	}

	return strncmp(art1->ga_name_searchstart, art2->ga_name_searchstart,
	    strlen(art1->ga_name_searchstart));
}


static int
cmp_track_by_id(const void *arg1, const void *arg2)
{
	gt_track_t *trk1;
	gt_track_t *trk2;

	trk1 = (gt_track_t *) arg1;
	trk2 = (gt_track_t *) arg2;

	return trk1->gt_id - trk2->gt_id;
}


static int
cmp_track_by_art_id_pop(const void *arg1, const void *arg2)
{
	gt_track_t	*trk1;
	gt_track_t	*trk2;
	int		diff;

	trk1 = (gt_track_t *) arg1;
	trk2 = (gt_track_t *) arg2;

	diff = trk1->gt_art_id - trk2->gt_art_id;

	if(diff)
		return diff;

	return trk2->gt_pop - trk1->gt_pop;
}


static int
cmp_track_by_score(const void *arg1, const void *arg2)
{
	gt_track_t	*trk1;
	gt_track_t	*trk2;
	int		diff;

	trk1 = (gt_track_t *) arg1;
	trk2 = (gt_track_t *) arg2;

	diff = trk2->gt_match_score - trk1->gt_match_score;
	if(diff != 0)
		return diff;

	return trk2->gt_pop - trk1->gt_pop;
}


static int
cmp_goet(const void *arg1, const void *arg2)
{
	gt_goet_t	*goet1;
	gt_goet_t	*goet2;
	int		diff;

	goet1 = (gt_goet_t *) arg1;
	goet2 = (gt_goet_t *) arg2;

	diff =  goet1->gg_id - goet2->gg_id;

	if(diff == 0)
		diff =  goet1->gg_type - goet2->gg_type;

	return diff;
}

static int
cmp_goetcorr_by_weight_type(const void *arg1, const void *arg2)
{
    gt_goetcorr_t *g1 = (gt_goetcorr_t *) arg1;
    gt_goetcorr_t *g2 = (gt_goetcorr_t *) arg2;

    if (g1->gc_type == g2->gc_type)
        return g2->gc_weight - g1->gc_weight;

    return g1->gc_type - g2->gc_type;
}


static int
cmp_goet_by_type(const void *arg1, const void *arg2)
{
	gt_goet_t	*goet1;
	gt_goet_t	*goet2;
	int		cmpval;

	goet1 = (gt_goet_t *) arg1;
	goet2 = (gt_goet_t *) arg2;

	if(goet1->gg_type != goet2->gg_type)
		return (goet1->gg_type - goet2->gg_type);

	cmpval = strcasecmp(goet1->gg_name, goet2->gg_name);
	if(cmpval != 0)
		return cmpval;

	return (goet1->gg_id - goet2->gg_id);
}


int
gt_set_table_file(int type, const char *path)
{
	if(path == 0 || strlen(path) == 0) {
		return EINVAL;
	}

	switch(type) {
	case GT_TABLE_TYPE_ARTIST_CATEGORY:
		gt_file_artist_category = strdup(path);
		break;
	case GT_TABLE_TYPE_ARTIST_LIST:
		gt_file_artist_list = strdup(path);
		break;
	case GT_TABLE_TYPE_ARTIST_POPULARITY:
		gt_file_artist_popularity = strdup(path);
		break;
	case GT_TABLE_TYPE_CATEGORY:
		gt_file_category = strdup(path);
		break;
	case GT_TABLE_TYPE_CORRELATE:
		gt_file_correlate = strdup(path);
		break;
	case GT_TABLE_TYPE_TRACK_CATEGORY:
		gt_file_track_category = strdup(path);
		break;
	case GT_TABLE_TYPE_TRACK_LIST:
		gt_file_track_list = strdup(path);
		break;
	case GT_TABLE_TYPE_TRACK_POPULARITY:
		gt_file_track_popularity = strdup(path);
		break;
	case GT_TABLE_TYPE_HIERARCHY_TYPE:
		gt_file_hierarchy_type = strdup(path);
		break;
	case GT_TABLE_TYPE_HIERARCHY_NODE:
		gt_file_hierarchy_node = strdup(path);
		break;
	case GT_TABLE_TYPE_HIERARCHY_NODE_LANGUAGE:
		gt_file_hierarchy_node_language = strdup(path);
		break;
	case GT_TABLE_TYPE_DESCRIPTOR_TO_HIERARCHY_NODE:
		gt_file_descriptor_to_hierarchy_node = strdup(path);
		break;
	case GT_TABLE_TYPE_LANGUAGE:
		gt_file_language = strdup(path);
		break;
	case GT_TABLE_TYPE_ALBUM_EDITION:
		gt_file_album_edition = strdup(path);
		break;
	case GT_TABLE_TYPE_ALBUM_MASTER:
		gt_file_album_master = strdup(path);
		break;
	case GT_TABLE_TYPE_SONG:
		gt_file_song = strdup(path);
		break;
	default:
		return ENOENT;
	}

	return 0;
}

int
gt_load_tables(int load_recordings, int load_hierarchies, int data_type, int one_recording_per_song)
{
	switch(data_type) {
	case GT_DATA_FORMAT_TSV:
	    return gt_load_tables_lang(load_recordings, load_hierarchies, 1);
	    break;
	case GT_DATA_FORMAT_GMD:
	    return gt_load_gmd_json(load_recordings, load_hierarchies, one_recording_per_song);
	    break;
	default:
		return ENOENT;
	}
}

int
gt_load_gmd_json(int load_recordings, int load_hierarchies, int one_recording_per_song) {
	int err = 0;
	gmd_recording_data_t *gmd_recordings_list = NULL;
	int cnt_recordings = 0;
	gmd_preferred_edition_t *gmd_preferred_edition_list = NULL;
	int cnt_preferred_editions = 0;
	gmd_rec_id_2_pref_edition_id_t *gmd_rec_2_ed = NULL;
	int cnt_rec_2_ed = 0;

	if(gt_file_artist_list == NULL) {
		gt_log("No artist_list file!");
		return ENOENT;
	}
	if(gt_file_album_master == NULL) {
		gt_log("No album_master file!");
		return ENOENT;
	}
	if(gt_file_album_edition == NULL) {
		gt_log("No album_edition file!");
		return ENOENT;
	}
	if(gt_file_track_list == NULL) {
		gt_log("No recording file!");
		return ENOENT;
	}
	/*
	if(gt_file_song == NULL) {
		gt_log("No song file!");
		return ENOENT;
	}
	*/
	if(gt_file_category == NULL) {
		gt_log("No descriptor file!");
		return ENOENT;
	}
	if(gt_file_correlate == NULL) {
		gt_log("No correlate file!");
		return ENOENT;
	}
	/*
	if(gt_file_hierarchy_node == NULL) {
		gt_log("No hierarchy file!");
		return ENOENT;
	}
	*/

    if (load_hierarchies) {
        gt_log("Hierarchies not supported for GMD at this time.");
        return ENOENT;
    }

	gt_log("Loading tables...");

    /*artists*/
	if ((err = gmd_load_table_artists()) != 0) {
        goto end_label_gmd;
	}

	/*goets*/
	if ((err = gmd_load_table_goets()) != 0) {
        goto end_label_gmd;
	}

	/*goet correlations*/
	if ((err = gmd_load_table_goet_correlations()) != 0) {
        goto end_label_gmd;
	}

    /* only load recordings if enabled in config */
    if (load_recordings)
    {
        /* load preferred edition info */
        if ((err = gmd_load_preferred_editions(&gmd_preferred_edition_list, &cnt_preferred_editions)) != 0) {
            goto end_label_gmd;
        }

        /* handle case when preferred edition id is not included in gmd */
        if (cnt_preferred_editions == 0)
        {
            /* free list if not NULL */
            if (gmd_preferred_edition_list != NULL)
            {
               free(gmd_preferred_edition_list);
               gmd_preferred_edition_list = NULL;
            }

            gt_log("Did not detect any preferred edition ids, loading 1 edition per master...");
            if ((err = gmd_load_one_edition_per_master(&gmd_preferred_edition_list, &cnt_preferred_editions)) != 0) {
                goto end_label_gmd;
            }
        }

        /*load recordings to edition map + edition names from edition file */
        if ((err = gmd_load_edition_recordings_data(gmd_preferred_edition_list, cnt_preferred_editions, &gmd_rec_2_ed, &cnt_rec_2_ed)) != 0) {
            goto end_label_gmd;
        }

        /*load recording id, release year, song_id from recordings file to use to choose recordings to load*/
        if ((err = gmd_load_recordings_to_use(&gmd_recordings_list,
                                              &cnt_recordings,
                                              gmd_rec_2_ed,
                                              cnt_rec_2_ed,
                                              gmd_preferred_edition_list,
                                              cnt_preferred_editions,
                                              one_recording_per_song)) != 0) {
            goto end_label_gmd;

        }
        if ((err = gmd_load_recording_data(gmd_recordings_list,
                                           cnt_recordings,
                                           gmd_rec_2_ed,
                                           cnt_rec_2_ed,
                                           gmd_preferred_edition_list,
                                           cnt_preferred_editions,
                                           one_recording_per_song)) != 0) {
            goto end_label_gmd;
        }

        gt_log("Adding recording links to artists.");
		gt_add_track_links();
		gt_log("Done adding recording links to artists.");
    }
    else
    {
        gt_log("Won't load recordings.");
    }

	gt_log("Sorting artists and recordings.");
	/* Sort artists by name for search. */
	qsort(gt_artists, gt_artists_cnt, sizeof(gt_artist_t),
	    cmp_artist_by_name);
	qsort(gt_tracks, gt_tracks_cnt, sizeof(gt_track_t),
	    cmp_track_by_id);

	initialize_sorted_id_list();

    gt_init_artists_by_id_list();
	gt_log("Done sorting artists and recordings.");

end_label_gmd:
	if(err != 0) {
		(void) gt_uninit();
	}

	if (gmd_recordings_list != NULL)
        free(gmd_recordings_list);

    if (gmd_preferred_edition_list != NULL)
        free(gmd_preferred_edition_list);

    if (gmd_rec_2_ed != NULL)
        free(gmd_rec_2_ed);

	return err;
}

int
gt_load_tables_lang(int load_recordings, int load_hierarchies, int load_lang)
{
    int		err;

    err = 0;

    if(gt_file_artist_category == NULL) {
		gt_log("No artist_category file!");
		return ENOENT;
	}
	if(gt_file_artist_list == NULL) {
		gt_log("No artist_list file!");
		return ENOENT;
	}
	/* Won't check for popuarity file, as that one is optional. */
	if(gt_file_category == NULL) {
		gt_log("No category file!");
		return ENOENT;
	}
	if(gt_file_correlate == NULL) {
		gt_log("No correlate file!");
		return ENOENT;
	}

	if(load_hierarchies) {
		if(gt_file_hierarchy_node == NULL) {
			gt_log("No hierarchy node file!");
			return ENOENT;
		}
		if(gt_file_descriptor_to_hierarchy_node == NULL) {
			gt_log("No descriptor to hierarchy node file!");
			return ENOENT;
		}

		if (load_lang) {
            if(gt_file_hierarchy_node_language == NULL) {
                gt_log("No hierarchy node language file!");
                return ENOENT;
            }

            if(gt_file_language == NULL) {
                gt_log("No language file!");
                return ENOENT;
            }
		}
	}

	gt_log("Loading tables...");

    /*artist goet values*/
	if ((err = gt_load_table_artist_goets()) != 0) {
        goto end_label_tsv;
	}

    /*artist names*/
	if ((err = gt_load_table_artist_names()) != 0) {
        goto end_label_tsv;
	}

    /*artist popularity*/
    if (gt_file_artist_popularity) {

        if ((err = gt_load_table_artist_popularity()) !=0) {
            goto end_label_tsv;
        }
	} else {
	    gt_log("Won't load artist popularities.");
	}

	/*goets*/
	if ((err = gt_load_table_goets()) != 0) {
        goto end_label_tsv;
	}

	/*goet correlations*/
	if ((err = gt_load_table_goet_correlations()) != 0) {
        goto end_label_tsv;
	}

	if(load_recordings && gt_file_track_category) {

        /*recording goets*/
        if ((err = gt_load_table_recording_goets()) != 0) {
            goto end_label_tsv;
        }

        /*recording names*/
        if ((err = gt_load_table_recording_names()) != 0) {
            goto end_label_tsv;
        }

        if(gt_file_track_popularity) {

            /*recording popularities*/
            if ((err = gt_load_table_recording_popularities()) != 0) {
                goto end_label_tsv;
            }

        } else {
            gt_log("Won't load recording popularities.");
        }

        gt_log("Adding recording links to artists.");
		gt_add_track_links();
		gt_log("Done adding recording links to artists.");

	} else {
        gt_log("Won't load recordings.");
	}

	gt_log("Sorting artists and recordings.");
	/* Sort artists by name for search. */
	qsort(gt_artists, gt_artists_cnt, sizeof(gt_artist_t),
	    cmp_artist_by_name);
	qsort(gt_tracks, gt_tracks_cnt, sizeof(gt_track_t),
	    cmp_track_by_id);

    gt_init_artists_by_id_list();
	gt_log("Done sorting artists and recordings.");

	if (load_hierarchies) {

	    /*hierarchy file*/
	    if ((err = gt_load_table_hierarchies()) != 0) {
            goto end_label_tsv;
	    }

	    /*hierarchy nodes*/
	    if ((err = gt_load_table_hierarchy_nodes()) != 0) {
            goto end_label_tsv;
	    }

	    /*descriptor to hierarchy node links*/
	    if ((err = gt_load_table_descriptor_hierarchy_nodes()) != 0) {
            goto end_label_tsv;
	    }

        if (load_lang) {
            /*languages*/
            if ((err = gt_load_table_languages()) != 0) {
                goto end_label_tsv;
            }

            /*hierarchy language localizations*/
            if ((err = gt_load_table_hierarchy_localizations()) != 0) {
                goto end_label_tsv;
            }
        }
	} else {
		gt_log("Won't load hierarchies.");
	}

end_label_tsv:
	if(err != 0) {
		(void) gt_uninit();
	}

	return err;
}

#define GT_ARTISTS_INITSIZE	50000
#define GT_ARTISTS_GROW		25000

#define GT_ARTIST_GOET_INITSIZE	8
#define GT_ARTIST_GOET_GROW	4


int
add_artist_goet(unsigned artistid, unsigned goetid, int type, int weight)
{

	gt_artist_t	*art;
	gt_artist_t	*newart;
	gt_goetcorr_t	*newgoetcorr;
	gt_goetcorr_t	*goetcorr;

	/* We assume that we get GOETs for a single artist in one block,
	 * ie. in consecutive lines. */
	if(gt_artists == NULL) {
		/* First artist. */
		gt_artists = malloc(GT_ARTISTS_INITSIZE * sizeof(gt_artist_t));
		if(gt_artists == NULL) {
			gt_log("Can't allocate artist table!");
			return ENOMEM;
		}
		gt_artists_size = GT_ARTISTS_INITSIZE;
		gt_artists_cnt = 1;
		art = &gt_artists[0];
		memset(art, 0, sizeof(gt_artist_t));
		art->ga_id = artistid;
	} else
	if(gt_artists[gt_artists_cnt - 1].ga_id == artistid) {
		/* Existing artist. */
		art = &gt_artists[gt_artists_cnt - 1];
	} else {
		/* New artist. */

		if(gt_artists_cnt == gt_artists_size) {
			/* Need to grow. */
			newart = realloc(gt_artists,
			    (gt_artists_size + GT_ARTISTS_GROW) *
			    sizeof(gt_artist_t));
			if(newart == NULL) {
				gt_log("Can't increase size of artist table!");
				return ENOMEM;
			}

			gt_artists = newart;
			gt_artists_size += GT_ARTISTS_GROW;
		}

		gt_artists_cnt++;
		art = &gt_artists[gt_artists_cnt - 1];
		memset(art, 0, sizeof(gt_artist_t));
		art->ga_id = artistid;

/*
		if((gt_artists_cnt % 10000) == 0) {
			gt_log("Loaded %d artists.", gt_artists_cnt);
		}
*/
	}

	if(art->ga_goetcorr == NULL) {
		/* First GOET for artist. */
		art->ga_goetcorr = malloc(GT_ARTIST_GOET_INITSIZE *
		    sizeof(gt_goetcorr_t));
		if(art->ga_goetcorr == NULL) {
			gt_log("Can't allocate artist GOET list!");
			return ENOMEM;
		}
		art->ga_goetcorr_size = GT_ARTIST_GOET_INITSIZE;
	} else
	if(art->ga_goetcorr_size == art->ga_goetcorr_cnt) {
		/* Need to grow GOET list. */
		newgoetcorr = realloc(art->ga_goetcorr,
		    (art->ga_goetcorr_size + GT_ARTIST_GOET_GROW) *
                    sizeof(gt_goetcorr_t));
		if(newgoetcorr == NULL) {
			gt_log("Can't increase size of artist GOET list!");
			return ENOMEM;
		}
		art->ga_goetcorr = newgoetcorr;
		art->ga_goetcorr_size += GT_ARTIST_GOET_GROW;
	}

	goetcorr = &art->ga_goetcorr[art->ga_goetcorr_cnt];
	goetcorr->gc_type = type;
	goetcorr->gc_id = goetid;
	goetcorr->gc_weight = weight;

	art->ga_goetcorr_cnt++;

	gt_artgoet_allcnt++;

	return 0;
}


int
add_artist_name(unsigned id, char *name)
{
	gt_artist_t	*art;
	gt_artist_t	searchart;
	char		*ch;

	searchart.ga_id = id;

	art = (gt_artist_t *) bsearch(&searchart, gt_artists, gt_artists_cnt,
	    sizeof(gt_artist_t), cmp_artist_by_id);

	if(art == NULL)
		return ENOENT;

	// only add artist name if it hasn't already been added
	if (art->ga_name == NULL) {
		art->ga_name = strdup(name);
		if(art->ga_name == NULL) {
			gt_log("Can't allocate space for artist name.");
			return ENOMEM;
		}

		art->ga_name_norm = strdup(name);
		if(art->ga_name_norm == NULL) {
			gt_log("Can't allocate space for normalized artist name.");
			return ENOMEM;
		}

		/* Just case folding for now. */
		for(ch = art->ga_name_norm; *ch; ch++)
			*ch = tolower(*ch);

		/* Chop off beginning "the " for easier search. */
		if(!strncmp(art->ga_name_norm, "the ", 4))
			art->ga_name_searchstart = art->ga_name_norm + 4;
		else
			art->ga_name_searchstart = art->ga_name_norm;

		gt_artname_allcnt++;
	}
	return 0;

}


int
add_track_mdata(unsigned id, unsigned artid, char *artnam, char *name,
    unsigned albid, char *albname, int year)
{
	gt_track_t	*trk;
	gt_track_t	searchtrk;

	searchtrk.gt_id = id;

	trk = (gt_track_t *) bsearch(&searchtrk, gt_tracks, gt_tracks_cnt,
	    sizeof(gt_track_t), cmp_track_by_id);

	if(trk == NULL)
		return ENOENT;

	// If title is already set, the metadata has aldready been added
	if (trk->gt_name == NULL) {
		trk->gt_name = strdup(name);
		if(trk->gt_name == NULL) {
			gt_log("Can't allocate space for track name.");
			return ENOMEM;
		}

		trk->gt_albname = strdup(albname);
		if(trk->gt_albname == NULL) {
			gt_log("Can't allocate space for track album name.");
			return ENOMEM;
		}

		trk->gt_art_id = artid;

		trk->gt_alb_id = albid;

		trk->gt_releaseyear = year;

		if(artnam != NULL && strlen(artnam) > 0) {
			/* If we're given an artist name directly, store it away.
			 * We see this in the custom format when a track's artist is not
		 	* loaded but we get the name from the track file. */
			trk->gt_artname = strdup(artnam);
			if(trk->gt_artname == NULL)
				gt_log("Can't allocate memory for track's artist name.");
		}

		gt_trkmdata_allcnt++;
	}

	return 0;

}

int
add_artist_popularity(unsigned id, unsigned popularity)
{
	gt_artist_t	*art;
	gt_artist_t	searchart;

	searchart.ga_id = id;

	art = (gt_artist_t *) bsearch(&searchart, gt_artists, gt_artists_cnt,
	    sizeof(gt_artist_t), cmp_artist_by_id);

	if(art == NULL)
		return ENOENT;

	art->ga_pop = popularity;

	gt_artpop_allcnt++;

	return 0;
}


int
add_track_popularity(unsigned id, unsigned popularity)
{
	gt_track_t	*trk;
	gt_track_t	searchtrk;

	searchtrk.gt_id = id;

	trk = (gt_track_t *) bsearch(&searchtrk, gt_tracks, gt_tracks_cnt,
	    sizeof(gt_track_t), cmp_track_by_id);

	if(trk == NULL)
		return ENOENT;

	trk->gt_pop = popularity;

	gt_trkpop_allcnt++;

	return 0;
}


#define GT_TRACKS_INITSIZE	50000
#define GT_TRACKS_GROW		25000

#define GT_TRACK_GOET_INITSIZE	8
#define GT_TRACK_GOET_GROW	4


int
add_track_goet(unsigned trackid, unsigned goetid, int type, int weight)
{
	gt_track_t	*trk;
	gt_track_t	*newtrk;
	gt_goetcorr_t	*newgoetcorr;
	gt_goetcorr_t	*goetcorr;

	/* We assume that we get GOETs for a single track in one block,
	 * ie. in consecutive lines. */
	if(gt_tracks == NULL) {
		/* First track. */
		gt_tracks = malloc(GT_TRACKS_INITSIZE * sizeof(gt_track_t));
		if(gt_tracks == NULL) {
			gt_log("Can't allocate track table!");
			return ENOMEM;
		}
		gt_tracks_size = GT_TRACKS_INITSIZE;
		gt_tracks_cnt = 1;
		trk = &gt_tracks[0];
		memset(trk, 0, sizeof(gt_track_t));
		trk->gt_id = trackid;
	} else
	if(gt_tracks[gt_tracks_cnt - 1].gt_id == trackid) {
		/* Existing track. */
		trk = &gt_tracks[gt_tracks_cnt - 1];
	} else {
		/* New track. */

		if(gt_tracks_cnt == gt_tracks_size) {
			/* Need to grow. */
			newtrk = realloc(gt_tracks,
			    (gt_tracks_size + GT_TRACKS_GROW) *
			    sizeof(gt_track_t));
			if(newtrk == NULL) {
				gt_log("Can't increase size of tracks table!");
				return ENOMEM;
			}

			gt_tracks = newtrk;
			gt_tracks_size += GT_TRACKS_GROW;
		}

		gt_tracks_cnt++;
		trk = &gt_tracks[gt_tracks_cnt - 1];
		memset(trk, 0, sizeof(gt_track_t));
		trk->gt_id = trackid;

	}

	if(trk->gt_goetcorr == NULL) {
		/* First GOET for track. */
		trk->gt_goetcorr = malloc(GT_TRACK_GOET_INITSIZE *
		    sizeof(gt_goetcorr_t));
		if(trk->gt_goetcorr == NULL) {
			gt_log("Can't allocate track GOET list!");
			return ENOMEM;
		}
		trk->gt_goetcorr_size = GT_TRACK_GOET_INITSIZE;
	} else
	if(trk->gt_goetcorr_size == trk->gt_goetcorr_cnt) {
		/* Need to grow GOET list. */
		newgoetcorr = realloc(trk->gt_goetcorr,
		    (trk->gt_goetcorr_size + GT_TRACK_GOET_GROW) *
                    sizeof(gt_goetcorr_t));
		if(newgoetcorr == NULL) {
			gt_log("Can't increase size of track GOET list!");
			return ENOMEM;
		}
		trk->gt_goetcorr = newgoetcorr;
		trk->gt_goetcorr_size += GT_TRACK_GOET_GROW;
	}

	goetcorr = &trk->gt_goetcorr[trk->gt_goetcorr_cnt];
	goetcorr->gc_type = type;
	goetcorr->gc_id = goetid;
	goetcorr->gc_weight = weight;

	trk->gt_goetcorr_cnt++;

	gt_trkgoet_allcnt++;

	return 0;
}

int add_track_goet_and_mdata(unsigned int id,
                             unsigned int artid,
                             char *artname,
                             char *name,
                             unsigned int albid,
                             char *albname,
                             int year,
                             gt_goetcorr_t *goets,
                             int cnt_goets)
{

    gt_track_t *trk = NULL;

    /* init gt_tracks */
    if (gt_tracks == NULL)
    {
        /* First track. */
		gt_tracks = malloc(GT_TRACKS_INITSIZE * sizeof(gt_track_t));
		if(gt_tracks == NULL) {
			gt_log("Can't allocate track table!");
			return ENOMEM;
		}

        gt_tracks_size = GT_TRACKS_INITSIZE;
    }
    else if (gt_tracks_cnt == gt_tracks_size)
    {
        gt_tracks_size += GT_TRACKS_GROW;
        gt_tracks = realloc(gt_tracks, gt_tracks_size * sizeof(gt_track_t));
        if (gt_tracks == NULL) {
            gt_log("Can't allocate track table!");
            return ENOMEM;
        }
    }

    trk = &gt_tracks[gt_tracks_cnt];
    memset(trk, 0, sizeof(gt_track_t));

    trk->gt_id = id;
    trk->gt_alb_id = albid;
    trk->gt_art_id = artid;
    trk->gt_releaseyear = year;

    trk->gt_name = strdup(name);
    if (trk->gt_name == NULL)
    {
        gt_log("Can't allocate space for track name.");
        return ENOMEM;
    }

    trk->gt_artname = strdup(artname);
    if (trk->gt_artname == NULL)
    {
        gt_log("Can't allocate space for track's artist name.");
        return ENOMEM;
    }

    trk->gt_albname = strdup(albname);
    if (trk->gt_albname == NULL)
    {
        gt_log("Can't allocate space for track's album name.");
        return ENOMEM;
    }

    /* load goets */
    trk->gt_goetcorr = malloc(sizeof(gt_goetcorr_t) * cnt_goets);
    if (trk->gt_goetcorr == NULL)
    {
        gt_log("Can't allocate space for track goets.");
        return ENOMEM;
    }
    trk->gt_goetcorr_size = cnt_goets;
    trk->gt_goetcorr_cnt = cnt_goets;
    memcpy(trk->gt_goetcorr, goets, sizeof(gt_goetcorr_t) * cnt_goets);

    /* increment counters */
    gt_trkmdata_allcnt++;
    gt_tracks_cnt++;
    gt_trkgoet_allcnt += cnt_goets;

    return 0;
}

#define GT_GOETS_INITSIZE	1000
#define GT_GOETS_GROW		200


int
add_goet(unsigned id, int type, char *name)
{
	gt_goet_t	*goet;
	gt_goet_t	*newgoet;

	if(gt_goets == NULL) {
		/* First one. */
		gt_goets = malloc(GT_GOETS_INITSIZE * sizeof(gt_goet_t));
		if(gt_goets == NULL) {
			gt_log("Can't allocate GOET table!");
			return ENOMEM;
		}
		gt_goets_size = GT_GOETS_INITSIZE;
		gt_goets_cnt = 1;
		goet = &gt_goets[0];
		memset(goet, 0, sizeof(gt_goet_t));
	} else {

		if(gt_goets_cnt == gt_goets_size) {
			/* Need to grow. */
			newgoet = realloc(gt_goets,
			    (gt_goets_size + GT_GOETS_GROW) *
			    sizeof(gt_goet_t));
			if(gt_goets == NULL) {
				gt_log("Can't increase size of GOET table!");
				return ENOMEM;
			}

			gt_goets = newgoet;
			gt_goets_size += GT_GOETS_GROW;
		}

		gt_goets_cnt++;
		goet = &gt_goets[gt_goets_cnt - 1];
		memset(goet, 0, sizeof(gt_goet_t));
	}

	goet->gg_id = id;
	goet->gg_type = type;
	goet->gg_name = strdup(name);
	if(goet->gg_name == NULL) {
		gt_log("Can't copy GOET name!");
		return ENOMEM;
	}

	return 0;
}


#define GT_GOET_GOET_INITSIZE	64
#define GT_GOET_GOET_GROW	16


int
add_goet_goet(unsigned sourceid, unsigned targetid, int type, int weight)
{

	gt_goet_t	*goet;
	gt_goet_t	searchgoet;
	gt_goetcorr_t	*newgoetcorr;
	gt_goetcorr_t	*goetcorr;

	/* Optimization, we get correlates for the same ID in blocks of
	 * consecutive lines. */
	if(gt_goetgoet_lastused != NULL &&
	    gt_goetgoet_lastused->gg_id == sourceid) {
		goet = gt_goetgoet_lastused;
	} else {
		searchgoet.gg_id = sourceid;
		searchgoet.gg_type = type;
		goet = (gt_goet_t *) bsearch(&searchgoet, gt_goets,
		    gt_goets_cnt, sizeof(gt_goet_t), cmp_goet);
		if(goet == NULL)
			return ENOENT;

        gt_goetgoet_lastused = goet;
	}

	if(goet->gg_goetcorr == NULL) {
		/* First GOET for GOET. */
		goet->gg_goetcorr = malloc(GT_GOET_GOET_INITSIZE *
		    sizeof(gt_goetcorr_t));
		if(goet->gg_goetcorr == NULL) {
			gt_log("Can't allocate GOET's GOET list!");
			return ENOMEM;
		}
		goet->gg_goetcorr_size = GT_GOET_GOET_INITSIZE;
	} else
	if(goet->gg_goetcorr_size == goet->gg_goetcorr_cnt) {
		/* Need to grow GOET list. */
		newgoetcorr = realloc(goet->gg_goetcorr,
		    (goet->gg_goetcorr_size + GT_GOET_GOET_GROW) *
                    sizeof(gt_goetcorr_t));
		if(newgoetcorr == NULL) {
			gt_log("Can't increase size of GOET's GOET list!");
			return ENOMEM;
		}
		goet->gg_goetcorr = newgoetcorr;
		goet->gg_goetcorr_size += GT_GOET_GOET_GROW;
	}

	goetcorr = &goet->gg_goetcorr[goet->gg_goetcorr_cnt];
	goetcorr->gc_type = type;
	goetcorr->gc_id = targetid;
	goetcorr->gc_weight = weight;

	goet->gg_goetcorr_cnt++;

	gt_goetgoet_allcnt++;

	/* Add to the goet correlate index for fast lookups. */
	goetcorr_index_add(sourceid, targetid, weight);

	return 0;
}


int gt_search_artist_by_name(char *str, gt_artist_t **res, int *rescnt)
{
	char		*search;
	char		*ch;
	gt_artist_t	searchart;
	gt_artist_t	*artist;
	gt_artist_t	*cursor;
	gt_artist_t	*end;
	int		cnt;
	gt_artist_t	*arts;
	int		i;

	search = strdup(str);
	for(ch = search; *ch; ch++) {
		*ch = tolower(*ch);
	}
	searchart.ga_name_norm = search;

	/* Chop off beginning "the " for easier search. */
	if(!strncmp(search, "the ", 4))
		searchart.ga_name_searchstart = search + 4;
	else
		searchart.ga_name_searchstart = search;

	cnt = 0;

	/* Try searching for the full string. If no results, try to chop
	 * one character away until we end up with nothing. */
	while(strlen(searchart.ga_name_searchstart)) {

		artist = (gt_artist_t *) bsearch(&searchart, gt_artists,
		    gt_artists_cnt, sizeof(gt_artist_t),
		    cmp_artist_by_name_sub);
		if(artist == NULL) {
			/* Chop off one character, try again. */
			searchart.ga_name_searchstart[
			    strlen(searchart.ga_name_searchstart) - 1] = 0;
			continue;
		}

		/* Walk back to find the first result. */
		while(artist > gt_artists &&
		    !strncmp((artist - 1)->ga_name_searchstart,
		    searchart.ga_name_searchstart,
		    strlen(searchart.ga_name_searchstart))) {
			artist--;
		}

		/* Count the results. */
		cnt = 0;
		cursor = artist;
		end = gt_artists;
		end += gt_artists_cnt;
		while(cursor <= end &&
		    !strncmp(cursor->ga_name_searchstart,
		    searchart.ga_name_searchstart,
		    strlen(searchart.ga_name_searchstart))) {
			cnt++;
			cursor++;
		}

		if(cnt == 0)
			break;

		arts = malloc(cnt * sizeof(gt_artist_t));
		if(arts == NULL) {
			gt_log("Can't allocate result array!");
			return ENOMEM;
		}
		cursor = artist;
		for(i = 0; i < cnt; ++i) {
			arts[i] = *cursor;
			/* Don't copy name strings or goet arrays. */
			cursor++;
		}

		/* Sort results by popularity. */
		qsort(arts, cnt, sizeof(gt_artist_t), cmp_artist_by_pop);

		*res = arts;
		*rescnt = cnt;
		break;
	}

	free(search);

	if(cnt == 0)
		return ENOENT;
	else
		return 0;

}


#define GT_SEARCHRES_INITSIZE	500
#define GT_SEARCHRES_GROW	100


int
gt_search_artist_by_goet(int id, int weightthresh, int popthresh,
    gt_artist_t **res, int *rescnt)
{
	gt_artist_t	*art;
	gt_artist_t	*end;
	int		cnt;
	int		size;
	gt_artist_t	*arts;
	int		i;
	gt_goetcorr_t	*goetcorr;
	int		add;
	int		score;
	gt_artist_t	*newlist;
	int		wt;
	int		rankthresh;

	wt = weightthresh / 10;
	arts = malloc(GT_SEARCHRES_INITSIZE * sizeof(gt_artist_t));
	if(!arts) {
		gt_log("Can't allocate result array!");
		return ENOMEM;
	}
	size = GT_SEARCHRES_INITSIZE;
	cnt = 0;

	rankthresh = (1000 - popthresh) * gt_rank_unit;

	end = (gt_artists + gt_artists_cnt);
	for(art = gt_artists; art < end; art++) {
		add = 0;
		if(art->ga_rank > rankthresh)
			continue;
		for(i = 0; i < art->ga_goetcorr_cnt; ++i) {
			goetcorr = &art->ga_goetcorr[i];
			if(goetcorr->gc_id == id &&
			    goetcorr->gc_weight >= wt) {
				add = 1;
				/* For now, weight is the score, may get more
				 * fancy later. */
				score = goetcorr->gc_weight;
				break;
			}
		}
		if(add) {
			if(cnt == size) {
				/* Need to resize. */
				newlist = realloc(arts,
				    (size + GT_SEARCHRES_GROW) *
				    sizeof(gt_artist_t));
				if(newlist == NULL) {
					gt_log("Can't grow result array!");
					free(arts);
					return ENOMEM;
				}
				arts = newlist;
				size += GT_SEARCHRES_GROW;
			}
			arts[cnt] = *art;
			arts[cnt].ga_match_score = score;
			cnt++;
			/* Don't copy name strings or goet arrays. */
		}
	}

	if(cnt == 0) {
		free(arts);
		return ENOENT;
	} else {
		/* Sort results by popularity. */
		qsort(arts, cnt, sizeof(gt_artist_t), cmp_artist_by_pop);

		*res = arts;
		*rescnt = cnt;
		return 0;
	}

}


int
artist_redo_weights(gt_artist_t *art)
{
	/* Makes all goets of the same type have equal weight, adding up to
	 * 100. */
	int 		i;
	int 		t;
	int		tcnt;
	int		part;

	if(art == NULL)
		return EINVAL;

	for(i = 0; i < GT_NUM_TYPE; ++i) {
		tcnt = 0;
		for(t = 0; t < art->ga_goetcorr_cnt; ++t) {
			if(art->ga_goetcorr[t].gc_type == i)
				tcnt++;
		}

		if(tcnt == 0)
			continue;
		part = 100 / tcnt;

		for(t = 0; t < art->ga_goetcorr_cnt; ++t) {
			if(art->ga_goetcorr[t].gc_type == i)
				art->ga_goetcorr[t].gc_weight = part;
		}
	}

	return 0;

}


#define MAX_SEARCHLISTS	10
#define MAX_SEARCHGOETS	1024


int
gt_search_artist_by_goetlist(gt_search_goet_t *sgoet, int sgoetcnt,
    int simthresh, int popthresh, int redoweights,
    gt_artist_t **res, int *rescnt)
{
	gt_artist_t	searchart;
	gt_artist_t	*art;
	gt_artist_t	*end;
	int		cnt;
	int		size;
	gt_artist_t	*arts;
	int		add;
	gt_artist_t	*newlist;
	gt_similarity_t	sim;
	int		skipped;
	int		computed;
	int		i;
	gt_goet_t	*goet;
	int		ret;
	int		rankthresh;
	int		singlesearchtype;
	int		stype;

	memset(&searchart, 0, sizeof(gt_artist_t));



	/* Build a GOET list for the "fake" artist we will be searching for. */
	searchart.ga_goetcorr = malloc(sgoetcnt * sizeof(gt_goetcorr_t));
	if(searchart.ga_goetcorr == NULL) {
		gt_log("Can't allocate artist GOET list!");
		return ENOMEM;
	}
	searchart.ga_goetcorr_size = sgoetcnt;

	stype = -1;
	singlesearchtype = 1;
	for(i = 0; i < sgoetcnt; ++i) {
		ret = gt_lookup_goet(sgoet[i].gs_goet_id, &goet);
		if(ret != 0) {
			continue;
		}
		if(stype == -1) {
			stype = goet->gg_type;
		} else if(stype != goet->gg_type) {
			singlesearchtype = 0;
		}

		searchart.ga_goetcorr[searchart.ga_goetcorr_cnt].gc_type =
		    goet->gg_type;
		searchart.ga_goetcorr[searchart.ga_goetcorr_cnt].gc_id =
		    goet->gg_id;
		searchart.ga_goetcorr[searchart.ga_goetcorr_cnt].gc_weight =
		    sgoet[i].gs_weight;
		searchart.ga_goetcorr_cnt++;
	}

	if(redoweights)
		(void) artist_redo_weights(&searchart);

	rankthresh = (1000 - popthresh) * gt_rank_unit;

	arts = malloc(GT_SEARCHRES_INITSIZE * sizeof(gt_artist_t));
	if(!arts) {
		gt_log("Can't allocate result array!");
		free(searchart.ga_goetcorr);
		return ENOMEM;
	}
	size = GT_SEARCHRES_INITSIZE;
	cnt = 0;
	skipped = 0;
	computed = 0;

	end = (gt_artists + gt_artists_cnt);
	for(art = gt_artists; art < end; art++) {
		add = 0;

		if(art->ga_rank > rankthresh) {
			/* Skip artists above the rank threshold. */
			++skipped;
			continue;
		}

		/* Note: no early bailout (primary genre cutoff) here. */

		gt_compute_artist_similarity_prim(&searchart, art, 0, 0, &sim, NULL, 0);
		/* If we're only searching for a single type of GOET, we
		 * take that goet's similarity as the overall similarity. */
		if(singlesearchtype) {
			if(sim.gs_score_genre > 0)
				sim.gs_score_weighted = sim.gs_score_genre;
			else
			if(sim.gs_score_era > 0)
				sim.gs_score_weighted = sim.gs_score_era;
			else
			if(sim.gs_score_origin > 0)
				sim.gs_score_weighted = sim.gs_score_origin;
			else
			if(sim.gs_score_artist_type > 0)
				sim.gs_score_weighted =
				    sim.gs_score_artist_type;
		}
		++computed;
		if(sim.gs_score_weighted >= simthresh) {
			add++;
		} if(add) {
			if(cnt == size) {
				/* Need to resize. */
				newlist = realloc(arts,
				    (size + GT_SEARCHRES_GROW) *
				    sizeof(gt_artist_t));
				if(newlist == NULL) {
					gt_log("Can't grow result array!");
					free(arts);
					free(searchart.ga_goetcorr);
					return ENOMEM;
				}
				arts = newlist;
				size += GT_SEARCHRES_GROW;
			}
			arts[cnt] = *art;
			/* Again, no Step 1 cutoff. */
			arts[cnt].ga_match_score = sim.gs_score_weighted;
			cnt++;
			/* Don't copy name strings or goet arrays. */
		}
	}

	free(searchart.ga_goetcorr);

	if(cnt == 0) {
		free(arts);
		return ENOENT;
	} else {
		/* Sort results by popularity. */
		qsort(arts, cnt, sizeof(gt_artist_t), cmp_artist_by_score);

		*res = arts;
		*rescnt = cnt;
		return 0;
	}

	return 0;
}


int
gt_search_artist_by_similarity_prim(gt_artist_t *artist, int simthreshold,
    int popthreshold, int flags, gt_artist_t **res, int *rescnt)
{
	gt_artist_t	*art;
	gt_artist_t	*end;
	int		cnt;
	int		size;
	gt_artist_t	*arts;
	int		add;
	gt_artist_t	*newlist;
	gt_similarity_t	sim;
	int		skipped;
	int		computed;
	int		rankthresh;

	rankthresh = (1000 - popthreshold) * gt_rank_unit;

	arts = malloc(GT_SEARCHRES_INITSIZE * sizeof(gt_artist_t));
	if(!arts) {
		gt_log("Can't allocate result array!");
		return ENOMEM;
	}
	size = GT_SEARCHRES_INITSIZE;
	cnt = 0;
	skipped = 0;
	computed = 0;

	end = (gt_artists + gt_artists_cnt);
	for(art = gt_artists; art < end; art++) {
		add = 0;

		if(art->ga_rank > rankthresh) {
			/* Skip artists above the rank threshold. */
			++skipped;
			continue;
		}

		gt_compute_artist_similarity_prim(artist, art, flags, 0, &sim, NULL, 0);
		if(!sim.gs_step1_pass) {
			++skipped;
			continue;
		}

		++computed;
		if(sim.gs_similarity_value >= simthreshold)
			add++;
		if(add) {
			if(cnt == size) {
				/* Need to resize. */
				newlist = realloc(arts,
				    (size + GT_SEARCHRES_GROW) *
				    sizeof(gt_artist_t));
				if(newlist == NULL) {
					gt_log("Can't grow result array!");
					free(arts);
					return ENOMEM;
				}
				arts = newlist;
				size += GT_SEARCHRES_GROW;
			}
			arts[cnt] = *art;
			arts[cnt].ga_match_score = sim.gs_similarity_value;
			cnt++;
			/* Don't copy name strings or goet arrays. */
		}
	}

	gt_log("Skipped %d, computed %d\n", skipped, computed);

	if(cnt == 0) {
		free(arts);
		return ENOENT;
	} else {
		/* Sort results by popularity. */
		qsort(arts, cnt, sizeof(gt_artist_t), cmp_artist_by_score);

		*res = arts;
		*rescnt = cnt;
		return 0;
	}

}


int
gt_search_artist_by_similarity(int artistid, int simthreshold, int popthreshold,
    int flags, gt_artist_t **res, int *rescnt)
{
	gt_artist_t	*art;
	int		ret;

	ret = gt_lookup_artist(artistid, &art);
	if(ret != 0)
		return ret;

	return gt_search_artist_by_similarity_prim(art, simthreshold,
	    popthreshold, flags, res, rescnt);
}


void
gt_free_artist_reslist(gt_artist_t *arts)
{
	/* Artist names and goet arrays weren't copied, so don't have to
	 * be freed up. */
	free(arts);
}


int
gt_search_track_by_similarity_prim(gt_artist_t *artist, gt_track_t *track,
    int simthreshold, int popthreshold, int flags, gt_track_t **res,
    int *rescnt)
{
	gt_track_t	*trk;
	gt_track_t	*end;
	int		cnt;
	int		size;
	gt_track_t	*trks;
	int		add;
	gt_track_t	*newlist;
	gt_similarity_t	sim;
	int		skipped;
	int		computed;
	/*int		rankthresh;*/

	if(artist == NULL && track == NULL)
		return EINVAL;

	/* TODO rank unit for tracks. */
	/*rankthresh = (1000 - popthreshold) * gt_rank_unit;*/

	trks = malloc(GT_SEARCHRES_INITSIZE * sizeof(gt_track_t));
	if(!trks) {
		gt_log("Can't allocate result array!");
		return ENOMEM;
	}
	size = GT_SEARCHRES_INITSIZE;
	cnt = 0;
	skipped = 0;
	computed = 0;

	end = (gt_tracks + gt_tracks_cnt);
	for(trk = gt_tracks; trk < end; trk++) {
		add = 0;

		/* TODO. for now, ignore tracks with 0 popularity. */
		//if(trk->gt_pop > rankthresh) {
		//commenting out this part for mdsbrowser functionality, not sending customers popularity
		//if(trk->gt_pop == 0) {
			/* Skip tracks above the rank threshold. */
		//	++skipped;
		//	continue;
		//}


		if(track != NULL) {
			gt_compute_track_similarity_prim(track, trk, 0, 0,
			    &sim, NULL, 0);
			if(!sim.gs_step1_pass) {
				++skipped;
				continue;
			}

			++computed;
		} else if(artist != NULL) {

		    if ((flags & GT_SIMFLAG_NO_SEED_ARTIST) && artist->ga_id == trk->gt_art_id) {
		        ++skipped;
                continue;
            }

			gt_compute_artist_to_track_similarity_prim(artist, trk,
			    0, 0, &sim, NULL, 0);
			if(!sim.gs_step1_pass) {
				++skipped;
				continue;
			}

			++computed;
		}

		if(sim.gs_similarity_value >= simthreshold)
			add++;
		if(add) {
			if(cnt == size) {
				/* Need to resize. */
				newlist = realloc(trks,
				    (size + GT_SEARCHRES_GROW) *
				    sizeof(gt_track_t));
				if(newlist == NULL) {
					gt_log("Can't grow result array!");
					free(trks);
					return ENOMEM;
				}
				trks = newlist;
				size += GT_SEARCHRES_GROW;
			}
			trks[cnt] = *trk;
			trks[cnt].gt_match_score = sim.gs_similarity_value;
			cnt++;
			/* Don't copy name strings or goet arrays. */
		}
	}

	gt_log("Skipped %d, computed %d\n", skipped, computed);

	if(cnt == 0) {
		free(trks);
		return ENOENT;
	} else {
		/* Sort results by popularity. */
		qsort(trks, cnt, sizeof(gt_track_t), cmp_track_by_score);

		*res = trks;
		*rescnt = cnt;
		return 0;
	}
}


int
gt_search_track_by_similarity(int artistid, int trackid, int simthreshold,
    int popthreshold, int flags, gt_track_t **res, int *rescnt)
{
	gt_artist_t	*art;
	gt_track_t	*trk;
	int		ret;

	art = NULL;
	trk = NULL;

	if(artistid > 0) {
		ret = gt_lookup_artist(artistid, &art);
		if(ret != 0)
			return ret;
	} else if(trackid > 0) {
		ret = gt_lookup_track(trackid, &trk);
		if(ret != 0)
			return ret;
	} else
		return EINVAL;

	return gt_search_track_by_similarity_prim(art, trk, simthreshold,
	    popthreshold, flags, res, rescnt);
}


void
gt_free_track_reslist(gt_track_t *trks)
{
	/* Titles and goet arrays weren't copied, so don't have to
	 * be freed up. */
	free(trks);
}




int
gt_lookup_artist(int id, gt_artist_t **res)
{
    gt_artist_t art_key;
    gt_artist_t *p_art_key = &art_key;
	gt_artist_t	**art_result = NULL;

    p_art_key->ga_id = id;
    art_result = (gt_artist_t **) bsearch(&p_art_key, gt_artists_by_id, gt_artists_cnt, sizeof(gt_artist_t *), comp_partist_by_id);

    if (art_result == NULL)
        return ENOENT;

    *res = *art_result;
    return 0;
}


char *
gt_get_artist_name(int id)
{
	gt_artist_t	*art;
	gt_artist_t	*end;

	end = (gt_artists + gt_artists_cnt);
	for(art = gt_artists; art < end; art++) {
		if(art->ga_id == id) {
			return art->ga_name;
		}
	}

	return NULL;
}


int gt_search_goet_by_name(char *str, gt_goet_t **res, int *rescnt)
{
	char		*search;
	char		*ch;
	gt_goet_t	*goet;
	int		cnt;
	gt_goet_t	*goets;
	int		i;
	int		len;

	/* Since there are not that many goets, we just allocate a result
	 * array for the full count, this way we don't have to deal with
	 * growing the list, etc. */

	goets = malloc(gt_goets_cnt * sizeof(gt_goet_t));
	if(goets == NULL) {
		gt_log("Can't allocate result array!");
		return ENOMEM;
	}
	search = strdup(str);
	for(ch = search; *ch; ch++) {
		*ch = tolower(*ch);
	}

	cnt = 0;

	/* Try searching for the full string. If no results, try to chop
	 * one character away until we end up with nothing. */
	while(strlen(search)) {
		len = (int)strlen(search);
		/* Sequential... we only have a couple thousand. */
		for(i = 0; i < gt_goets_cnt; ++i) {
			goet = &gt_goets[i];
			if(!strncasecmp(search, goet->gg_name, len)) {
				goets[cnt] = *goet;
				++cnt;
			}
		}
		if(cnt > 0) {
			/* Sort results by type/alpha. */
			qsort(goets, cnt, sizeof(gt_goet_t), cmp_goet_by_type);

			*res = goets;
			*rescnt = cnt;
			break;
		}

		search[strlen(search) - 1] = 0;
	}

	free(search);

	if(cnt == 0) {
		free(goets);
		return ENOENT;
	} else {
		return 0;
	}
}


void
gt_free_goet_reslist(gt_goet_t *goets)
{
	/* Names and goet arrays weren't copied, so don't have to
	 * be freed up. */
	free(goets);
}


int
gt_lookup_goet(int id, gt_goet_t **res)
{
	gt_goet_t	*goet;
	gt_goet_t	*end;

	end = (gt_goets + gt_goets_cnt);
	for(goet = gt_goets; goet < end; goet++) {
		if(goet->gg_id == id) {
			*res = goet;
			return 0;
		}
	}

	return ENOENT;
}


int
gt_get_full_artistlist(gt_artist_t **res, int *cnt)
{
	*res = gt_artists;
	*cnt = (int)gt_artists_cnt;

	return 0;
}


int
gt_get_full_goetlist(gt_goet_t **res, int *cnt)
{
	*res = gt_goets;
	*cnt = (int)gt_goets_cnt;

	return 0;
}


int
gt_get_full_tracklist(gt_track_t **res, int *cnt)
{
	*res = gt_tracks;
	*cnt = (int)gt_tracks_cnt;

	return 0;
}


char *
gt_get_goet_name(int id, int type)
{
	gt_goet_t	searchgoet;
	gt_goet_t	*res;

	searchgoet.gg_id = id;
	searchgoet.gg_type = type;

	res = (gt_goet_t *) bsearch(&searchgoet, gt_goets,
		    gt_goets_cnt, sizeof(gt_goet_t), cmp_goet);

	if(res == NULL)
		return NULL;

	return res->gg_name;
}


char *
gt_get_goet_typestr(int id)
{
	switch(id) {
	case GT_TYPE_GENRE:
		return "GENRE";
	case GT_TYPE_ORIGIN:
		return "ORIGIN";
	case GT_TYPE_ERA:
		return "ERA";
	case GT_TYPE_ATYPE:
		return "ATYPE";
	case GT_TYPE_MOOD:
		return "MOOD";
	case GT_TYPE_TEMPO_M:
		return "TEMPO_MAIN";
    case GT_TYPE_LANG:
        return "LANGUAGE";
	}

	return NULL;
}


int
gt_lookup_track(int id, gt_track_t **res)
{
	gt_track_t	*trk;
	gt_track_t	searchtrk;

	searchtrk.gt_id = id;

	trk = (gt_track_t *) bsearch(&searchtrk, gt_tracks, gt_tracks_cnt,
	    sizeof(gt_track_t), cmp_track_by_id);

	if(trk) {
		*res = trk;
		return 0;
	}

	return ENOENT;
}


int
gt_get_goet_correlation(int id1, int id2)
{
	return goetcorr_index_get(id1, id2);
}


int
gt_get_artist_primary_goet_value(gt_artist_t *art, int type)
{
	int		i;
	gt_goetcorr_t	*gc;

	if(art == NULL)
		return 0;

	for(i = 0; i < art->ga_goetcorr_cnt; ++i) {
		gc = &art->ga_goetcorr[i];
		if(gc->gc_type == type) {
			return gc->gc_id;
		}
	}

	return 0;
}


int
gt_get_artist_goet_vector(gt_artist_t *art, int type, gt_goetcorr_t *gc,
    int maxgc)
{
	int	i;
	int	cnt;

	if(art == NULL)
		return 0;

	cnt = 0;
	for(i = 0; i < art->ga_goetcorr_cnt; ++i) {
		if(art->ga_goetcorr[i].gc_type == type) {
			gc[cnt] = art->ga_goetcorr[i];
			++cnt;
			if(cnt >= maxgc)
				break;
		}
	}

	return cnt;
}


int
gt_get_track_primary_goet_value(gt_track_t *trk, int type)
{
	int		i;
	gt_goetcorr_t	*gc;

	if(trk == NULL)
		return 0;

	for(i = 0; i < trk->gt_goetcorr_cnt; ++i) {
		gc = &trk->gt_goetcorr[i];
		if(gc->gc_type == type) {
			return gc->gc_id;
		}
	}

	return 0;
}


int
gt_get_track_goet_vector(gt_track_t *trk, int type, gt_goetcorr_t *gc,
    int maxgc)
{
	int	i;
	int	cnt;

	if(trk == NULL)
		return 0;

	cnt = 0;
	for(i = 0; i < trk->gt_goetcorr_cnt; ++i) {
		if(trk->gt_goetcorr[i].gc_type == type) {
			gc[cnt] = trk->gt_goetcorr[i];
			++cnt;
			if(cnt >= maxgc)
				break;
		}
	}

	return cnt;
}


static int gt_compare_goetcorrs(const void *gm1, const void *gm2) {
	int corr_diff	= 0;
	corr_diff = ((gt_goetcorr_match_t *)gm2)->gm_corr - ((gt_goetcorr_match_t *)gm1)->gm_corr;
	if (corr_diff != 0)
		return corr_diff;
	return 	(((gt_goetcorr_match_t *)gm1)->gm_order - ((gt_goetcorr_match_t *)gm2)->gm_order);
}

#define GT_NO_CORR	-1001
#define GT_MAX_VECTORSIZE		10


int
gt_compute_vector_similarity(gt_goetcorr_t *vect1, int vectcnt1,
    gt_goetcorr_t *vect2, int vectcnt2, int include_unknown_pair, int verbose, char *report_out, size_t size_report)
{
	/* NOTE: this function modifies the vectors passed in!! */

	int		sum;
	int		weightsum;
	int		i;
	int		t;
	int 	j;
	gt_goetcorr_match_t	corrs[GT_MAX_VECTORSIZE * GT_MAX_VECTORSIZE];
	int 	num_corrs;
	int		corr;
	int		weight;
	int 	total_weight_1, total_weight_2;
	int 	squared_weight_1, squared_weight_2;
	gt_goetcorr_t	*gc1;
	gt_goetcorr_t	*gc2;
	gt_goetcorr_t 	*gcswap; // in case reordering has to happen
	int 	intswap;
	int 	swap;
	int		known;
	int		unknown;
	int		res;
	int		itercnt;
	int     report_index;

	num_corrs = vectcnt1 * vectcnt2;

    if (verbose && report_out)
        report_index = strlen(report_out);

	/* Optimization/quick check: if both vectors consist of single value
	 * at 100% weight, just return the correlation between the GOETs, no
	 * need to bother with allocations for such "single value" GOETs. */
	if(vectcnt1 == 1 && vectcnt2 == 1 &&
	    vect1[0].gc_weight == 100 && vect2[0].gc_weight == 100) {

		corr = gt_get_goet_correlation(vect1[0].gc_id, vect2[0].gc_id);

		if(verbose) {
			gt_log("  SINGLE: %u (%d) %u",
			    vect1[0].gc_id, corr, vect2[0].gc_id);

            if (report_out) {
                report_index += snprintf(report_out + report_index,
                                         size_report - report_index,
                                         "\n  SINGLE: %u (%d) %u",
                                         vect1[0].gc_id,
                                         corr,
                                         vect2[0].gc_id);
            }
		}

		return corr;
	}

	total_weight_1 = total_weight_2 = 0;
	squared_weight_1 = squared_weight_2 = 0;
	swap = 0;
	for (i = 0; i < vectcnt1; ++i)
	{
		total_weight_1 += vect1[i].gc_weight;
		squared_weight_1 += (vect1[i].gc_weight * vect1[i].gc_weight);
	}
	for (i = 0; i < vectcnt2; ++i)
	{
		total_weight_2 += vect2[i].gc_weight;
		squared_weight_2 += (vect2[i].gc_weight * vect2[i].gc_weight);
	}
	if (total_weight_1 < total_weight_2) {
		// vect1 should have the higher weight
		swap = 1;
	}
	else if (total_weight_1 == total_weight_2) {
		// vect1 should have higher statistical dispersion
		// using average distance from the mean
		if (vectcnt1 == vectcnt2) {
			// shortcut if counts are equal: greater sum of squares
			// has higher average distance from mean
			if (squared_weight_1 < squared_weight_2)
				swap = 1;
		}
		else {
			int disp1 = (vectcnt2 * vectcnt2 * vectcnt1 * squared_weight_1) +
				((vectcnt1 * vectcnt1) - (vectcnt2 * vectcnt2)) * total_weight_1 * total_weight_1;
			int disp2 = (vectcnt2 * vectcnt1 * vectcnt1 * squared_weight_2);
			if (disp1 < disp2)
				swap = 1;
		}
	}

	if (swap == 1) {
		gcswap = vect1;
		vect1 = vect2;
		vect2 = gcswap;
		intswap = vectcnt1;
		vectcnt1 = vectcnt2;
		vectcnt2 = intswap;
		intswap = total_weight_1;
		total_weight_1 = total_weight_2;
		total_weight_2 = intswap;

	}

	/* Preload the correlation scores
	 */
	j = 0;
	for(i = 0; i < vectcnt1; ++i) {
		gc1 = &vect1[i];
		for(t = 0; t < vectcnt2; ++t) {
			gc2 = &vect2[t];
			corrs[j].gm_corr = gt_get_goet_correlation(gc1->gc_id,
			    gc2->gc_id);
			corrs[j].gm_index1 = i;
			corrs[j].gm_index2 = t;
			corrs[j].gm_order = j;
			++j;
		}
	}

	qsort((void *) corrs, num_corrs, sizeof(gt_goetcorr_match_t),
			gt_compare_goetcorrs);

	sum = weightsum = 0;
	res = 0;

	itercnt = 0;
	while (total_weight_1 && total_weight_2 && itercnt < num_corrs) {
		gc1 = &vect1[corrs[itercnt].gm_index1];
		gc2 = &vect2[corrs[itercnt].gm_index2];

		weight = (gc1->gc_weight < gc2->gc_weight) ?
			gc1->gc_weight : gc2->gc_weight;
		if (weight > 0) {
			if(verbose) {
				gt_log("  %d. %u/%d (%d) %u/%d",
				    itercnt, gc1->gc_id, gc1->gc_weight,
				    corrs[itercnt].gm_corr, gc2->gc_id, gc2->gc_weight);

	            if (report_out) {
	                report_index += snprintf(report_out + report_index,
	                                         size_report - report_index,
	                                         "\n  %d. %u/%d (%d) %u/%d",
	                                         itercnt,
	                                         gc1->gc_id,
	                                         gc1->gc_weight,
	                                         corrs[itercnt].gm_corr,
	                                         gc2->gc_id,
	                                         gc2->gc_weight);
	            }
			}

			gc1->gc_weight -= weight;
			gc2->gc_weight -= weight;
			total_weight_1 -= weight;
			total_weight_2 -= weight;

			sum += weight * corrs[itercnt].gm_corr;
			weightsum += weight;

			if(verbose) {
				gt_log("     Sum=%d, Weightsum=%d",
				    sum, weightsum);
	            if (report_out) {
	                report_index += snprintf(report_out + report_index,
	                                         size_report - report_index,
	                                         "\n     Sum=%d, Weightsum=%d",
	                                         sum,
	                                         weightsum);
	            }
			}
		}
		++itercnt;

	}

	res = 0;
	if(weightsum > 0) {
		known = sum / weightsum;

		if(verbose) {
			gt_log("   --------------------");
			gt_log("   Known = %d / %d", known, weightsum);

			if (report_out) {
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n   --------------------");
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n   Known = %d / %d", known, weightsum);
			}
		}

		if(!include_unknown_pair) {
			res = known;
		} else {
			unknown = known / 2;

			if(verbose) {
				gt_log(" Unknown = %d / %d",
				    unknown, 100 - weightsum);
                if (report_out) {
                    report_index += snprintf(report_out + report_index,
                                             size_report - report_index,
                                             "\n Unknown = %d / %d",
                                             unknown,
                                             100 - weightsum);
                }
			}

			res = (known * weightsum + unknown * (100 - weightsum)) / 100;
		}

		if(verbose) {
			gt_log("   ====================");
			gt_log("   Result = %d", res);

			if (report_out) {
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n   ====================");
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n   Result = %d", res);
			}
		}
	}

	return res;
}


#define GT_A2A_WEIGHT_GENRE		60
#define GT_A2A_WEIGHT_ERA		20
#define GT_A2A_WEIGHT_ORIGIN		10
#define GT_A2A_WEIGHT_ARTTYPE		10
/* For bbm
#define GT_A2A_WEIGHT_ERA		10
#define GT_A2A_WEIGHT_ORIGIN		10
#define GT_A2A_WEIGHT_ARTTYPE		20
*/



#define GT_A2A_STEP1_GENRE_CUTOFF	350

int
gt_compute_artist_similarity_prim(gt_artist_t *art1, gt_artist_t *art2,
    int flags, int verbose, gt_similarity_t *sim, char *report_out, size_t size_report)
{
    gt_sim_params_t params;

    params.genre_cutoff = GT_A2A_STEP1_GENRE_CUTOFF;
    params.genre_weight = GT_A2A_WEIGHT_GENRE;
    params.era_weight = GT_A2A_WEIGHT_ERA;
    params.origin_weight = GT_A2A_WEIGHT_ORIGIN;
    params.atype_weight = GT_A2A_WEIGHT_ARTTYPE;

    return gt_compute_artist_similarity_prim_params(art1,
                                                    art2,
                                                    flags,
                                                    verbose,
                                                    &params,
                                                    sim,
                                                    report_out,
                                                    size_report);
}

int gt_compute_artist_similarity_prim_params(gt_artist_t *art1,
                                             gt_artist_t *art2,
                                             int flags,
                                             int verbose,
                                             const gt_sim_params_t *params,
                                             gt_similarity_t *sim,
                                             char *report_out,
                                             size_t size_report)
{
	/* Fills in the passed in similarity struct. */

	int		gid1;
	int		gid2;
	gt_goetcorr_t	goetvect1[GT_MAX_VECTORSIZE];
	int		goetvectcnt1;
	gt_goetcorr_t	goetvect2[GT_MAX_VECTORSIZE];
	int		goetvectcnt2;
	int		sum;
	int		corr;
	int     report_index;

	if(art1 == NULL || art2 == NULL || sim == NULL) {
		gt_log("Missing argument!");
		return EINVAL;
	}

	memset(sim, 0, sizeof(gt_similarity_t));
	sum = 0;

	if(verbose) {
		gt_log("");
		if (report_out) {
            report_index = 0;
            memset(report_out, 0, size_report);
		}
	}

	/* Step1: Primary genre cutoff. */
	gid1 = gt_get_artist_primary_goet_value(art1, GT_TYPE_GENRE);
	gid2 = gt_get_artist_primary_goet_value(art2, GT_TYPE_GENRE);
	corr = gt_get_goet_correlation(gid1, gid2);

	if(corr < params->genre_cutoff) {
		sim->gs_step1_pass = 0;

		if(verbose) {
			gt_log("STEP 1: %d (%d) %d < %d -> FAIL",
			    gid1, corr, gid2, params->genre_cutoff);

            if (report_out) {
                report_index += snprintf(report_out,
                                         size_report,
                                         "STEP 1: %d (%d) %d < %d -> FAIL\n",
                                         gid1,
                                         corr,
                                         gid2,
                                         params->genre_cutoff);
            }

		}

		if(!(flags & GT_SIMFLAG_NO_EARLY_BAILOUT))
			return 0;
	} else {
		if(verbose) {
			gt_log("STEP 1: %d (%d) %d >= %d -> PASS",
			    gid1, corr, gid2, params->genre_cutoff);
            if (report_out) {
                report_index += snprintf(report_out,
                                         size_report,
                                         "STEP 1: %d (%d) %d >= %d -> PASS",
                                         gid1,
                                         corr,
                                         gid2,
                                         params->genre_cutoff);
            }
		}

		sim->gs_step1_pass = 1;
	}

	if(verbose) {
		gt_log("");
		gt_log("STEP 2: (Genre=%d Era=%d Origin=%d"
		    " Artist Type=%d)\n",
		    params->genre_weight, params->era_weight,
		    params->origin_weight, params->atype_weight);

        if (report_out) {
            report_index += snprintf(report_out + report_index, size_report - report_index, "\n\nSTEP 2: (Genre=%d Era=%d Origin=%d"
		    " Artist Type=%d)\n",
		    params->genre_weight,
		    params->era_weight,
		    params->origin_weight,
		    params->atype_weight);
        }
	}

	/* Genre. */
	if(!(flags & GT_SIMFLAG_PRIMARY_GENRE_ONLY)) {
		/* Use full genre pie (default). */
		goetvectcnt1 = gt_get_artist_goet_vector(art1, GT_TYPE_GENRE,
		    goetvect1, GT_MAX_VECTORSIZE);
		goetvectcnt2 = gt_get_artist_goet_vector(art2, GT_TYPE_GENRE,
		    goetvect2, GT_MAX_VECTORSIZE);
		if(goetvectcnt1 <= 0 || goetvectcnt2 <= 0)
			sim->gs_score_genre = 0;
		else {
			if(verbose) {
				gt_log("Computing GENRE similarity:");
				if (report_out) {
                    report_index += snprintf(report_out + report_index, size_report - report_index, "\nComputing GENRE similarity:");
				}
			}

			sim->gs_score_genre = gt_compute_vector_similarity(
			    goetvect1, goetvectcnt1, goetvect2, goetvectcnt2, 0,
			    verbose, report_out, size_report);
			if(verbose) {
				gt_log("");
				if (report_out) {
                    report_index = strlen(report_out);
                    report_index += snprintf(report_out + report_index, size_report - report_index , "\n");
				}
			}
			/* Only account for genre weight if present for both. */
			sum += params->genre_weight;
		}
	} else {
		/* We're instructed to only use the primary genre. */
		gid1 = gt_get_artist_primary_goet_value(art1, GT_TYPE_GENRE);
		gid2 = gt_get_artist_primary_goet_value(art2, GT_TYPE_GENRE);
		if(gid1 && gid2) {
			corr = gt_get_goet_correlation(gid1, gid2);
			sim->gs_score_genre = corr;
			if(verbose) {
				gt_log("  GENRE (PRIMARY_ONLY): %d (%d) %d\n",
				    gid1, corr, gid2);
                if (report_out) {
                    report_index += snprintf(report_out + report_index, size_report - report_index, "\n  GENRE (PRIMARY_ONLY): %d (%d) %d\n",
                                             gid1,
                                             corr,
                                             gid2);
                }
			}

			/* Only account for genre weight if present for both. */
			sum += params->genre_weight;
		} else {
			if(verbose) {
				gt_log("No primary GENRE(s)!\n");
				if (report_out) {
                    report_index += snprintf(report_out + report_index, size_report - report_index, "\nNo primary GENRE(s)!\n");
				}
			}

			sim->gs_score_genre = 0;
		}
	}

	/* Mood is N/A for artists. */
	sim->gs_score_mood = 0;

	/* Era. */
	goetvectcnt1 = gt_get_artist_goet_vector(art1, GT_TYPE_ERA, goetvect1,
	    GT_MAX_VECTORSIZE);
	goetvectcnt2 = gt_get_artist_goet_vector(art2, GT_TYPE_ERA, goetvect2,
	    GT_MAX_VECTORSIZE);
	if(goetvectcnt1 <= 0 || goetvectcnt2 <= 0)
		sim->gs_score_era = 0;
	else {
		if(verbose) {
			gt_log("Computing ERA similarity:");
			if (report_out) {
                report_index += snprintf(report_out + report_index, size_report - report_index, "\nComputing ERA similarity:");
			}
		}

		sim->gs_score_era = gt_compute_vector_similarity(
		    goetvect1, goetvectcnt1, goetvect2, goetvectcnt2, 0,
		    verbose, report_out, size_report);
		if(verbose) {
			gt_log("");
			if (report_out) {
                report_index = strlen(report_out);
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n");
			}
		}
		/* Only account for era weight if present for both. */
		sum += params->era_weight;
	}

	/* Origin. */
	goetvectcnt1 = gt_get_artist_goet_vector(art1, GT_TYPE_ORIGIN,
	    goetvect1, GT_MAX_VECTORSIZE);
	goetvectcnt2 = gt_get_artist_goet_vector(art2, GT_TYPE_ORIGIN,
	    goetvect2, GT_MAX_VECTORSIZE);
	if(goetvectcnt1 <= 0 || goetvectcnt2 <= 0)
		sim->gs_score_origin = 0;
	else {
		if(verbose) {
			gt_log("Computing ORIGIN similarity:");
			if (report_out) {
                report_index += snprintf(report_out + report_index, size_report - report_index, "\nComputing ORIGIN similarity:");
			}
		}
		sim->gs_score_origin = gt_compute_vector_similarity(
		    goetvect1, goetvectcnt1, goetvect2, goetvectcnt2, 0,
		    verbose, report_out, size_report);
		if(verbose) {
			gt_log("");
			if (report_out) {
                report_index = strlen(report_out);
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n");
			}
		}
		/* Only account for origin weight if present for both. */
		sum += params->origin_weight;
	}

	/* Artist type. */
	goetvectcnt1 = gt_get_artist_goet_vector(art1, GT_TYPE_ATYPE,
	    goetvect1, GT_MAX_VECTORSIZE);
	goetvectcnt2 = gt_get_artist_goet_vector(art2, GT_TYPE_ATYPE,
	    goetvect2, GT_MAX_VECTORSIZE);
	if(goetvectcnt1 <= 0 || goetvectcnt2 <= 0)
		sim->gs_score_artist_type = 0;
	else {
		if(verbose) {
			gt_log("Computing ARTIST TYPE similarity:");
			if (report_out) {
                report_index += snprintf(report_out + report_index, size_report - report_index, "\nComputing ARTIST TYPE similarity:");
			}
		}
		sim->gs_score_artist_type = gt_compute_vector_similarity(
		    goetvect1, goetvectcnt1, goetvect2, goetvectcnt2, 0,
		    verbose, report_out, size_report);
		if(verbose) {
			gt_log("");
			if (report_out) {
                report_index = strlen(report_out);
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n");
			}
		}
		/* Only account for artist type weight if present for both. */
		sum += params->atype_weight;
	}

	/* Overall score is a weighted sum of the components. */
	sim->gs_score_weighted = (
	    sim->gs_score_genre * params->genre_weight +
	    sim->gs_score_era * params->era_weight +
	    sim->gs_score_origin * params->origin_weight +
	    sim->gs_score_artist_type * params->atype_weight) / 100;
	   // sim->gs_score_artist_type * GT_A2A_WEIGHT_ARTTYPE) / sum;

	if(sim->gs_step1_pass)
		sim->gs_similarity_value = sim->gs_score_weighted;

	return 0;
}


int
gt_compute_artist_similarity(int id1, int id2, int flags, int verbose,
    gt_similarity_t *sim, char *report_out, size_t size_report)
{
	gt_artist_t	*art1;
	gt_artist_t	*art2;
	int		ret;

	ret = gt_lookup_artist(id1, &art1);
	if(ret != 0)
		return ret;

	ret = gt_lookup_artist(id2, &art2);
	if(ret != 0)
		return ret;

	return gt_compute_artist_similarity_prim(art1, art2, flags,
	    verbose, sim, report_out, size_report);
}


#define GT_A2T_WEIGHT_GENRE		60
#define GT_A2T_WEIGHT_ERA		20
#define GT_A2T_WEIGHT_ORIGIN		10
#define GT_A2T_WEIGHT_ARTTYPE		10

#define GT_A2T_STEP1_GENRE_CUTOFF	500

int
gt_compute_artist_to_track_similarity_prim(gt_artist_t *art, gt_track_t *trk,
    int flags, int verbose, gt_similarity_t *sim, char *report_out, size_t size_report)
{
    gt_sim_params_t params;

    params.genre_cutoff = GT_A2T_STEP1_GENRE_CUTOFF;
    params.genre_weight = GT_A2T_WEIGHT_GENRE;
    params.era_weight = GT_A2T_WEIGHT_ERA;
    params.origin_weight = GT_A2T_WEIGHT_ORIGIN;
    params.atype_weight = GT_A2T_WEIGHT_ARTTYPE;

    return gt_compute_artist_to_track_similarity_prim_params(art,
                                                             trk,
                                                             flags,
                                                             verbose,
                                                             &params,
                                                             sim,
                                                             report_out,
                                                             size_report);
}

int
gt_compute_artist_to_track_similarity_prim_params(gt_artist_t *art,
                                                        gt_track_t *trk,
                                                        int flags,
                                                        int verbose,
                                                        const gt_sim_params_t *params,
                                                        gt_similarity_t *sim,
                                                        char *report_out,
                                                        size_t size_report)
{
	/* Fills in the passed in similarity struct. */

	int		gid1;
	int		gid2;
	gt_goetcorr_t	goetvect1[GT_MAX_VECTORSIZE];
	int		goetvectcnt1;
	gt_goetcorr_t	goetvect2[GT_MAX_VECTORSIZE];
	int		goetvectcnt2;
	int		sum;
	int		corr;
	int     report_index;

	if(art == NULL || trk == NULL || sim == NULL) {
		gt_log("Missing argument!");
		return EINVAL;
	}

	memset(sim, 0, sizeof(gt_similarity_t));
	sum = 0;

	if(verbose) {
		gt_log("");
		if (report_out) {
		    report_index = 0;
            memset(report_out, 0, size_report);
		}
	}

	/* Step1: Primary genre cutoff. */
	gid1 = gt_get_artist_primary_goet_value(art, GT_TYPE_GENRE);
	gid2 = gt_get_track_primary_goet_value(trk, GT_TYPE_GENRE);
	corr = gt_get_goet_correlation(gid1, gid2);

	if(corr < params->genre_cutoff) {
		sim->gs_step1_pass = 0;

		if(verbose) {
			gt_log("STEP 1: %d (%d) %d < %d -> FAIL",
			    gid1, corr, gid2, params->genre_cutoff);

            if (report_out) {
                report_index += snprintf(report_out,
                                         size_report,
                                         "STEP 1: %d (%d) %d < %d -> FAIL\n",
                                         gid1,
                                         corr,
                                         gid2,
                                         params->genre_cutoff);
            }
		}

		if(!(flags & GT_SIMFLAG_NO_EARLY_BAILOUT))
			return 0;
	} else {
		if(verbose) {
			gt_log("STEP 1: %d (%d) %d >= %d -> PASS",
			    gid1, corr, gid2, params->genre_cutoff);

            if (report_out) {
                report_index += snprintf(report_out,
                                         size_report,
                                         "STEP 1: %d (%d) %d >= %d -> PASS",
                                         gid1,
                                         corr,
                                         gid2,
                                         params->genre_cutoff);
            }
		}

		sim->gs_step1_pass = 1;
	}

	if(verbose) {
		gt_log("");
		gt_log("STEP 2: (Genre=%d Era=%d Origin=%d"
		    " Artist Type=%d)\n",
		    params->genre_weight, params->era_weight,
		    params->origin_weight, params->atype_weight);

        if (report_out) {
            report_index += snprintf(report_out + report_index,
                                     size_report - report_index,
                                     "\n\nSTEP 2: (Genre=%d Era=%d Origin=%d"
                                     " Artist Type=%d)\n",
                                     params->genre_weight,
                                     params->era_weight,
                                     params->origin_weight,
                                     params->atype_weight);
        }
	}

	/* Genre. */
	if(!(flags & GT_SIMFLAG_PRIMARY_GENRE_ONLY)) {
		/* Use full genre pie (default). */
		goetvectcnt1 = gt_get_artist_goet_vector(art, GT_TYPE_GENRE,
		    goetvect1, GT_MAX_VECTORSIZE);
		goetvectcnt2 = gt_get_track_goet_vector(trk, GT_TYPE_GENRE,
		    goetvect2, GT_MAX_VECTORSIZE);
		if(goetvectcnt1 <= 0 || goetvectcnt2 <= 0)
			sim->gs_score_genre = 0;
		else {
			if(verbose) {
				gt_log("Computing GENRE similarity:");
				if (report_out) {
                    report_index += snprintf(report_out + report_index, size_report - report_index, "\nComputing GENRE similarity:");
				}
			}

			sim->gs_score_genre = gt_compute_vector_similarity(
			    goetvect1, goetvectcnt1, goetvect2, goetvectcnt2, 0,
			    verbose, report_out, size_report);
			if(verbose) {
				gt_log("");
                if (report_out) {
                    report_index = strlen(report_out);
                    report_index += snprintf(report_out + report_index, size_report - report_index, "\n");
                }
			}
			/* Only account for genre weight if present for both. */
			sum += params->genre_weight;
		}
	} else {
		/* We're instructed to only use the primary genre. */
		gid1 = gt_get_artist_primary_goet_value(art, GT_TYPE_GENRE);
		gid2 = gt_get_track_primary_goet_value(trk, GT_TYPE_GENRE);
		if(gid1 && gid2) {
			corr = gt_get_goet_correlation(gid1, gid2);
			sim->gs_score_genre = corr;
			if(verbose) {
				gt_log("  GENRE (PRIMARY_ONLY): %d (%d) %d\n",
				    gid1, corr, gid2);

                if (report_out) {
                    report_index += snprintf(report_out + report_index,
                                             size_report - report_index,
                                             "\n  GENRE (PRIMARY_ONLY): %d (%d) %d\n",
                                             gid1,
                                             corr,
                                             gid2);
                }
			}
			/* Only account for genre weight if present for both. */
			sum += params->genre_weight;
		} else {
			if(verbose) {
				gt_log("No primary GENRE(s)!\n");
				if (report_out) {
                    report_index += snprintf(report_out + report_index, size_report - report_index, "\nNo primary GENRE(s)!\n");
				}
			}
			sim->gs_score_genre = 0;
		}
	}

	/* Mood is N/A for artists. */
	sim->gs_score_mood = 0;

	/* Era. */
	goetvectcnt1 = gt_get_artist_goet_vector(art, GT_TYPE_ERA, goetvect1,
	    GT_MAX_VECTORSIZE);
	goetvectcnt2 = gt_get_track_goet_vector(trk, GT_TYPE_ERA, goetvect2,
	    GT_MAX_VECTORSIZE);
	if(goetvectcnt1 <= 0 || goetvectcnt2 <= 0)
		sim->gs_score_era = 0;
	else {
		if(verbose) {
			gt_log("Computing ERA similarity:");
			if (report_out) {
                report_index += snprintf(report_out + report_index, size_report - report_index, "\nComputing ERA similarity:");
			}
		}
		sim->gs_score_era = gt_compute_vector_similarity(
		    goetvect1, goetvectcnt1, goetvect2, goetvectcnt2, 0,
		    verbose, report_out, size_report);
		if(verbose) {
			gt_log("");
			if (report_out) {
                report_index = strlen(report_out);
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n");
			}
		}
		/* Only account for era weight if present for both. */
		sum += params->era_weight;
	}

	/* Origin. */
	goetvectcnt1 = gt_get_artist_goet_vector(art, GT_TYPE_ORIGIN,
	    goetvect1, GT_MAX_VECTORSIZE);
	goetvectcnt2 = gt_get_track_goet_vector(trk, GT_TYPE_ORIGIN,
	    goetvect2, GT_MAX_VECTORSIZE);
	if(goetvectcnt1 <= 0 || goetvectcnt2 <= 0)
		sim->gs_score_origin = 0;
	else {
		if(verbose) {
			gt_log("Computing ORIGIN similarity:");

			if (report_out) {
                report_index += snprintf(report_out + report_index, size_report - report_index, "\nComputing ORIGIN similarity:");
			}
		}
		sim->gs_score_origin = gt_compute_vector_similarity(
		    goetvect1, goetvectcnt1, goetvect2, goetvectcnt2, 0,
		    verbose, report_out, size_report);
		if(verbose) {
			gt_log("");
			if (report_out) {
                report_index = strlen(report_out);
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n");
			}
		}
		/* Only account for origin weight if present for both. */
		sum += params->origin_weight;
	}

	/* Artist type. */
	goetvectcnt1 = gt_get_artist_goet_vector(art, GT_TYPE_ATYPE,
	    goetvect1, GT_MAX_VECTORSIZE);
	goetvectcnt2 = gt_get_track_goet_vector(trk, GT_TYPE_ATYPE,
	    goetvect2, GT_MAX_VECTORSIZE);
	if(goetvectcnt1 <= 0 || goetvectcnt2 <= 0)
		sim->gs_score_artist_type = 0;
	else {
		if(verbose) {
			gt_log("Computing ARTIST TYPE similarity:");
			if (report_out) {
                report_index += snprintf(report_out + report_index, size_report - report_index, "\nComputing ARTIST TYPE similarity:");
			}
		}
		sim->gs_score_artist_type = gt_compute_vector_similarity(
		    goetvect1, goetvectcnt1, goetvect2, goetvectcnt2, 0,
		    verbose, report_out, size_report);
		if(verbose) {
			gt_log("");
			if (report_out) {
                report_index = strlen(report_out);
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n");
			}
		}
		/* Only account for artist type weight if present for both. */
		sum += params->atype_weight;
	}

	/* Overall score is a weighted sum of the components. */
	sim->gs_score_weighted = (
	    sim->gs_score_genre * params->genre_weight +
	    sim->gs_score_era * params->era_weight +
	    sim->gs_score_origin * params->origin_weight +
	    sim->gs_score_artist_type * params->atype_weight) / 100;
	   // sim->gs_score_artist_type * GT_A2A_WEIGHT_ARTTYPE) / sum;

	if(sim->gs_step1_pass)
		sim->gs_similarity_value = sim->gs_score_weighted;

	return 0;
}


int
gt_compute_artist_to_track_similarity(int artid, int trkid, int flags,
    int verbose, gt_similarity_t *sim, char *report_out, size_t size_report)
{
	gt_artist_t	*art;
	gt_track_t	*trk;
	int		ret;

	ret = gt_lookup_artist(artid, &art);
	if(ret != 0)
		return ret;

	ret = gt_lookup_track(trkid, &trk);
	if(ret != 0)
		return ret;

	return gt_compute_artist_to_track_similarity_prim(art, trk, flags,
	    verbose, sim, report_out, size_report);
}



#define GT_T2T_WEIGHT_GENRE		40
#define GT_T2T_WEIGHT_MOOD		50
#define GT_T2T_WEIGHT_ERA		5
#define GT_T2T_WEIGHT_ORIGIN		3
#define GT_T2T_WEIGHT_ARTTYPE		2

#define GT_T2T_STEP1_GENRE_CUTOFF	350
#define GT_T2T_STEP1_MOOD_CUTOFF	-160

int
gt_compute_track_similarity_prim(gt_track_t *trk1, gt_track_t *trk2,
    int flags, int verbose, gt_similarity_t *sim, char *report_out, size_t size_report)
{
    gt_sim_params_t params;

    params.genre_cutoff = GT_T2T_STEP1_GENRE_CUTOFF;
    params.mood_cutoff = GT_T2T_STEP1_MOOD_CUTOFF;

    params.genre_weight = GT_T2T_WEIGHT_GENRE;
    params.mood_weight = GT_T2T_WEIGHT_MOOD;
    params.era_weight = GT_T2T_WEIGHT_ERA;
    params.origin_weight = GT_T2T_WEIGHT_ORIGIN;
    params.atype_weight = GT_T2T_WEIGHT_ARTTYPE;

    return gt_compute_track_similarity_prim_params(trk1,
                                                   trk2,
                                                   flags,
                                                   verbose,
                                                   &params,
                                                   sim,
                                                   report_out,
                                                   size_report);
}

int gt_compute_track_similarity_prim_params(gt_track_t *trk1,
                                            gt_track_t *trk2,
                                            int flags,
                                            int verbose,
                                            const gt_sim_params_t *params,
                                            gt_similarity_t *sim,
                                            char *report_out,
                                            size_t size_report)
{
	/* Fills in the passed in similarity struct. */

	int		gid1;
	int		gid2;
	gt_goetcorr_t	goetvect1[GT_MAX_VECTORSIZE];
	int		goetvectcnt1;
	gt_goetcorr_t	goetvect2[GT_MAX_VECTORSIZE];
	int		goetvectcnt2;
	int		sum;
	int		corr;
	int     report_index;

	if(trk1 == NULL || trk2 == NULL || sim == NULL) {
		gt_log("Missing argument!");
		return EINVAL;
	}

	memset(sim, 0, sizeof(gt_similarity_t));
	sum = 0;

	if(verbose) {
		gt_log("");
		if (report_out) {
            report_index = 0;
            memset(report_out, 0, size_report);
		}
	}

	/* Step1: Primary genre cutoff. */
	gid1 = gt_get_track_primary_goet_value(trk1, GT_TYPE_GENRE);
	gid2 = gt_get_track_primary_goet_value(trk2, GT_TYPE_GENRE);
	corr = gt_get_goet_correlation(gid1, gid2);
	if(corr < params->genre_cutoff) {
		sim->gs_step1_pass = 0;

		if(verbose) {
			gt_log("STEP 1: %d (%d) %d < %d -> FAIL",
			    gid1, corr, gid2, params->genre_cutoff);

            if (report_out) {
                report_index += snprintf(report_out,
                                         size_report,
                                         "STEP 1: %d (%d) %d < %d -> FAIL\n",
                                         gid1,
                                         corr,
                                         gid2,
                                         params->genre_cutoff);
            }
		}

		if(!(flags & GT_SIMFLAG_NO_EARLY_BAILOUT))
			return 0;

	} else {

		if(verbose) {
			gt_log("STEP 1: %d (%d) %d >= %d -> Compare MOOD",
			    gid1, corr, gid2, params->genre_cutoff);

            if (report_out) {
                report_index += snprintf(report_out,
                                         size_report,
                                         "STEP 1: %d (%d) %d >= %d -> Compare MOOD",
                                         gid1,
                                         corr,
                                         gid2,
                                         params->genre_cutoff);
            }
		}

		/* Step1: Primary mood cutoff. */
		gid1 = gt_get_track_primary_goet_value(trk1, GT_TYPE_MOOD);
		gid2 = gt_get_track_primary_goet_value(trk2, GT_TYPE_MOOD);
		if(gid1 && gid2) {
			corr = gt_get_goet_correlation(gid1, gid2);

            if(corr < params->mood_cutoff) {
				sim->gs_step1_pass = 0;

				if(verbose) {
					gt_log(
					    "STEP 1: %d (%d) %d < %d -> FAIL",
					    gid1, corr, gid2,
					    params->mood_cutoff);
                    if (report_out) {
                        report_index += snprintf(report_out + report_index,
                                                 size_report - report_index,
                                                 "\nSTEP 1: %d (%d) %d < %d -> FAIL\n",
                                                 gid1,
                                                 corr,
                                                 gid2,
                                                 params->mood_cutoff);
                    }
				}

				if(!(flags & GT_SIMFLAG_NO_EARLY_BAILOUT))
					return 0;
			} else {
				sim->gs_step1_pass = 1;

				if(verbose) {
					gt_log(
					    "STEP 1: %d (%d) %d >= %d -> PASS",
					    gid1, corr, gid2, params->mood_cutoff);

                    if (report_out) {
                        report_index += snprintf(report_out + report_index,
                                                 size_report - report_index,
                                                 "\nSTEP 1: %d (%d) %d >= %d -> PASS",
                                                 gid1,
                                                 corr,
                                                 gid2,
                                                 params->mood_cutoff);
                    }
				}
			}
		}
	}

	if(verbose) {
		gt_log("");
		gt_log("STEP 2: (Genre=%d Mood=%d Era=%d Origin=%d"
		    " Artist Type=%d)\n",
		    params->genre_weight, params->mood_weight, params->era_weight,
		    params->origin_weight, params->atype_weight);

        if (report_out) {
            report_index += snprintf(report_out + report_index,
                                     size_report - report_index,
                                     "\n\nSTEP 2: (Genre=%d Mood=%d Era=%d Origin=%d"
                                     " Artist Type=%d)\n",
                                     params->genre_weight,
                                     params->mood_weight,
                                     params->era_weight,
                                     params->origin_weight,
                                     params->atype_weight);
        }
	}

	/* Genre. */
	goetvectcnt1 = gt_get_track_goet_vector(trk1, GT_TYPE_GENRE, goetvect1,
	    GT_MAX_VECTORSIZE);
	goetvectcnt2 = gt_get_track_goet_vector(trk2, GT_TYPE_GENRE, goetvect2,
	    GT_MAX_VECTORSIZE);
	if(goetvectcnt1 <= 0 || goetvectcnt2 <= 0)
		sim->gs_score_genre = 0;
	else {
		if(verbose) {
			gt_log("Computing GENRE similarity:");
			if (report_out) {
                report_index += snprintf(report_out + report_index, size_report - report_index, "\nComputing GENRE similarity:");
			}
		}
		sim->gs_score_genre = gt_compute_vector_similarity(
		    goetvect1, goetvectcnt1, goetvect2, goetvectcnt2, 0,
		    verbose, report_out, size_report);
		if(verbose) {
			gt_log("");
			if (report_out) {
                report_index = strlen(report_out);
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n");
			}
		}
		/* Only account for genre weight if present for both. */
		sum += params->genre_weight;
	}

	/* Mood. */
	goetvectcnt1 = gt_get_track_goet_vector(trk1, GT_TYPE_MOOD, goetvect1,
	    GT_MAX_VECTORSIZE);
	goetvectcnt2 = gt_get_track_goet_vector(trk2, GT_TYPE_MOOD, goetvect2,
	    GT_MAX_VECTORSIZE);
	if(goetvectcnt1 <= 0 || goetvectcnt2 <= 0)
		sim->gs_score_mood = DEFAULT_MOOD_SCORE;
	else {
		if(verbose) {
			gt_log("Computing MOOD similarity:");
			if (report_out) {
                report_index += snprintf(report_out + report_index, size_report - report_index, "\nComputing MOOD similarity:");
			}
		}
		sim->gs_score_mood = gt_compute_vector_similarity(
		    goetvect1, goetvectcnt1, goetvect2, goetvectcnt2, 1,
		    verbose, report_out, size_report);
		if(verbose) {
			gt_log("");
			if (report_out) {
                report_index = strlen(report_out);
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n");
			}
		}
		/* Only account for mood weight if present for both. */
		sum += params->mood_weight;
	}

	/* Era. */
	goetvectcnt1 = gt_get_track_goet_vector(trk1, GT_TYPE_ERA, goetvect1,
	    GT_MAX_VECTORSIZE);
	goetvectcnt2 = gt_get_track_goet_vector(trk2, GT_TYPE_ERA, goetvect2,
	    GT_MAX_VECTORSIZE);
	if(goetvectcnt1 <= 0 || goetvectcnt2 <= 0)
		sim->gs_score_era = 0;
	else {
		if(verbose) {
			gt_log("Computing ERA similarity:");
			if (report_out) {
                report_index += snprintf(report_out + report_index, size_report - report_index, "\nComputing ERA similarity:");
			}
		}
		sim->gs_score_era = gt_compute_vector_similarity(
		    goetvect1, goetvectcnt1, goetvect2, goetvectcnt2, 0,
		    verbose, report_out, size_report);
		if(verbose) {
			gt_log("");
			if (report_out) {
                report_index = strlen(report_out);
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n");
			}
		}
		/* Only account for era weight if present for both. */
		sum += params->era_weight;
	}


	/* Origin. */
	goetvectcnt1 = gt_get_track_goet_vector(trk1, GT_TYPE_ORIGIN,
	    goetvect1, GT_MAX_VECTORSIZE);
	goetvectcnt2 = gt_get_track_goet_vector(trk2, GT_TYPE_ORIGIN,
	    goetvect2, GT_MAX_VECTORSIZE);
	if(goetvectcnt1 <= 0 || goetvectcnt2 <= 0)
		sim->gs_score_origin = 0;
	else {
		if(verbose) {
			gt_log("Computing ORIGIN similarity:");
			if (report_out) {
                report_index += snprintf(report_out + report_index, size_report - report_index, "\nComputing ORIGIN similarity:");
			}
		}
		sim->gs_score_origin = gt_compute_vector_similarity(
		    goetvect1, goetvectcnt1, goetvect2, goetvectcnt2, 0,
		    verbose, report_out, size_report);
		if(verbose) {
			gt_log("");
			if (report_out) {
                report_index = strlen(report_out);
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n");
			}
		}
		/* Only account for origin weight if present for both. */
		sum += params->origin_weight;
	}

	/* Artist type. */
	goetvectcnt1 = gt_get_track_goet_vector(trk1, GT_TYPE_ATYPE,
	    goetvect1, GT_MAX_VECTORSIZE);
	goetvectcnt2 = gt_get_track_goet_vector(trk2, GT_TYPE_ATYPE,
	    goetvect2, GT_MAX_VECTORSIZE);
	if(goetvectcnt1 <= 0 || goetvectcnt2 <= 0)
		sim->gs_score_artist_type = 0;
	else {
		if(verbose) {
			gt_log("Computing ARTIST TYPE similarity:");
			if (report_out) {
                report_index += snprintf(report_out + report_index, size_report - report_index, "\nComputing ARTIST TYPE similarity:");
			}
		}
		sim->gs_score_artist_type = gt_compute_vector_similarity(
		    goetvect1, goetvectcnt1, goetvect2, goetvectcnt2, 0,
		    verbose, report_out, size_report);
		if(verbose) {
			gt_log("");
			if (report_out) {
                report_index = strlen(report_out);
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n");
			}
		}
		/* Only account for artist type weight if present for both. */
		sum += params->atype_weight;
	}

	/* Overall score is a weighted sum of the components. */
	sim->gs_score_weighted = (
	    sim->gs_score_genre * params->genre_weight +
	    sim->gs_score_mood * params->mood_weight +
	    sim->gs_score_era * params->era_weight +
	    sim->gs_score_origin * params->origin_weight +
	    sim->gs_score_artist_type * params->atype_weight) / sum;

	if(sim->gs_step1_pass)
		sim->gs_similarity_value = sim->gs_score_weighted;

	return 0;
}


int
gt_compute_track_similarity(int id1, int id2, int flags, int verbose,
    gt_similarity_t *sim, char *report_out, size_t size_report)
{
	gt_track_t	*trk1;
	gt_track_t	*trk2;
	int		ret;

	ret = gt_lookup_track(id1, &trk1);
	if(ret != 0)
		return ret;

	ret = gt_lookup_track(id2, &trk2);
	if(ret != 0)
		return ret;

	return gt_compute_track_similarity_prim(trk1, trk2, flags,
	    verbose, sim, report_out, size_report);
}


#define GT_TOP_10K_ARTIST_POP	148668

int
gt_artist_nxn(int threshold)
{
	gt_artist_t	*artist;
	gt_artist_t	*artist2;
	int		i;
	int		t;
	gt_similarity_t	sim;

	for(i = 0; i < gt_artists_cnt; ++i) {
		artist = &gt_artists[i];
		if(artist->ga_pop < GT_TOP_10K_ARTIST_POP)
			continue;
		printf("%s (%d)\n", artist->ga_name, artist->ga_id);
		for(t = 0; t < gt_artists_cnt; ++t) {
			if(t == i)
				continue;
			artist2 = &gt_artists[t];
			if(artist2->ga_pop< GT_TOP_10K_ARTIST_POP)
				continue;
			gt_compute_artist_similarity_prim(artist, artist2, 0, 0,
			    &sim, NULL, 0);
			if(sim.gs_similarity_value >= threshold) {
				printf(" %d %s (%d)\n", sim.gs_similarity_value,
				    artist2->ga_name, artist2->ga_id);
			}
		}

		printf("\n");
	}

	return 0;
}


int
goetcorr_index_add(unsigned id1, unsigned id2, int corr)
{
	unsigned		bucketnum;
	gt_goetcorr_ent_t	*bucket;
	gt_goetcorr_ent_t	*newent;
	int			depth;

	newent = calloc(sizeof(gt_goetcorr_ent_t), 1);
	if(newent == NULL) {
		gt_log("Can't allocate new correlate index entry!");
		return ENOENT;
	}

	newent->ge_key1 = id1;
	newent->ge_key2 = id2;
	newent->ge_corr = corr;
	newent->ge_next = NULL;

	depth = 0;
	bucketnum = (id1 * 1000 + id2) % GT_GOETCORRIDX_BUCKETS;
	if(gt_gci.gi_buckets[bucketnum] == NULL) {
		gt_gci.gi_buckets[bucketnum] = newent;
	} else {
		bucket = gt_gci.gi_buckets[bucketnum];
		while(bucket->ge_next) {
			++depth;
			bucket = bucket->ge_next;
		}
		bucket->ge_next = newent;
	}

	++depth;
	if(depth > gt_gci.gi_maxdepth) {
		gt_gci.gi_maxdepth = depth;
	}

	return 0;
}


int
goetcorr_index_get(unsigned id1, unsigned id2)
{
	unsigned		bucketnum;
	gt_goetcorr_ent_t	*ent;

	bucketnum = (id1 * 1000 + id2) % GT_GOETCORRIDX_BUCKETS;
	if(gt_gci.gi_buckets[bucketnum] == NULL) {
		return 0;
	} else {
		ent = gt_gci.gi_buckets[bucketnum];
		while(ent && (ent->ge_key1 != id1 || ent->ge_key2 != id2))
			ent = ent->ge_next;

		if(ent)
			return ent->ge_corr;
	}

	return 0;
}


int
gt_get_rank_unit()
{
	return gt_rank_unit;
}



#define GT_ARTIST_TRACKS_INITSIZE     32
#define GT_ARTIST_TRACKS_GROW         16

int
add_artist_track(gt_artist_t *art, gt_track_t *trk)
{
	unsigned	*newtracklist;

	if(art->ga_tracks == NULL) {
		/* First track for artist. */
		art->ga_tracks = malloc(GT_ARTIST_TRACKS_INITSIZE *
		    sizeof(unsigned));
		if(art->ga_tracks == NULL) {
			gt_log("Can't allocate artist track list!");
			return ENOMEM;
		}
		art->ga_tracks_size = GT_ARTIST_TRACKS_INITSIZE;
	} else
	if(art->ga_tracks_size == art->ga_tracks_cnt) {
		/* Need to grow track list. */
		newtracklist = realloc(art->ga_tracks,
		    (art->ga_tracks_size + GT_ARTIST_TRACKS_GROW) *
                    sizeof(unsigned));
		if(newtracklist == NULL) {
			gt_log("Can't increase size of artist track list!");
			return ENOMEM;
		}
		art->ga_tracks = newtracklist;
		art->ga_tracks_size += GT_ARTIST_TRACKS_GROW;
	}

	art->ga_tracks[art->ga_tracks_cnt] = trk->gt_id;
	art->ga_tracks_cnt++;

	return 0;
}


int
gt_add_track_links()
{
	gt_track_t	*trk;
	gt_track_t	*end;
	gt_artist_t	*art;
	gt_artist_t	*artend;
	int		i;

	qsort(gt_artists, gt_artists_cnt, sizeof(gt_artist_t),
	    cmp_artist_by_id);

	/* This sort will make sure that tracks are added to artists in order
	 * of popularity. */
	qsort(gt_tracks, gt_tracks_cnt, sizeof(gt_track_t),
	    cmp_track_by_art_id_pop);

	i = 0;
	art = gt_artists;
	artend = (gt_artists + gt_artists_cnt);
	end = (gt_tracks + gt_tracks_cnt);
	for(trk = gt_tracks; trk < end; trk++) {
		while(art < artend && art->ga_id < trk->gt_art_id)
			++art;
		if(art < artend && trk->gt_art_id == art->ga_id)
			add_artist_track(art, trk);

		++i;
	}

	return 0;
}


#define GT_HIERS_INITSIZE	32
#define GT_HIERS_GROW		32


int
add_hier(unsigned id, char *name)
{
	gt_hier_t	*hier;
	gt_hier_t	*newhier;

	if(gt_hiers == NULL) {
		/* First one. */
		gt_hiers = malloc(GT_HIERS_INITSIZE * sizeof(gt_hier_t));
		if(gt_hiers == NULL) {
			gt_log("Can't allocate hierarchy table!");
			return ENOMEM;
		}
		gt_hiers_size = GT_HIERS_INITSIZE;
		gt_hiers_cnt = 1;
		hier = &gt_hiers[0];
		memset(hier, 0, sizeof(gt_hier_t));
	} else {

		if(gt_hiers_cnt == gt_hiers_size) {
			/* Need to grow. */
			newhier = realloc(gt_hiers,
			    (gt_hiers_size + GT_HIERS_GROW) *
			    sizeof(gt_hier_t));
			if(gt_hiers == NULL) {
				gt_log("Can't increase size of"
				    " hierarchy table!");
				return ENOMEM;
			}

			gt_hiers = newhier;
			gt_hiers_size += GT_HIERS_GROW;
		}

		gt_hiers_cnt++;
		hier = &gt_hiers[gt_hiers_cnt - 1];
		memset(hier, 0, sizeof(gt_hier_t));
	}

	hier->gh_id = id;
	hier->gh_name = strdup(name);
	if(hier->gh_name == NULL) {
		gt_log("Can't copy hierarchy name!");
		return ENOMEM;
	}

	return 0;
}


#define GT_HIER_NODES_INITSIZE	2048
#define GT_HIER_NODES_GROW	256

int
add_hier_node(unsigned id, char *name, unsigned hierid,
    unsigned level, unsigned parentid)
{
	gt_hier_node_t	*node;
	gt_hier_node_t	*newnode;

	if(gt_hier_nodes == NULL) {
		/* First one. */
		gt_hier_nodes = malloc(GT_HIER_NODES_INITSIZE *
		    sizeof(gt_hier_node_t));
		if(gt_hier_nodes == NULL) {
			gt_log("Can't allocate hierarchy node table!");
			return ENOMEM;
		}
		gt_hier_nodes_size = GT_HIER_NODES_INITSIZE;
		gt_hier_nodes_cnt = 1;
		node = &gt_hier_nodes[0];
		memset(node, 0, sizeof(gt_hier_node_t));
	} else {

		if(gt_hier_nodes_cnt == gt_hier_nodes_size) {
			/* Need to grow. */
			newnode = realloc(gt_hier_nodes,
			    (gt_hier_nodes_size + GT_HIER_NODES_GROW) *
			    sizeof(gt_hier_node_t));
			if(gt_hier_nodes == NULL) {
				gt_log("Can't increase size of"
				    " hierarchy node table!");
				return ENOMEM;
			}

			gt_hier_nodes = newnode;
			gt_hier_nodes_size += GT_HIER_NODES_GROW;
		}

		gt_hier_nodes_cnt++;
		node = &gt_hier_nodes[gt_hier_nodes_cnt - 1];
		memset(node, 0, sizeof(gt_hier_node_t));
	}

	node->gn_id = id;
	node->gn_name = strdup(name);
	if(node->gn_name == NULL) {
		gt_log("Can't copy hierarchy node name!");
		return ENOMEM;
	}
	node->gn_level = level;
	node->gn_hier_id = hierid;
	node->gn_parentid = parentid;

	return 0;
}


int
gt_add_hier_node_links()
{
	gt_hier_node_t	*node;
	gt_hier_node_t	*end;
	gt_hier_node_t	*parent;
	gt_hier_t	*hier;
	int		ret;

	end = (gt_hier_nodes + gt_hier_nodes_cnt);
	for(node = gt_hier_nodes; node < end; node++) {

		/* First level nodes don't have parent. Add them to the
		 * hierarchy instead. */
		if(node->gn_parentid == 0 && node->gn_level == 1) {

			ret = gt_lookup_hierarchy(node->gn_hier_id, &hier);
			if(ret != 0) {
				gt_log("Missing hierarchy for node %d!");
				continue;
			}

			if(hier->gh_first_top == NULL) {
				/* First top-level node in hierarchy. */
				hier->gh_first_top = node;
				hier->gh_last_top = node;
			} else {
				hier->gh_last_top->gn_next_sibling = node;
				hier->gh_last_top = node;
			}

		} else {
			/* Add to parent node's children. */
			ret = gt_lookup_hierarchy_node(node->gn_parentid,
			    &parent);
			if(ret != 0) {
				gt_log("Missing parent for node %d!",
				    node->gn_id);
				continue;
			}

			if(parent->gn_first_child == NULL) {
				/* First child node for parent. */
				parent->gn_first_child = node;
				parent->gn_last_child = node;
			} else {
				parent->gn_last_child->gn_next_sibling = node;
				parent->gn_last_child = node;
			}

			node->gn_parent = parent;
		}

	}

	return 0;
}


#define GT_HIER_NODE_LOC_INITSIZE	32
#define GT_HIER_NODE_LOC_GROW		8


int
add_hier_node_locstr(unsigned nodeid, unsigned langid, char *locstr)
{
	gt_hier_node_t	*node;
	gt_locstr_t	*newlocs;
	int		ret;
	gt_locstr_t	*loc;

	ret = gt_lookup_hierarchy_node(nodeid, &node);
	if(ret != 0)
		return ret;

	if(node->gn_locs == NULL) {
		/* First localization for node. */
		node->gn_locs = malloc(GT_HIER_NODE_LOC_INITSIZE *
		    sizeof(gt_locstr_t));
		if(node->gn_locs == NULL) {
			gt_log("Can't allocate node localization list!");
			return ENOMEM;
		}
		node->gn_locs_size = GT_HIER_NODE_LOC_INITSIZE;
	} else
	if(node->gn_locs_size == node->gn_locs_cnt) {
		/* Need to grow localization list. */
		newlocs = realloc(node->gn_locs,
		    (node->gn_locs_size + GT_HIER_NODE_LOC_GROW) *
                    sizeof(gt_locstr_t));
		if(newlocs == NULL) {
			gt_log("Can't increase size"
			    " of node localization list!");
			return ENOMEM;
		}
		node->gn_locs = newlocs;
		node->gn_locs_size += GT_HIER_NODE_LOC_GROW;
	}

	loc = &node->gn_locs[node->gn_locs_cnt];
	loc->go_langid = langid;
	loc->go_str = strdup(locstr);
	if(loc->go_str == NULL) {
		gt_log("Can't copy language ISO!");
		return ENOMEM;
	}

	node->gn_locs_cnt++;

	gt_hier_nodes_alllocstr++;

	return 0;
}


int
gt_lookup_hierarchy(int id, gt_hier_t **res)
{
	gt_hier_t	*hier;
	gt_hier_t	*end;

	end = (gt_hiers + gt_hiers_cnt);
	for(hier = gt_hiers; hier < end; hier++) {
		if(hier->gh_id == id) {
			*res = hier;
			return 0;
		}
	}

	return ENOENT;
}


int gt_lookup_hierarchy_node(int id, gt_hier_node_t **res)
{
	gt_hier_node_t	*node;
	gt_hier_node_t	*end;

	end = (gt_hier_nodes + gt_hier_nodes_cnt);
	for(node = gt_hier_nodes; node < end; node++) {
		if(node->gn_id == id) {
			*res = node;
			return 0;
		}
	}

	return ENOENT;
}


int
gt_get_full_hierlist(gt_hier_t **res, int *cnt)
{
	*res = gt_hiers;
	*cnt = (int)gt_hiers_cnt;

	return 0;
}


int
gt_get_full_hier_nodelist(gt_hier_node_t **res, int *cnt)
{
	*res = gt_hier_nodes;
	*cnt = (int)gt_hier_nodes_cnt;

	return 0;
}

#define GT_LANGS_INITSIZE	32
#define GT_LANGS_GROW		16


int
add_language(unsigned id, char *name, char *iso)
{
	gt_lang_t	*lang;
	gt_lang_t	*newlang;

	if(gt_langs == NULL) {
		/* First one. */
		gt_langs = malloc(GT_LANGS_INITSIZE * sizeof(gt_lang_t));
		if(gt_langs == NULL) {
			gt_log("Can't allocate language table!");
			return ENOMEM;
		}
		gt_langs_size = GT_HIERS_INITSIZE;
		gt_langs_cnt = 1;
		lang = &gt_langs[0];
		memset(lang, 0, sizeof(gt_lang_t));
	} else {

		if(gt_langs_cnt == gt_langs_size) {
			/* Need to grow. */
			newlang = realloc(gt_langs,
			    (gt_langs_size + GT_LANGS_GROW) *
			    sizeof(gt_lang_t));
			if(gt_langs == NULL) {
				gt_log("Can't increase size of"
				    " language table!");
				return ENOMEM;
			}

			gt_langs = newlang;
			gt_langs_size += GT_LANGS_GROW;
		}

		gt_langs_cnt++;
		lang = &gt_langs[gt_langs_cnt - 1];
		memset(lang, 0, sizeof(gt_lang_t));
	}

	lang->gl_id = id;
	lang->gl_name = strdup(name);
	if(lang->gl_name == NULL) {
		gt_log("Can't copy language name!");
		return ENOMEM;
	}
	lang->gl_iso = strdup(iso);
	if(lang->gl_iso == NULL) {
		gt_log("Can't copy language ISO!");
		return ENOMEM;
	}

	return 0;
}


int gt_lookup_lang(int id, gt_lang_t **res)
{
	gt_lang_t	*lang;
	gt_lang_t	*end;

	end = (gt_langs + gt_langs_cnt);
	for(lang = gt_langs; lang < end; lang++) {
		if(lang->gl_id == id) {
			*res = lang;
			return 0;
		}
	}

	return ENOENT;
}


int
gt_get_full_langlist(gt_lang_t **res, int *cnt)
{
	*res = gt_langs;
	*cnt = (int)gt_langs_cnt;

	return 0;
}


#define GT_GOET_TO_NODE_INITSIZE	8
#define GT_GOET_TO_NODE_GROW		8
#define GT_NODE_TO_GOET_INITSIZE	16
#define GT_NODE_TO_GOET_GROW		16


int
add_goet_to_hier_node(unsigned goetid, unsigned nodeid)
{
	gt_goet_t	*goet;
	gt_hier_node_t	*node;
	int		ret;
	unsigned	*newlist;

	ret = gt_lookup_goet(goetid, &goet);
	if(ret != 0) {
		gt_log("Can't find GOET %d!", goetid);
		return 0;
//		return ENOENT;
	}
	ret = gt_lookup_hierarchy_node(nodeid, &node);
	if(ret != 0) {
		gt_log("Can't find node %d!", nodeid);
		return 0;
//		return ENOENT;
	}

	/* Add link to goet. */
	if(goet->gg_hier_node_ids == NULL) {
		/* First hier node ID for goet. */
		goet->gg_hier_node_ids = malloc(GT_GOET_TO_NODE_INITSIZE *
		    sizeof(unsigned));
		if(goet->gg_hier_node_ids == NULL) {
			gt_log("Can't allocate goet to hier node list!");
			return ENOMEM;
		}
		goet->gg_hier_node_ids_size = GT_GOET_TO_NODE_INITSIZE;
	} else
	if(goet->gg_hier_node_ids_size == goet->gg_hier_node_ids_cnt) {
		/* Need to grow ID list. */
		newlist = realloc(goet->gg_hier_node_ids,
		    (goet->gg_hier_node_ids_size + GT_GOET_TO_NODE_GROW) *
                    sizeof(unsigned));
		if(newlist == NULL) {
			gt_log("Can't increase size of"
			    " goet to hier node list!");
			return ENOMEM;
		}
		goet->gg_hier_node_ids = newlist;
		goet->gg_hier_node_ids_size += GT_GOET_TO_NODE_GROW;
	}

	goet->gg_hier_node_ids[goet->gg_hier_node_ids_cnt] = nodeid;
	goet->gg_hier_node_ids_cnt++;


	/* Add link to node. */
	if(node->gn_goet_ids == NULL) {
		/* First goet ID for node. */
		node->gn_goet_ids = malloc(GT_NODE_TO_GOET_INITSIZE *
		    sizeof(unsigned));
		if(node->gn_goet_ids == NULL) {
			gt_log("Can't allocate hier node to goet list!");
			return ENOMEM;
		}
		node->gn_goet_ids_size = GT_NODE_TO_GOET_INITSIZE;
	} else
	if(node->gn_goet_ids_size == node->gn_goet_ids_cnt) {
		/* Need to grow ID list. */
		newlist = realloc(node->gn_goet_ids,
		    (node->gn_goet_ids_size + GT_NODE_TO_GOET_GROW) *
                    sizeof(unsigned));
		if(newlist == NULL) {
			gt_log("Can't increase size of"
			    " hier node to goet list!");
			return ENOMEM;
		}
		node->gn_goet_ids = newlist;
		node->gn_goet_ids_size += GT_NODE_TO_GOET_GROW;
	}

	node->gn_goet_ids[node->gn_goet_ids_cnt] = goetid;
	node->gn_goet_ids_cnt++;

	gt_goet_to_hier_node_allcnt++;
	return 0;
}

void
gt_trim_string(char *str)
{
    char	*ch;

	if(str == NULL || !strlen(str))
		return;

	/* Remove new line at end. */
	ch = &str[strlen(str) - 1];
	while(ch >= str) {
		if(*ch == '\n' || *ch == '\r' || *ch == ' ') {
			*ch = '\0';
		}
		else
            break;

		ch--;
	}
}
/*split_result should be allocated before calling function*/
/*this is because memory allocation is expensive. don't want to allocate memory for every string split*/
int
gt_split_text(char *text, const char *delim, char **split_result, int column_cnt)
{
    int columns_set_cnt = 0;

    return gt_split_text_extended(text, delim, split_result, column_cnt, &columns_set_cnt);
}

int
gt_split_text_extended(char *text, const char *delim, char **split_result, int column_cnt, int *columns_set_cnt_out)
{
    char    *token = NULL;
    int     i = 0;

    *columns_set_cnt_out = 0;

    if (text == NULL || strlen(text) == 0)
        return EINVAL;

    token = gt_strsep(&text, delim);
    while (token != NULL) {

        if (i >= column_cnt) {
            gt_log("Warning, more columns found in row than expected.");
            break;
        }

        /*snprintf is way faster than strncpy on windows cygwin compiler*/
        snprintf(split_result[i], GT_MAXFILELINE, "%s", token);
        /*strncpy(split_result[i], token, GT_MAXFILELINE - 1);*/

        token = gt_strsep(&text, delim);
        i++;
    }

    *columns_set_cnt_out = i;

    return 0;
}

/*returns number of tokens in string*/
int
gt_get_split_text_cnt(const char *text, const char *delim)
{
    char    *textcopy = NULL;
    char    *token = NULL;
	char	*pmem = NULL;	/*have to keep pointer to memory allocated, since gt_strsep changes textcopy*/
    int     cnt = 0;

    if (text == NULL || strlen(text) == 0)
        return 0;

    textcopy = strdup(text);
	pmem = textcopy;
    if (textcopy == NULL) {
        gt_log("Error: out of memory!");
        return 0;
    }

    token = gt_strsep(&textcopy, delim);
    while (token != NULL) {
        cnt++;
        token = gt_strsep(&textcopy, delim);
    }
    free(pmem);

    return cnt;
}

char **
gt_allocate_split_text(int cnt)
{
    int     i = 0;
    char    **split_text = (char **) malloc (sizeof(char *) * cnt);

    if (split_text == NULL) {
        gt_log("Error: out of memory!");
        return NULL;
    }

    for (i = 0; i < cnt; ++i) {
        split_text[i] = (char *) malloc (sizeof(char) * GT_MAXFILELINE);

        if (split_text[i] == NULL) {
            gt_log("Error: out of memory!");
            return NULL;
        }

        memset(split_text[i], 0, sizeof(char) * GT_MAXFILELINE);
    }

    return split_text;
}

void
gt_free_split_text(char **split_result, int cnt)
{
    int     i = 0;

    if (split_result == NULL)
        return;

    for(i = 0; i < cnt; ++i) {
        free(split_result[i]);
    }

    free(split_result);
}

/*custom implementation of strsep, since not in win c lib*/
/*NOTE: like strsep, this function will overwrite the text passed into it*/
char *
gt_strsep(char **text, const char *delim)
{
    char    *start = *text;
    char    *p;

    p = (start != NULL) ? strpbrk(start, delim) : NULL;

    if (p == NULL) {
        *text = NULL;
    } else {
        *p = '\0';
        *text = p + 1;
    }

    return start;
}

static int
gt_get_column_index(int gt_table_type, int gt_column_type, char **headers, int cnt_headers)
{
    char header_text[GT_MAXFILELINE] = {0};
    int index_found = -1;
    int result = 0;
    int i = 0;

    result = gt_get_header_from_column(gt_table_type, gt_column_type, header_text);
    if (result != 0)
        return -1;

    for (i = 0; i < cnt_headers; ++i) {

        if (strcasecmp(header_text, headers[i]) == 0) {
            index_found = i;
            break;
        }
    }

    /*
    if (index_found < 0) {
        gt_log("Warning, could not find column header: %s", header_text);
    }
    */

    return index_found;
}

static int
gt_get_header_from_column(int gt_table_type, int gt_column_type, char *header_text)
{
    int iReturn = 1;

    if (header_text == NULL)
        return -1;

    strcpy(header_text, "\0");

    /*check column overrides*/
    if (gt_get_column_override(gt_table_type, gt_column_type, header_text)) {

        gt_log("Using column override: %s", header_text);
        return 0;
    }

    switch (gt_column_type) {
        case GT_TABLE_COLUMN_TYPE_ARTIST_ID:
            strncpy(header_text, "ARTIST_ID", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_DESCRIPTOR_ID:
            strncpy(header_text, "DESCRIPTOR_ID", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_DESCRIPTOR_TYPE:
            strncpy(header_text, "DESCRIPTOR_TYPE", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_WEIGHT:
            strncpy(header_text, "WEIGHT", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_ARTIST_NAME:
            strncpy(header_text, "ARTIST_NAME", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_POPULARITY:
            strncpy(header_text, "VOLUME", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_DESCRIPTOR_NAME:
            strncpy(header_text, "DESCRIPTOR_NAME", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_DESCRIPTOR_ID_1:
            strncpy(header_text, "DESCRIPTOR_ID_1", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_DESCRIPTOR_ID_2:
            strncpy(header_text, "DESCRIPTOR_ID_2", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_RECORDING_ID:
            strncpy(header_text, "RECORDING_ID", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_RECORDING_NAME:
            strncpy(header_text, "NAME", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_ALBUM_NAME:
            strncpy(header_text, "ALBUM_NAME", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_YEAR:
            strncpy(header_text, "RELEASE_YEAR", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_HIERARCHY_TYPE_ID:
            strncpy(header_text, "HIERARCHY_TYPE_ID", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_HIERARCHY_NAME:
            strncpy(header_text, "HIERARCHY_NAME", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_HIERARCHY_NODE_ID:
            strncpy(header_text, "HIERARCHY_NODE_ID", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_HIERARCHY_NODE_NAME:
            strncpy(header_text, "HIERARCHY_NODE_NAME", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_LEVEL:
            strncpy(header_text, "LEVEL", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_PARENT_NODE_ID:
            strncpy(header_text, "PARENT_NODE_ID", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_LANGUAGE_ID:
            strncpy(header_text, "LANGUAGE_ID", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_LANGUAGE_NAME:
            strncpy(header_text, "LANGUAGE_NAME", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_LANGUAGE_ISO:
            strncpy(header_text, "ISO_CODE", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_LOCALIZED_NAME:
            strncpy(header_text, "HIERARCHY_NODED_NAME_LOCALIZED", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_CORRELATE_TYPE:
            strncpy(header_text, "CORRELATE_TYPE", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_CORRELATE_VALUE:
            strncpy(header_text, "CORRELATE_VALUE", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_CATEGORY_ID:
            strncpy(header_text, "CATEGORY_ID", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_CATEGORY_TYPE:
            strncpy(header_text, "CATEGORY_TYPE", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_TRACK_ID:
            strncpy(header_text, "TRACK_ID", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_TRACK_NAME:
            strncpy(header_text, "TRACK_NAME", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_CATEGORY_NAME:
            strncpy(header_text, "CATEGORY_NAME", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_SRC_CATEGORY_ID:
            strncpy(header_text, "SRC_CATEGORY_ID", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_TGT_CATEGORY_ID:
            strncpy(header_text, "TGT_CATEGORY_ID", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_ALBUM_ID:
            strncpy(header_text, "ALBUM_ID", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_RELEASE_YEAR:
            strncpy(header_text, "RELEASE_YEAR", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_NEW_POPULARITY:
            strncpy(header_text, "NEWPOPULARITY", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_GN_ID:
            strncpy(header_text, "GN_ID", GT_MAXFILELINE);
            break;
        case GT_TABLE_COLUMN_TYPE_LOCALIZED_NAME2:
            strncpy(header_text, "HIERARCHY_NODE_NAME_LOCALIZED", GT_MAXFILELINE);
    }

    if (strlen(header_text) > 0) {
        iReturn = 0;
    } else {
        iReturn = EINVAL;
        gt_log("Error, invalid column header.");
    }

    return iReturn;
}

static int
gt_load_table_artist_goets()
{
    FILE		*f;
	int		    err;
	char		line[GT_MAXFILELINE];
	unsigned	artistid;
	unsigned	goetid;
	int		    weight;
	char		typestr[128];
	int		    type;
	int         ret;
	char        **headers = NULL;
	char        **data_row = NULL;
	char        *tail = NULL;
	int         cnt_columns = 0;
	int         artistid_index = 0;
	int         goetid_index = 0;
	int         type_index = 0;
	int         weight_index = 0;
	int         i = 0;

	err = 0;

    gt_log("Loading artist GOET table '%s'", gt_file_artist_category);

	f = fopen(gt_file_artist_category, "r");
	if(f == NULL) {
		gt_log("Can't open file: %s", strerror(errno));
		return ENOENT;
	}

	while(fgets(line, GT_MAXFILELINE, f)) {

        gt_trim_string(line);

        if (headers == NULL) {
            cnt_columns = gt_get_split_text_cnt(line, GT_DATA_DELIM);

            if (cnt_columns < 4) {
                gt_log("Error parsing headers in artist GOET table");
                err = EINVAL;
                break;
            }

            /*allocating once for performance*/
            headers = gt_allocate_split_text(cnt_columns);
            data_row = gt_allocate_split_text(cnt_columns);

            if (headers == NULL || data_row == NULL) {
                gt_log("Error: out of memory!");
                err = ENOMEM;
                break;
            }

            if ((ret = gt_split_text(line, GT_DATA_DELIM, headers, cnt_columns)) != 0) {
                gt_log("Error parsing headers in artist GOET table");
                err = EINVAL;
                break;
            }

            artistid_index = gt_get_column_index(GT_TABLE_TYPE_ARTIST_CATEGORY, GT_TABLE_COLUMN_TYPE_ARTIST_ID, headers, cnt_columns);
            goetid_index = gt_get_column_index(GT_TABLE_TYPE_ARTIST_CATEGORY, GT_TABLE_COLUMN_TYPE_DESCRIPTOR_ID, headers, cnt_columns);
            weight_index = gt_get_column_index(GT_TABLE_TYPE_ARTIST_CATEGORY, GT_TABLE_COLUMN_TYPE_WEIGHT, headers, cnt_columns);
            type_index = gt_get_column_index(GT_TABLE_TYPE_ARTIST_CATEGORY, GT_TABLE_COLUMN_TYPE_DESCRIPTOR_TYPE, headers, cnt_columns);

            /*check txt file column names if column not found*/
            if (goetid_index < 0) {
                goetid_index = gt_get_column_index(GT_TABLE_TYPE_ARTIST_CATEGORY, GT_TABLE_COLUMN_TYPE_CATEGORY_ID, headers, cnt_columns);
            }
            if (type_index < 0) {
                type_index = gt_get_column_index(GT_TABLE_TYPE_ARTIST_CATEGORY, GT_TABLE_COLUMN_TYPE_CATEGORY_TYPE, headers, cnt_columns);
            }

            if (artistid_index < 0 ||
                goetid_index < 0 ||
                weight_index < 0 ||
                type_index < 0) {

                    gt_log("Error parsing headers in artist GOET table");
                    err = EINVAL;
                    break;
                }

            continue;
        }

        if ((ret = gt_split_text(line, GT_DATA_DELIM, data_row, cnt_columns)) != 0) {
            gt_log("Warning, invalid row in artist GOET table");
            continue;
        }

        artistid = strtoul(data_row[artistid_index], &tail, 10);
        goetid = strtoul(data_row[goetid_index], &tail, 10);
        weight = strtol(data_row[weight_index], &tail, 10);
        strncpy(typestr, data_row[type_index], 127);

		type = gettype(typestr);
		if(type < 0)
			continue;

        if (weight == 0)
            continue;

		ret = add_artist_goet(artistid, goetid, type, weight);
		if(ret != 0) {
			err = ENOEXEC;
			break;
		}
	}

	fclose(f);

    if (data_row != NULL) {
        gt_free_split_text(data_row, cnt_columns);
        data_row = NULL;
    }

    if (headers != NULL) {
        gt_free_split_text(headers, cnt_columns);
        headers = NULL;
    }

    if (err == 0) {
        gt_log("Loaded %d artists, %d GOET values.", gt_artists_cnt,
            gt_artgoet_allcnt);

        gt_log("Sorting artist goet values.");
        for (i = 0; i < gt_artists_cnt; ++i)
        {
            qsort(gt_artists[i].ga_goetcorr, gt_artists[i].ga_goetcorr_cnt, sizeof(gt_goetcorr_t), cmp_goetcorr_by_weight_type);
        }
        gt_log("Done sorting artist goet values.");

        /* Sort artists by ID to be able to search it easily
         * in the next steps. */
        qsort(gt_artists, gt_artists_cnt, sizeof(gt_artist_t),
            cmp_artist_by_id);
    }

    return err;
}

static int gmd_load_table_artists(){
	int result = 0;
	unsigned int artist_id_num = 0;
	unsigned int descriptor_id = 0;
	gmd_json_reader_t *reader = NULL;
	char artist_id_gmd[GT_MAX_SIZE_INPUTSTR];
	char artist_name[GT_MAX_SIZE_INPUTSTR];
	gmd_json_descriptor_t descriptors[GMD_DESCRIPTOR_MAX_SIZE];
	int cnt_descriptors = 0;
	int has_genre = 0;
	int total_artists = 0;
	int total_descriptors = 0;
	int i = 0;

    gt_log("Loading GMD artist file '%s'", gt_file_artist_list);
    result = gmd_json_reader_init(&reader, gt_file_artist_list, GMD_TYPE_ARRAY, GMD_KEY_ARTISTS, 1, GT_MAX_SIZE_INPUTSTR, gt_log);
	if(result != 0) {
		gt_log("JSON reader failed to initialize: %s", strerror(errno));
		return ENOENT;
	}

    while (result != GMD_JSON_NO_MORE_DATA)
    {
    	has_genre = 0;
        result = gmd_json_reader_get_artist_info(reader, 50, artist_id_gmd, artist_name, descriptors, &cnt_descriptors);

        if (result == 0)
        {
            for (i = 0; i < cnt_descriptors; ++i)
            {
            	if (descriptors[i].type == GMD_DESCRIPTOR_TYPE_GENRE) {
            		has_genre = 1;
            		break;
            	}
            }
           	if (!has_genre)
           		continue;
           	artist_id_num = add_gmd_id(artist_id_gmd);
            for (i = 0; i < cnt_descriptors; ++i) {
                descriptor_id = strtoul(descriptors[i].id, NULL, 10);
            	result = add_artist_goet(artist_id_num, descriptor_id, descriptors[i].type, atoi(descriptors[i].weight));
            	if (result != 0) {
            		return result;
            	}
            	total_descriptors++;
            }
            result = add_artist_name(artist_id_num, artist_name);
            if (result != 0)
            	return result;
            total_artists++;
        }
    }
    gt_log("Loaded %d artists, %d GOET values.", total_artists,
        total_descriptors);

    gt_log("Sorting artist goet values.");
    for (i = 0; i < gt_artists_cnt; ++i)
    {
        qsort(gt_artists[i].ga_goetcorr, gt_artists[i].ga_goetcorr_cnt, sizeof(gt_goetcorr_t), cmp_goetcorr_by_weight_type);
    }
    gt_log("Done sorting artist goet values.");

    /* Sort artists by ID to be able to search it easily
     * in the next steps. */
    qsort(gt_artists, gt_artists_cnt, sizeof(gt_artist_t),
        cmp_artist_by_id);

    result = gmd_json_reader_uninit(reader);
    return 0;
}


static int
gt_load_table_artist_names()
{
    FILE		*f;
	int		    err;
	char		line[GT_MAXFILELINE];
	unsigned	artistid;
	int		    ret;
	char		artistname[GT_MAX_SIZE_INPUTSTR];
	char        **headers = NULL;
	char        **data_row = NULL;
	char        *tail = NULL;
	int         cnt_columns = 0;
	int         artistid_index = 0;
	int         artistname_index = 0;

	err = 0;

    gt_log("Loading artist names '%s'", gt_file_artist_list);

	f = fopen(gt_file_artist_list, "r");
	if(f == NULL) {
		gt_log("Can't open file: %s", strerror(errno));
		return ENOENT;
	}

	while(fgets(line, GT_MAXFILELINE, f)) {

	    gt_trim_string(line);

        if (headers == NULL) {
            cnt_columns = gt_get_split_text_cnt(line, GT_DATA_DELIM);

            if (cnt_columns < 2) {
                gt_log("Error parsing headers in artist names file");
                err = EINVAL;
                break;
            }

            /*allocating once for performance*/
            headers = gt_allocate_split_text(cnt_columns);
            data_row = gt_allocate_split_text(cnt_columns);

            if (headers == NULL || data_row == NULL) {
                gt_log("Error: out of memory!");
                err = ENOMEM;
                break;
            }

            if ((ret = gt_split_text(line, GT_DATA_DELIM, headers, cnt_columns)) != 0) {
                gt_log("Error parsing headers in artist names file");
                err = EINVAL;
                break;
            }

            artistid_index = gt_get_column_index(GT_TABLE_TYPE_ARTIST_LIST, GT_TABLE_COLUMN_TYPE_ARTIST_ID, headers, cnt_columns);
            artistname_index = gt_get_column_index(GT_TABLE_TYPE_ARTIST_LIST, GT_TABLE_COLUMN_TYPE_ARTIST_NAME, headers, cnt_columns);

            if (artistid_index < 0 ||
                artistname_index < 0) {

                    gt_log("Error parsing headers in artist names file");
                    err = EINVAL;
                    break;
                }

            continue;
        }

		if ((ret = gt_split_text(line, GT_DATA_DELIM, data_row, cnt_columns)) != 0) {
            gt_log("Warning, invalid row in artist names table");
            continue;
        }

        artistid = strtoul(data_row[artistid_index], &tail, 10);
        strncpy(artistname, data_row[artistname_index], GT_MAX_SIZE_INPUTSTR - 1);

		ret = add_artist_name(artistid, artistname);
		/* Ignore ENOENT, this list contains more artists than we
		 * read previously */
		if(ret != 0 && ret != ENOENT) {
			err = ret;
			break;
		}
	}

	fclose(f);

    if (data_row != NULL) {
        gt_free_split_text(data_row, cnt_columns);
        data_row = NULL;
    }

    if (headers != NULL) {
        gt_free_split_text(headers, cnt_columns);
        headers = NULL;
    }

    if (err == 0) {
        gt_log("Loaded %d artist names.", gt_artname_allcnt);
    }

    return err;
}

static int
gt_load_table_artist_popularity()
{
    FILE		*f;
	int		    err;
	char		line[GT_MAXFILELINE];
	unsigned	artistid;
	unsigned	pop;
	int		    ret;
	int         i;
	char        **headers = NULL;
	char        **data_row = NULL;
	char        *tail = NULL;
	int         cnt_columns = 0;
	int         artistid_index = 0;
	int         pop_index = 0;

	err = 0;

    gt_log("Loading artist popularities '%s'",
           gt_file_artist_popularity);

    f = fopen(gt_file_artist_popularity, "r");
    if(f == NULL) {
        gt_log("Can't open file: %s", strerror(errno));
        return ENOENT;
    }

    while(fgets(line, GT_MAXFILELINE, f)) {

        gt_trim_string(line);

        if (headers == NULL) {
            cnt_columns = gt_get_split_text_cnt(line, GT_DATA_DELIM);

            if (cnt_columns < 2) {
                gt_log("Error parsing headers in artist popularity file");
                err = EINVAL;
                break;
            }

            /*allocating once for performance*/
            headers = gt_allocate_split_text(cnt_columns);
            data_row = gt_allocate_split_text(cnt_columns);

            if (headers == NULL || data_row == NULL) {
                gt_log("Error: out of memory!");
                err = ENOMEM;
                break;
            }

            if ((ret = gt_split_text(line, GT_DATA_DELIM, headers, cnt_columns)) != 0) {
                gt_log("Error parsing headers in artist popularity file");
                err = EINVAL;
                break;
            }

            artistid_index = gt_get_column_index(GT_TABLE_TYPE_ARTIST_POPULARITY, GT_TABLE_COLUMN_TYPE_ARTIST_ID, headers, cnt_columns);
            pop_index = gt_get_column_index(GT_TABLE_TYPE_ARTIST_POPULARITY, GT_TABLE_COLUMN_TYPE_POPULARITY, headers, cnt_columns);

            if (artistid_index < 0) {
                artistid_index = gt_get_column_index(GT_TABLE_TYPE_ARTIST_POPULARITY, GT_TABLE_COLUMN_TYPE_GN_ID, headers, cnt_columns);
            }

            if (pop_index < 0) {
                pop_index = gt_get_column_index(GT_TABLE_TYPE_ARTIST_POPULARITY, GT_TABLE_COLUMN_TYPE_NEW_POPULARITY, headers, cnt_columns);
            }

            if (artistid_index < 0 ||
                pop_index < 0) {

                    gt_log("Error parsing headers in artist popularity file");
                    err = EINVAL;
                    break;
                }

            continue;
        }

        if ((ret = gt_split_text(line, GT_DATA_DELIM, data_row, cnt_columns)) != 0) {
            gt_log("Warning, invalid row in artist popularity table");
            continue;
        }

        artistid = strtoul(data_row[artistid_index], &tail, 10);
        pop = strtoul(data_row[pop_index], &tail, 10);

        ret = add_artist_popularity(artistid, pop);
        /* Ignore ENOENT, this list may contain more artists
         * than we read previously */
        if(ret != 0 && ret != ENOENT) {
            err = ret;
            break;
        }
    }

    fclose(f);

    if (data_row != NULL) {
        gt_free_split_text(data_row, cnt_columns);
        data_row = NULL;
    }

    if (headers != NULL) {
        gt_free_split_text(headers, cnt_columns);
        headers = NULL;
    }

    if (err == 0) {
        /* Sort artists by popularity and assign ranking. */
        qsort(gt_artists, gt_artists_cnt, sizeof(gt_artist_t),
            cmp_artist_by_pop);
        for(i = 0; i < gt_artists_cnt; ++i)
            gt_artists[i].ga_rank = i + 1;
        gt_rank_unit = (int)(gt_artists_cnt / 1000);
        if(gt_rank_unit == 0)
            gt_rank_unit = 1;

        gt_log("Loaded %d artist popularities.", gt_artpop_allcnt);
    }

    return err;
}

static int
gt_load_table_goets()
{
    FILE		*f;
	int		    err;
	char		line[GT_MAXFILELINE];
	unsigned	goetid;
	char		typestr[128];
	int		    type;
	int		    ret;
	char		goetname[GT_MAX_SIZE_INPUTSTR];
	char        **headers = NULL;
	char        **data_row = NULL;
	char        *tail = NULL;
	int         cnt_columns = 0;
	int         goetid_index = 0;
	int         type_index = 0;
	int         goetname_index = 0;

	err = 0;

    gt_log("Loading GOET table '%s'", gt_file_category);

	f = fopen(gt_file_category, "r");
	if(f == NULL) {
		gt_log("Can't open file: %s", strerror(errno));
		return ENOENT;
	}

	while(fgets(line, GT_MAXFILELINE, f)) {

        gt_trim_string(line);

        if (headers == NULL) {
            cnt_columns = gt_get_split_text_cnt(line, GT_DATA_DELIM);

            if (cnt_columns < 3) {
                gt_log("Error parsing headers in GOET file");
                err = EINVAL;
                break;
            }

            /*allocating once for performance*/
            headers = gt_allocate_split_text(cnt_columns);
            data_row = gt_allocate_split_text(cnt_columns);

            if (headers == NULL || data_row == NULL) {
                gt_log("Error: out of memory!");
                err = ENOMEM;
                break;
            }

            if ((ret = gt_split_text(line, GT_DATA_DELIM, headers, cnt_columns)) != 0) {
                gt_log("Error parsing headers in GOET file");
                err = EINVAL;
                break;
            }

            goetid_index = gt_get_column_index(GT_TABLE_TYPE_CATEGORY, GT_TABLE_COLUMN_TYPE_DESCRIPTOR_ID, headers, cnt_columns);
            type_index = gt_get_column_index(GT_TABLE_TYPE_CATEGORY, GT_TABLE_COLUMN_TYPE_DESCRIPTOR_TYPE, headers, cnt_columns);
            goetname_index = gt_get_column_index(GT_TABLE_TYPE_CATEGORY, GT_TABLE_COLUMN_TYPE_DESCRIPTOR_NAME, headers, cnt_columns);

            /*check txt file column names if column not found*/
            if (goetid_index < 0) {
                goetid_index = gt_get_column_index(GT_TABLE_TYPE_CATEGORY, GT_TABLE_COLUMN_TYPE_CATEGORY_ID, headers, cnt_columns);
            }
            if (type_index < 0) {
                type_index = gt_get_column_index(GT_TABLE_TYPE_CATEGORY, GT_TABLE_COLUMN_TYPE_CATEGORY_TYPE, headers, cnt_columns);
            }
            if (goetname_index < 0) {
                goetname_index = gt_get_column_index(GT_TABLE_TYPE_CATEGORY, GT_TABLE_COLUMN_TYPE_CATEGORY_NAME, headers, cnt_columns);
            }

            if (goetid_index < 0 ||
                type_index < 0 ||
                goetname_index < 0) {

                    gt_log("Error parsing headers in GOET file");
                    err = EINVAL;
                    break;
                }

            continue;
        }

        if ((ret = gt_split_text(line, GT_DATA_DELIM, data_row, cnt_columns)) != 0) {
            gt_log("Warning, invalid row in GOET table");
            continue;
        }

        goetid = strtoul(data_row[goetid_index], &tail, 10);
        strncpy(typestr, data_row[type_index], 127);
        strncpy(goetname, data_row[goetname_index], GT_MAX_SIZE_INPUTSTR - 1);

		type = gettype(typestr);
		if(type < 0)
			continue;

		ret = add_goet(goetid, type, goetname);
		if(ret != 0) {
			err = ENOEXEC;
			break;
		}
	}

	fclose(f);

	if (data_row != NULL) {
        gt_free_split_text(data_row, cnt_columns);
        data_row = NULL;
	}

    if (headers != NULL) {
        gt_free_split_text(headers, cnt_columns);
        headers = NULL;
    }

    if (err == 0) {
        gt_log("Loaded %d GOETs.", gt_goets_cnt);

        /* Sort GOETs. */
        qsort(gt_goets, gt_goets_cnt, sizeof(gt_goet_t),
            cmp_goet);

    }

    return err;
}

static int
gmd_load_table_goets()
{
	int result = 0;
	gmd_json_reader_t *reader = NULL;

	char descriptor_id[GMD_MAX_LINE];
	char descriptor_name[GMD_MAX_LINE];
	int descriptor_result = 0;
	int i = 0;

    char *descriptor_keys[] = {GMD_KEY_GENRES, GMD_KEY_ORIGINS, GMD_KEY_ERAS, GMD_KEY_ATYPES, GMD_KEY_MOODS, GMD_KEY_LANGUAGES, GMD_KEY_TEMPOS};
    int descriptor_types[] = {GMD_DESCRIPTOR_TYPE_GENRE, GMD_DESCRIPTOR_TYPE_ORIGIN, GMD_DESCRIPTOR_TYPE_ERA, GMD_DESCRIPTOR_TYPE_ATYPE, GMD_DESCRIPTOR_TYPE_MOOD, GMD_DESCRIPTOR_TYPE_LANG, GMD_DESCRIPTOR_TYPE_TEMPO};

    gt_log("Loading GMD descriptor file '%s'", gt_file_category);
    for (i=0; i<7; i++) {
        result = gmd_json_reader_init(&reader, gt_file_category, GMD_TYPE_ARRAY, descriptor_keys[i], 2, GT_MAX_SIZE_INPUTSTR, gt_log);

		if(result != 0) {
			gt_log("JSON reader failed to initialize: %s", strerror(errno));
			return ENOENT;
		}
        descriptor_result = 0;
        while (descriptor_result != GMD_JSON_NO_MORE_DATA)
        {
            descriptor_id[0] = '\0';
            descriptor_name[0] = '\0';
            descriptor_result = 0;
            descriptor_result = gmd_json_reader_get_descriptor_info(reader, descriptor_name, descriptor_id);
            if (descriptor_result == 0 &&
                descriptor_id[0] != '\0' &&
                descriptor_name[0] != '\0') {
                unsigned int id = strtoul(descriptor_id, NULL, 10);
                result = add_goet(id, descriptor_types[i], descriptor_name);
                if(result != 0) {
                    return result;
                }
            }
        }
        result = gmd_json_reader_uninit(reader);
		if(result != 0) {
			return result;
		}
    }

    gt_log("Loaded %d GOETs.", gt_goets_cnt);

    /* Sort GOETs. */
    qsort(gt_goets, gt_goets_cnt, sizeof(gt_goet_t),
        cmp_goet);

    return 0;
}

static int
gt_load_table_goet_correlations()
{
    FILE		*f;
	int		    err;
	char		line[GT_MAXFILELINE];
	unsigned	sourceid;
	unsigned	targetid;
	int		    weight;
	char		typestr[128];
	int		    type;
	int		    ret;
	char        **headers = NULL;
	char        **data_row = NULL;
	char        *tail = NULL;
	int         cnt_columns = 0;
	int         type_index = 0;
	int         weight_index = 0;
	int         sourceid_index = 0;
	int         targetid_index = 0;

	err = 0;

	gt_log("Loading GOET correlate table '%s'", gt_file_correlate);

	f = fopen(gt_file_correlate, "r");
	if(f == NULL) {
		gt_log("Can't open file: %s", strerror(errno));
		return ENOENT;
	}

	while(fgets(line, GT_MAXFILELINE, f)) {

        gt_trim_string(line);

        if (headers == NULL) {
            cnt_columns = gt_get_split_text_cnt(line, GT_DATA_DELIM);

            if (cnt_columns < 4) {
                gt_log("Error parsing headers in GOET correlate file");
                err = EINVAL;
                break;
            }

             /*allocating once for performance*/
            headers = gt_allocate_split_text(cnt_columns);
            data_row = gt_allocate_split_text(cnt_columns);

            if (headers == NULL || data_row == NULL) {
                gt_log("Error: out of memory!");
                err = ENOMEM;
                break;
            }

            if ((ret = gt_split_text(line, GT_DATA_DELIM, headers, cnt_columns)) != 0) {
                gt_log("Error parsing headers in GOET correlate file");
                err = EINVAL;
                break;
            }

            sourceid_index = gt_get_column_index(GT_TABLE_TYPE_CORRELATE, GT_TABLE_COLUMN_TYPE_DESCRIPTOR_ID_1, headers, cnt_columns);
            targetid_index = gt_get_column_index(GT_TABLE_TYPE_CORRELATE, GT_TABLE_COLUMN_TYPE_DESCRIPTOR_ID_2, headers, cnt_columns);
            weight_index = gt_get_column_index(GT_TABLE_TYPE_CORRELATE, GT_TABLE_COLUMN_TYPE_CORRELATE_VALUE, headers, cnt_columns);
            type_index = gt_get_column_index(GT_TABLE_TYPE_CORRELATE, GT_TABLE_COLUMN_TYPE_CORRELATE_TYPE, headers, cnt_columns);

            /*check txt file column names if column not found*/
            if (sourceid_index < 0) {
                sourceid_index = gt_get_column_index(GT_TABLE_TYPE_CORRELATE, GT_TABLE_COLUMN_TYPE_SRC_CATEGORY_ID, headers, cnt_columns);
            }
            if (targetid_index < 0) {
                targetid_index = gt_get_column_index(GT_TABLE_TYPE_CORRELATE, GT_TABLE_COLUMN_TYPE_TGT_CATEGORY_ID, headers, cnt_columns);
            }
            if (weight_index < 0) {
                weight_index = gt_get_column_index(GT_TABLE_TYPE_CORRELATE, GT_TABLE_COLUMN_TYPE_WEIGHT, headers, cnt_columns);
            }
            if (type_index < 0) {
                type_index = gt_get_column_index(GT_TABLE_TYPE_CORRELATE, GT_TABLE_COLUMN_TYPE_CATEGORY_TYPE, headers, cnt_columns);
            }

            if (sourceid_index < 0 ||
                targetid_index < 0 ||
                weight_index < 0 ||
                type_index < 0) {

                    gt_log("Error parsing headers in GOET correlate file");
                    err = EINVAL;
                    break;
                }

            continue;
        }

        if ((ret = gt_split_text(line, GT_DATA_DELIM, data_row, cnt_columns)) != 0) {
            gt_log("Warning, invalid row in GOET correlate file");
            continue;
        }

        sourceid = strtoul(data_row[sourceid_index], &tail, 10);
        targetid = strtoul(data_row[targetid_index], &tail, 10);
        strncpy(typestr, data_row[type_index], 127);
        weight = strtol(data_row[weight_index], &tail, 10);

		type = gettype(typestr);
		if(type < 0)
			continue;

		ret = add_goet_goet(sourceid, targetid, type, weight);
		/* Ignore ENOENT. */
		if(ret != 0 && ret != ENOENT) {
			err = ENOEXEC;
			break;
		}
	}

	fclose(f);

    if (data_row != NULL) {
        gt_free_split_text(data_row, cnt_columns);
        data_row = NULL;
    }

	if (headers != NULL) {
        gt_free_split_text(headers, cnt_columns);
        headers = NULL;
	}

    if (err == 0) {
        gt_log("Loaded %d GOET correlates.", gt_goetgoet_allcnt);
        gt_log("Max depth of correlate hash is %d.", gt_gci.gi_maxdepth);
    }

	return err;
}

static int
gmd_load_table_goet_correlations()
{
    int error = 0;
	int result = 0;
	gmd_json_reader_t *reader = NULL;

	gmd_json_correlate_t *correlates = NULL;
	int cnt_correlates = 0;
	int size_correlates = 0;

	int i = 0;
	int j = 0;

    char *descriptor_keys[] = {GMD_KEY_ATYPES, GMD_KEY_GENRES, GMD_KEY_ORIGINS, GMD_KEY_ERAS, GMD_KEY_MOODS};
    int descriptor_types[] = {GMD_DESCRIPTOR_TYPE_ATYPE, GMD_DESCRIPTOR_TYPE_GENRE, GMD_DESCRIPTOR_TYPE_ORIGIN, GMD_DESCRIPTOR_TYPE_ERA, GMD_DESCRIPTOR_TYPE_MOOD};

	gt_log("Loading GMD correlate file '%s'", gt_file_correlate);

    for (i=0; i<5; i++) {

	    result = gmd_json_reader_init(&reader, gt_file_correlate, GMD_TYPE_OBJECT, descriptor_keys[i], 2, GT_MAX_SIZE_INPUTSTR, gt_log);
	    if (result != 0) {
		    gt_log("JSON reader failed to initialize: %s", strerror(errno));
			error = ENOENT;
			goto cleanup;
		}

        result = gmd_json_reader_get_correlates(reader, descriptor_types[i], &correlates, &cnt_correlates, &size_correlates);
        if (result != 0)
        	return result;

        result = gmd_json_reader_uninit(reader);
        reader = NULL;

		if (result != 0)
        {
            error = result;
            goto cleanup;
        }

        for (j = 0; j < cnt_correlates; ++j)
        {
            unsigned int id1 = strtoul(correlates[j].id1, NULL, 10);
            unsigned int id2 = strtoul(correlates[j].id2, NULL, 10);
            int weight = atoi(correlates[j].score);

			result = add_goet_goet(id1, id2, correlates[j].type, weight);

			if (result != 0 && result != ENOENT)
            {
                error = result;
                goto cleanup;
            }
        }

        gt_log("Done adding correlates for type: %s", descriptor_keys[i]);
	}

cleanup:

    if (reader) {
        gmd_json_reader_uninit(reader);
    }

    if (correlates != NULL) {
        free(correlates);
	    correlates = NULL;
	}

    if (error == 0)
    {
        gt_log("Loaded %d GOET correlates.", gt_goetgoet_allcnt);
        gt_log("Max depth of correlate hash is %d.", gt_gci.gi_maxdepth);
    }

    return error;
}

static int
gt_load_table_recording_goets()
{
    FILE		*f;
	int		    err;
	char		line[GT_MAXFILELINE];
	unsigned	trackid;
	unsigned	goetid;
	int		    weight;
	char		typestr[128];
	int		    type;
	int		    ret;
	char        **headers = NULL;
	char        **data_row = NULL;
	char        *tail = NULL;
	int         cnt_columns = 0;
	int         goetid_index = 0;
	int         type_index = 0;
	int         weight_index = 0;
	int         trackid_index = 0;
	int         i = 0;

	err = 0;

	gt_log("Loading recording GOET table '%s'",
        gt_file_track_category);

    f = fopen(gt_file_track_category, "r");
    if(f == NULL) {
        gt_log("Can't open file: %s", strerror(errno));
        return ENOENT;
    }

    while(fgets(line, GT_MAXFILELINE, f)) {

        gt_trim_string(line);

        if (headers == NULL) {
            cnt_columns = gt_get_split_text_cnt(line, GT_DATA_DELIM);

            if (cnt_columns < 4) {
                gt_log("Error parsing headers in recording GOET file");
                err = EINVAL;
                break;
            }

             /*allocating once for performance*/
            headers = gt_allocate_split_text(cnt_columns);
            data_row = gt_allocate_split_text(cnt_columns);

            if (headers == NULL || data_row == NULL) {
                gt_log("Error: out of memory!");
                err = ENOMEM;
                break;
            }

            if ((ret = gt_split_text(line, GT_DATA_DELIM, headers, cnt_columns)) != 0) {
                gt_log("Error parsing headers in recording GOET file");
                err = EINVAL;
                break;
            }

            trackid_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_CATEGORY, GT_TABLE_COLUMN_TYPE_RECORDING_ID, headers, cnt_columns);
            goetid_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_CATEGORY, GT_TABLE_COLUMN_TYPE_DESCRIPTOR_ID, headers, cnt_columns);
            weight_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_CATEGORY, GT_TABLE_COLUMN_TYPE_WEIGHT, headers, cnt_columns);
            type_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_CATEGORY, GT_TABLE_COLUMN_TYPE_DESCRIPTOR_TYPE, headers, cnt_columns);

            /*check txt file column names if column not found*/
            if (trackid_index < 0) {
                trackid_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_CATEGORY, GT_TABLE_COLUMN_TYPE_TRACK_ID, headers, cnt_columns);
            }
            if (goetid_index < 0) {
                goetid_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_CATEGORY, GT_TABLE_COLUMN_TYPE_CATEGORY_ID, headers, cnt_columns);
            }
            if (type_index < 0) {
                type_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_CATEGORY, GT_TABLE_COLUMN_TYPE_CATEGORY_TYPE, headers, cnt_columns);
            }

            if (trackid_index < 0 ||
                goetid_index < 0 ||
                weight_index < 0 ||
                type_index < 0) {

                    gt_log("Error parsing headers in recording GOET file");
                    err = EINVAL;
                    break;
                }

            continue;
        }

        if ((ret = gt_split_text(line, GT_DATA_DELIM, data_row, cnt_columns)) != 0) {
            gt_log("Warning, invalid row in recording GOET file");
            continue;
        }

        trackid = strtoul(data_row[trackid_index], &tail, 10);
        goetid = strtoul(data_row[goetid_index], &tail, 10);
        weight = strtol(data_row[weight_index], &tail, 10);
        strncpy(typestr, data_row[type_index], 127);

        type = gettype(typestr);
        if(type < 0)
            continue;

        if (weight == 0)
            continue;

        ret = add_track_goet(trackid, goetid, type, weight);
        if(ret != 0) {
            err = ENOEXEC;
            break;
        }
    }
    fclose(f);

    if (data_row != NULL) {
        gt_free_split_text(data_row, cnt_columns);
        data_row = NULL;
    }

    if (headers != NULL) {
        gt_free_split_text(headers, cnt_columns);
        headers = NULL;
    }

    if (err == 0) {
        gt_log("Loaded %d recordings, %d GOET values.", gt_tracks_cnt,
            gt_trkgoet_allcnt);

        gt_log("Sorting track goet values.");
        for (i = 0; i < gt_tracks_cnt; ++i)
        {
            qsort(gt_tracks[i].gt_goetcorr, gt_tracks[i].gt_goetcorr_cnt, sizeof(gt_goetcorr_t), cmp_goetcorr_by_weight_type);
        }
        gt_log("Done sorting track goet values.");

        qsort(gt_tracks, gt_tracks_cnt, sizeof(gt_track_t),
            cmp_track_by_id);
    }

    return err;
}

#define GMD_REC_2_EDITION_SIZE_INIT     4194304
#define GMD_REC_2_EDITION_SIZE_GROW     262144
static int gmd_load_edition_recordings_data(gmd_preferred_edition_t *gmd_preferred_edition_list, int cnt_preferred_editions, gmd_rec_id_2_pref_edition_id_t **gmd_rec_2_ed_out, int *cnt_rec_2_ed_out)
{
    int error = 0;
    int result = 0;
    gmd_json_reader_t *reader = NULL;
    gmd_rec_id_2_pref_edition_id_t *gmd_rec_2_ed = NULL;
    int cnt_gmd_rec_2_ed = 0;
    int size_gmd_rec_2_ed = 0;
    char **recording_ids  = NULL;
    int cnt_recording_ids = 0;
    char edition_id[GT_MAX_SIZE_INPUTSTR] = {0};
    int i = 0;
    int release_type = -1;
    gmd_preferred_edition_t *edition_found = NULL;
    gmd_preferred_edition_t search_edition;

    gt_log("Loading recording data from GMD album edition file '%s'", gt_file_album_edition);
    error = gmd_json_reader_init(&reader, gt_file_album_edition, GMD_TYPE_ARRAY, GMD_KEY_EDITIONS, 1, GT_MAX_SIZE_INPUTSTR, gt_log);
    if (error != 0) {
        gt_log("JSON reader failed to initialize: %s", strerror(errno));
        return ENOENT;
    }

    size_gmd_rec_2_ed = GMD_REC_2_EDITION_SIZE_INIT;
    gmd_rec_2_ed = (gmd_rec_id_2_pref_edition_id_t *) malloc(sizeof(gmd_rec_id_2_pref_edition_id_t) * size_gmd_rec_2_ed);
    if (gmd_rec_2_ed == NULL)
    {
        error = ENOMEM;
        goto cleanup;
    }

    recording_ids = (char **) malloc(sizeof(char *) * GMD_MAX_RECORDINGS_PER_EDITION);
    if (recording_ids == NULL)
    {
        error = ENOMEM;
        goto cleanup;
    }
    for (i = 0; i < GMD_MAX_RECORDINGS_PER_EDITION; ++i)
    {
        recording_ids[i] = (char *) malloc(sizeof(char) * GMD_GLOBAL_ID_SIZE);
        if (recording_ids[i] == NULL)
        {
            error = ENOMEM;
            goto cleanup;
        }
    }

    while (1)
    {
        result = gmd_json_reader_get_edition_recordings(reader, edition_id, GMD_MAX_RECORDINGS_PER_EDITION, recording_ids, &cnt_recording_ids, &release_type);
        if (result != 0)
        {
            break;
        }

        /* search list of preferred editions to see if we need to load this edition */
        strncpy(search_edition.gmd_id, edition_id, GMD_GLOBAL_ID_SIZE);
        edition_found = bsearch(&search_edition, gmd_preferred_edition_list, cnt_preferred_editions, sizeof(gmd_preferred_edition_t), cmp_gmd_preferred_editions);
        if (edition_found == NULL)
            continue;

        for (i = 0; i < cnt_recording_ids; ++i)
        {
            /* check for reallocation */
            if (cnt_gmd_rec_2_ed == size_gmd_rec_2_ed)
            {
                size_gmd_rec_2_ed += GMD_REC_2_EDITION_SIZE_GROW;
                gmd_rec_2_ed = (gmd_rec_id_2_pref_edition_id_t *) realloc(gmd_rec_2_ed, sizeof(gmd_rec_id_2_pref_edition_id_t) * size_gmd_rec_2_ed);
                if (gmd_rec_2_ed == NULL)
                {
                    error = ENOMEM;
                    goto cleanup;
                }
            }

            strncpy(gmd_rec_2_ed[cnt_gmd_rec_2_ed].gmd_edition_id, edition_id, GMD_GLOBAL_ID_SIZE);
            strncpy(gmd_rec_2_ed[cnt_gmd_rec_2_ed].gmd_recording_id, recording_ids[i], GMD_GLOBAL_ID_SIZE);
            gmd_rec_2_ed[cnt_gmd_rec_2_ed].release_type = release_type;
            cnt_gmd_rec_2_ed++;
        }
    }

cleanup:
    if (recording_ids)
    {
        for (i = 0; i < GMD_MAX_RECORDINGS_PER_EDITION; ++i)
        {
            if (recording_ids[i])
                free(recording_ids[i]);
        }
        free(recording_ids);
    }

    if (reader)
    {
        result = gmd_json_reader_uninit(reader);
    }

    if (error != 0)
    {
        if (gmd_rec_2_ed)
            free(gmd_rec_2_ed);
    }
    else
    {
        gt_log("Loaded %d preferred edition id to recording id mappings.", cnt_gmd_rec_2_ed);
        gt_log("Sorting preferred edition id to recording id by recording id.");
        qsort(gmd_rec_2_ed, cnt_gmd_rec_2_ed, sizeof(gmd_rec_id_2_pref_edition_id_t), cmp_gmd_rec_2_preferred_edition_id_w_release_type);
        gt_log("Done sorting preferred edition id to recording ids.");
        *gmd_rec_2_ed_out = gmd_rec_2_ed;
        *cnt_rec_2_ed_out = cnt_gmd_rec_2_ed;
    }

    return error;
}

#define GMD_EDITIONS_SIZE_INIT  262144
#define GMD_EDITIONS_SIZE_GROW  65536
static int gmd_load_one_edition_per_master(gmd_preferred_edition_t **gmd_preferred_edition_list_out, int *cnt_preferred_editions_out)
{
    int error = 1;
    int result = 0;
    int index_master = 0, index_edition = 0;
    gmd_json_reader_t *reader_editions = NULL;
    gmd_json_reader_t *reader_masters = NULL;
    gmd_preferred_edition_t *gmd_preferred_edition_list = NULL;
    gmd_master_data_t *gmd_master_list = NULL;
    gmd_edition_master_t *edition_master_list = NULL;
    char master_name[GT_MAX_SIZE_INPUTSTR] = {0};
    char edition_name[GT_MAX_SIZE_INPUTSTR] = {0};
    char edition_id[GT_MAX_SIZE_INPUTSTR] = {0};
    char master_id[GT_MAX_SIZE_INPUTSTR] = {0};
    char release_year[GT_MAX_SIZE_INPUTSTR] = {0};

    int cnt_edition_master = 0;
    int size_edition_master = 0;
    int cnt_master_data = 0;
    int size_master_data = 0;
    int cnt_preferred_editions = 0;
    int size_preferred_editions = 0;

    gt_log("Loading all editions from GMD edition file '%s'", gt_file_album_edition);
    result = gmd_json_reader_init(&reader_editions, gt_file_album_edition, GMD_TYPE_ARRAY, GMD_KEY_EDITIONS, 1, GT_MAX_SIZE_INPUTSTR, gt_log);
    if (result != 0) {
        gt_log("JSON reader failed to initialize: %s", strerror(errno));
        goto cleanup;
    }

    size_edition_master = GMD_EDITIONS_SIZE_INIT;
    edition_master_list = (gmd_edition_master_t *) malloc(sizeof(gmd_edition_master_t) * size_edition_master);
    if (edition_master_list == NULL)
    {
        error = ENOMEM;
        goto cleanup;
    }

    while(1)
    {
        edition_id[0] = '\0';
        edition_name[0] = '\0';
        master_id[0] = '\0';

        result = gmd_json_reader_get_edition_data(reader_editions, edition_id, master_id, edition_name);
        if (result != 0)
            break;

        if (cnt_edition_master == size_edition_master)
        {
            size_edition_master += GMD_EDITIONS_SIZE_GROW;
            edition_master_list = (gmd_edition_master_t *) realloc(edition_master_list, sizeof(gmd_edition_master_t) * size_edition_master);
            if (edition_master_list == NULL)
            {
                error = ENOMEM;
                goto cleanup;
            }
        }

        strncpy(edition_master_list[cnt_edition_master].gmd_edition_id, edition_id, GMD_GLOBAL_ID_SIZE);
        strncpy(edition_master_list[cnt_edition_master].gmd_master_id, master_id, GMD_GLOBAL_ID_SIZE);
        cnt_edition_master++;
    }
    gt_log("Total number of edition to master links: %d", cnt_edition_master);
    /* sort list by master id */
    gt_log("Sorting editions...");
    qsort(edition_master_list, cnt_edition_master, sizeof(gmd_edition_master_t), cmp_edition_master);
    gt_log("Done sorting editions!");

    /* load data from masters file */
    gt_log("Loading all masters from GMD master file '%s'", gt_file_album_master);
    result = gmd_json_reader_init(&reader_masters, gt_file_album_master, GMD_TYPE_ARRAY, GMD_KEY_MASTERS, 1, GT_MAX_SIZE_INPUTSTR, gt_log);
    if (result != 0) {
        gt_log("JSON reader failed to initialize: %s", strerror(errno));
        goto cleanup;
    }

    size_master_data = GMD_EDITIONS_SIZE_INIT;
    gmd_master_list = (gmd_master_data_t *) malloc(sizeof(gmd_master_data_t) * size_master_data);
    if (gmd_master_list == NULL)
    {
        error = ENOMEM;
        goto cleanup;
    }

    while (1)
    {
        master_name[0] = '\0';
        master_id[0] = '\0';
        release_year[0] = '\0';

        result = gmd_json_reader_get_master_data(reader_masters, master_id, master_name, release_year);
        if (result != 0)
            break;

        if (cnt_master_data == size_master_data)
        {
            size_master_data += GMD_EDITIONS_SIZE_GROW;
            gmd_master_list = (gmd_master_data_t *) realloc(gmd_master_list, sizeof(gmd_master_data_t) * size_master_data);
            if (gmd_master_list == NULL)
            {
                error = ENOMEM;
                goto cleanup;
            }
        }

        strncpy(gmd_master_list[cnt_master_data].gmd_master_id, master_id, GMD_GLOBAL_ID_SIZE);
        strncpy(gmd_master_list[cnt_master_data].gmd_name, master_name, GT_MAX_SIZE_INPUTSTR);
        strncpy(gmd_master_list[cnt_master_data].gmd_release_year, release_year, GT_MAX_SIZE_RELEASEYEAR);
        cnt_master_data++;
    }
    gt_log("Loaded %d masters from file.", cnt_master_data);
    gt_log("Sorting master data...");
    qsort(gmd_master_list, cnt_master_data, sizeof(gmd_master_data_t), cmp_master_data);
    gt_log("Done sorting master data!");

    gt_log("Picking one edition per master...");
    size_preferred_editions = GMD_EDITIONS_SIZE_INIT;
    gmd_preferred_edition_list = (gmd_preferred_edition_t *) malloc(sizeof(gmd_preferred_edition_t) * size_preferred_editions);
    if (gmd_preferred_edition_list == NULL)
    {
        error = ENOMEM;
        goto cleanup;
    }
    index_edition = 0;
    for (index_master = 0; index_master < cnt_master_data; ++index_master)
    {
        /* find next matching master id in editions list */
        while (
               index_edition < cnt_edition_master &&
               strcmp(edition_master_list[index_edition].gmd_master_id, gmd_master_list[index_master].gmd_master_id) < 0
        )
        {
            index_edition++;
        }

        if (index_edition >= cnt_edition_master)
            break;


        if (strcmp(edition_master_list[index_edition].gmd_master_id, gmd_master_list[index_master].gmd_master_id) > 0)
            continue;

        if (cnt_preferred_editions == size_preferred_editions)
        {
            size_preferred_editions += GMD_EDITIONS_SIZE_GROW;
            gmd_preferred_edition_list = (gmd_preferred_edition_t *) realloc(gmd_preferred_edition_list, sizeof(gmd_preferred_edition_t) * size_preferred_editions);
            if (gmd_preferred_edition_list == NULL)
            {
                error = ENOMEM;
                goto cleanup;
            }
        }

        gmd_preferred_edition_list[cnt_preferred_editions].numerical_id = add_gmd_id(edition_master_list[index_edition].gmd_edition_id);
        strncpy(gmd_preferred_edition_list[cnt_preferred_editions].gmd_id, edition_master_list[index_edition].gmd_edition_id, GMD_GLOBAL_ID_SIZE);
        strncpy(gmd_preferred_edition_list[cnt_preferred_editions].gmd_release_year, gmd_master_list[index_master].gmd_release_year, GT_MAX_SIZE_RELEASEYEAR);
        strncpy(gmd_preferred_edition_list[cnt_preferred_editions].gmd_name, gmd_master_list[index_master].gmd_name, GT_MAX_SIZE_INPUTSTR);
        cnt_preferred_editions++;

    }
    error = 0;
    gt_log("Done picking one edition per master!");

cleanup:
    if (reader_editions)
    {
        result = gmd_json_reader_uninit(reader_editions);
        reader_editions = NULL;
    }

    if (reader_masters)
    {
        result = gmd_json_reader_uninit(reader_masters);
        reader_masters = NULL;
    }

    if (edition_master_list)
    {
        free(edition_master_list);
        edition_master_list = NULL;
    }
    if (gmd_master_list != NULL)
    {
        free(gmd_master_list);
        gmd_master_list = NULL;
    }

    if (error != 0)
    {
        if (gmd_preferred_edition_list)
        {
            free(gmd_preferred_edition_list);
            gmd_preferred_edition_list = NULL;
        }
    }
    else
    {
        gt_log("Loaded %d preferred editions.", cnt_preferred_editions);

        gt_log("Sorting preferred editions by id.");
        qsort(gmd_preferred_edition_list, cnt_preferred_editions, sizeof(gmd_preferred_edition_t), cmp_gmd_preferred_editions);
        gt_log("Done sorting preferred editions by id!");

        *gmd_preferred_edition_list_out = gmd_preferred_edition_list;
        *cnt_preferred_editions_out = cnt_preferred_editions;
    }
    return error;

}

static int gmd_load_preferred_editions(gmd_preferred_edition_t **gmd_preferred_edition_list_out, int *cnt_preferred_editions_out)
{
    int error = 0;
    int result = 0;
    gmd_json_reader_t *reader = NULL;
    gmd_preferred_edition_t *gmd_preferred_edition_list = NULL;
    char preferred_edition_id[GT_MAX_SIZE_INPUTSTR] = {0};
    char master_name[GT_MAX_SIZE_INPUTSTR] = {0};
    char master_id[GT_MAX_SIZE_INPUTSTR] = {0};
    char release_year[GT_MAX_SIZE_INPUTSTR] = {0};
    int cnt_preferred_editions = 0;
    int size_preferred_editions = 0;

    gt_log("Loading preferred editions from GMD album master file '%s'", gt_file_album_master);
    error = gmd_json_reader_init(&reader, gt_file_album_master, GMD_TYPE_ARRAY, GMD_KEY_MASTERS, 1, GT_MAX_SIZE_INPUTSTR, gt_log);
    if (error != 0) {
        gt_log("JSON reader failed to initialize: %s", strerror(errno));
        return ENOENT;
    }

    size_preferred_editions = GMD_EDITIONS_SIZE_INIT;
    gmd_preferred_edition_list = (gmd_preferred_edition_t *) malloc(sizeof(gmd_preferred_edition_t) * size_preferred_editions);
    if (gmd_preferred_edition_list == NULL)
    {
        error = ENOMEM;
        goto cleanup;
    }

    while (1)
    {
        release_year[0] = '\0';
        result = gmd_json_reader_get_master_preferred_edition(reader, preferred_edition_id, master_name, master_id, release_year);
        if (result != 0)
            break;

        /* check reallocation */
        if (cnt_preferred_editions == size_preferred_editions)
        {
            size_preferred_editions += GMD_EDITIONS_SIZE_GROW;
            gmd_preferred_edition_list = (gmd_preferred_edition_t *) realloc(gmd_preferred_edition_list, sizeof(gmd_preferred_edition_t) * size_preferred_editions);
            if (gmd_preferred_edition_list == NULL)
            {
                error = ENOMEM;
                goto cleanup;
            }
        }

        gmd_preferred_edition_list[cnt_preferred_editions].numerical_id = add_gmd_id(preferred_edition_id);
        strncpy(gmd_preferred_edition_list[cnt_preferred_editions].gmd_id, preferred_edition_id, GMD_GLOBAL_ID_SIZE);
        strncpy(gmd_preferred_edition_list[cnt_preferred_editions].gmd_release_year, release_year, GT_MAX_SIZE_RELEASEYEAR);
        strncpy(gmd_preferred_edition_list[cnt_preferred_editions].gmd_name, master_name, GT_MAX_SIZE_INPUTSTR);

        cnt_preferred_editions++;
    }

cleanup:
    if (reader)
    {
        result = gmd_json_reader_uninit(reader);
        reader = NULL;
    }

    if (error != 0)
    {
        free(gmd_preferred_edition_list);
    }
    else
    {
        gt_log("Loaded %d preferred editions.", cnt_preferred_editions);

        gt_log("Sorting preferred editions by id.");
        qsort(gmd_preferred_edition_list, cnt_preferred_editions, sizeof(gmd_preferred_edition_t), cmp_gmd_preferred_editions);
        gt_log("Done sorting preferred editions by id!");

        *gmd_preferred_edition_list_out = gmd_preferred_edition_list;
        *cnt_preferred_editions_out = cnt_preferred_editions;
    }

    return error;
}

#define GMD_RECORDING_DATA_SIZE_INIT    1048576
#define GMD_RECORDING_DATA_SIZE_GROW    131072
static int
gmd_load_recordings_to_use(gmd_recording_data_t **gmd_recordings_list_out,
                             int *cnt_recordings_out,
                             gmd_rec_id_2_pref_edition_id_t *rec2editions,
                             int cnt_rec2editions,
                             gmd_preferred_edition_t *editions_list,
                             int cnt_editions,
                             int do_one_recording_per_song)
{
    int error = 0;
    int result = 0;
    gmd_json_reader_t *reader = NULL;
    char recording_id[GT_MAX_SIZE_INPUTSTR] = {0};
    char song_id[GT_MAX_SIZE_INPUTSTR] = {0};
    char release_year[GT_MAX_SIZE_INPUTSTR] = {0};
    char recording_name[GT_MAX_SIZE_INPUTSTR] = {0};
    char artist_id[GT_MAX_SIZE_INPUTSTR] = {0};
    char artist_name[GT_MAX_SIZE_INPUTSTR] = {0};
    gmd_json_descriptor_t descriptors[GMD_DESCRIPTOR_MAX_SIZE];
    int cnt_descriptors = 0;
    int has_genre = 0;
    gmd_recording_data_t *gmd_recordings_list = NULL;
    int cnt_recordings = 0;
    int size_recordings = 0;
    int i = 0;
    int cnt_best_recordings = 0;
    char *prev_song_id = NULL;
    gmd_rec_id_2_pref_edition_id_t *found_rec2edition = NULL;
    gmd_rec_id_2_pref_edition_id_t search_rec2edition;
    gmd_preferred_edition_t *found_edition = NULL;
    gmd_preferred_edition_t search_edition;
    int recording_index = 0;

    gt_log("Loading recording data from GMD recording file '%s'", gt_file_track_list);
    result = gmd_json_reader_init(&reader, gt_file_track_list, GMD_TYPE_ARRAY, GMD_KEY_RECORDINGS, 1, GT_MAX_SIZE_INPUTSTR, gt_log);
    if (result != 0) {
        gt_log("JSON reader failed to initialize: %s", strerror(errno));
        return ENOENT;
    }

    gmd_recordings_list = (gmd_recording_data_t *) malloc(sizeof(gmd_recording_data_t) * GMD_RECORDING_DATA_SIZE_INIT);
    if (gmd_recordings_list == NULL)
    {
        error = ENOMEM;
        goto cleanup;
    }
    size_recordings = GMD_RECORDING_DATA_SIZE_INIT;

    while (1)
    {
        has_genre = 0;
        release_year[0] = '\0';
        result = gmd_json_reader_get_recording_info(reader,
                                                    recording_id,
                                                    recording_name,
                                                    artist_id,
                                                    artist_name,
                                                    release_year,
                                                    song_id,
                                                    GMD_DESCRIPTOR_MAX_SIZE,
                                                    descriptors,
                                                    &cnt_descriptors
                                                    );

        if (result != 0)
            break;


        /* ignore recordings with no genre */
        for (i = 0; i < cnt_descriptors; ++i)
        {
            if (descriptors[i].type == GT_TYPE_GENRE)
            {
                has_genre = 1;
                break;
            }
        }

        if (!has_genre)
            continue;

        /* look up edition id, ignore recordings with no preferred edition */
        strncpy(search_rec2edition.gmd_recording_id, recording_id, GMD_GLOBAL_ID_SIZE);
        found_rec2edition = (gmd_rec_id_2_pref_edition_id_t *) bsearch(&search_rec2edition, rec2editions, cnt_rec2editions, sizeof(gmd_rec_id_2_pref_edition_id_t), cmp_gmd_rec_2_preferred_edition_id_wout_release_type);
        if (found_rec2edition == NULL)
        {
            /*gt_log("Warning, could not find edition for recording id %s!", recording_id);*/
            continue;
        }

        /* look up edition release year */
        strncpy(search_edition.gmd_id, found_rec2edition->gmd_edition_id, GMD_GLOBAL_ID_SIZE);
        found_edition = (gmd_preferred_edition_t *) bsearch(&search_edition, editions_list, cnt_editions, sizeof(gmd_preferred_edition_t), cmp_gmd_preferred_editions);
        if (found_edition == NULL)
        {
            gt_log("Error: could not find edition info for edition id %s!", found_rec2edition->gmd_edition_id);
            break;
        }

        /* reallocate */
        if (cnt_recordings == size_recordings)
        {
            size_recordings += GMD_RECORDING_DATA_SIZE_GROW;
            gmd_recordings_list = (gmd_recording_data_t *) realloc(gmd_recordings_list, sizeof(gmd_recording_data_t) * size_recordings);
            if (gmd_recordings_list == NULL)
            {
                error = ENOMEM;
                goto cleanup;
            }
        }

        strncpy(gmd_recordings_list[cnt_recordings].gmd_recording_id, recording_id, GMD_GLOBAL_ID_SIZE);
        strncpy(gmd_recordings_list[cnt_recordings].gmd_song_id, song_id, GMD_GLOBAL_ID_SIZE);
        /*if release year not set in recording, use release year from preferred edition */
        if (strlen(release_year) == 0 || strcmp(release_year, "null") == 0)
            strncpy(gmd_recordings_list[cnt_recordings].gmd_release_year, found_edition->gmd_release_year, GT_MAX_SIZE_RELEASEYEAR);
        else
            strncpy(gmd_recordings_list[cnt_recordings].gmd_release_year, release_year, GT_MAX_SIZE_RELEASEYEAR);
        gmd_recordings_list[cnt_recordings].b_use_recording = 0;
        gmd_recordings_list[cnt_recordings].index = recording_index;

        cnt_recordings++;
        recording_index++;
    }

cleanup:
    if (reader)
    {
        gmd_json_reader_uninit(reader);
    }

    if (error != 0)
    {
        if (gmd_recordings_list)
            free(gmd_recordings_list);
    }
    else
    {
        gt_log("Loaded %d recordings!", cnt_recordings);

        /* sort recordings by song id, then release year, then recording id */
        gt_log("Sorting GMD recording data.");
        qsort(gmd_recordings_list, cnt_recordings, sizeof(gmd_recording_data_t), cmp_gmd_recording_data);
        gt_log("Done sorting GMD recording data!");

        if (do_one_recording_per_song) {
	        gt_log("Finding best recordings.");
	        prev_song_id = NULL;
	        for (i = 0; i < cnt_recordings; ++i) {
	        	if (prev_song_id == NULL || strcmp(gmd_recordings_list[i].gmd_song_id, prev_song_id) != 0) {
	        		gmd_recordings_list[i].b_use_recording = 1;
	        		prev_song_id = gmd_recordings_list[i].gmd_song_id;
	        		cnt_best_recordings++;
	        	}
	        }
	        gt_log("Found %d best recordings!", cnt_best_recordings);

	    }
	    else {
	        for (i = 0; i < cnt_recordings; ++i)
	        	gmd_recordings_list[i].b_use_recording = 1;
	    }

        /* sort by index so recordings are in order they were read in */
	    gt_log("Sorting GMD recording data.");
        qsort(gmd_recordings_list, cnt_recordings, sizeof(gmd_recording_data_t), cmp_gmd_recording_index);
        gt_log("Done sorting GMD recording data!");

        *gmd_recordings_list_out = gmd_recordings_list;
        *cnt_recordings_out = cnt_recordings;
    }

    return error;
}

static int gmd_load_recording_data(gmd_recording_data_t *gmd_recordings_list,
                                   int cnt_recordings,
                                   gmd_rec_id_2_pref_edition_id_t *rec2editions,
                                   int cnt_rec2editions,
                                   gmd_preferred_edition_t *editions_list,
                                   int cnt_editions,
                                   int do_one_recording_per_song)
{
    int error = 0;
    int result = 0;
    int recording_index = 0;
    gmd_json_reader_t *reader = NULL;
    char recording_id[GT_MAX_SIZE_INPUTSTR] = {0};
    char song_id[GT_MAX_SIZE_INPUTSTR] = {0};
    char release_year[GT_MAX_SIZE_INPUTSTR] = {0};
    char recording_name[GT_MAX_SIZE_INPUTSTR] = {0};
    char artist_id[GT_MAX_SIZE_INPUTSTR] = {0};
    char artist_name[GT_MAX_SIZE_INPUTSTR] = {0};
    gmd_json_descriptor_t descriptors[GMD_DESCRIPTOR_MAX_SIZE];
    int cnt_descriptors = 0;
    int has_genre = 0;
    unsigned int recording_id_num = 0;
    unsigned int album_id_num = 0;
    unsigned int artist_id_num = 0;
    gmd_recording_data_t *current_recording = NULL;
    gmd_id_mapping_t *artist_album_id_mappings = NULL;
    gmd_id_mapping_t *found_mapping = NULL;
    gmd_id_mapping_t search_mapping;
    int cnt_artist_album_id_mappings = 0;
    int release_year_num = 0;
    int i = 0;
    gmd_rec_id_2_pref_edition_id_t *found_rec2edition = NULL;
    gmd_rec_id_2_pref_edition_id_t search_rec2edition;
    gmd_preferred_edition_t *found_edition = NULL;
    gmd_preferred_edition_t search_edition;
    gt_goetcorr_t goets[GMD_DESCRIPTOR_MAX_SIZE];

    gt_log("Setting recording data.");
    result = gmd_json_reader_init(&reader, gt_file_track_list, GMD_TYPE_ARRAY, GMD_KEY_RECORDINGS, 1, GT_MAX_SIZE_INPUTSTR, gt_log);
    if (result != 0) {
        gt_log("JSON reader failed to initialize: %s", strerror(errno));
        return ENOENT;
    }

    /* allocate array of id mappings for album and artist id lookup */
    cnt_artist_album_id_mappings = id_count;
    artist_album_id_mappings = (gmd_id_mapping_t *) malloc(sizeof(gmd_id_mapping_t) * cnt_artist_album_id_mappings);
    if (artist_album_id_mappings == NULL)
        goto cleanup;
    memcpy(artist_album_id_mappings, id_to_gmd, sizeof(gmd_id_mapping_t) * cnt_artist_album_id_mappings);
    qsort(artist_album_id_mappings, cnt_artist_album_id_mappings, sizeof(gmd_id_mapping_t), cmp_gmd_id_mapping);


    while (1)
    {
        has_genre = 0;
        release_year[0] = '\0';
        result = gmd_json_reader_get_recording_info(reader,
                                                    recording_id,
                                                    recording_name,
                                                    artist_id,
                                                    artist_name,
                                                    release_year,
                                                    song_id,
                                                    GMD_DESCRIPTOR_MAX_SIZE,
                                                    descriptors,
                                                    &cnt_descriptors
                                                    );

        if (result != 0)
            break;


        /* ignore recordings with no genre */
        for (i = 0; i < cnt_descriptors; ++i)
        {
            if (descriptors[i].type == GT_TYPE_GENRE)
            {
                has_genre = 1;
                break;
            }
        }

        if (!has_genre)
            continue;

        current_recording = &gmd_recordings_list[recording_index];
        if (strcmp(current_recording->gmd_recording_id, recording_id) != 0)
            continue;

        if (current_recording->b_use_recording)
        {
            /* look up edition id, ignore recordings with no preferred edition */
            strncpy(search_rec2edition.gmd_recording_id, recording_id, GMD_GLOBAL_ID_SIZE);
            found_rec2edition = (gmd_rec_id_2_pref_edition_id_t *) bsearch(&search_rec2edition, rec2editions, cnt_rec2editions, sizeof(gmd_rec_id_2_pref_edition_id_t), cmp_gmd_rec_2_preferred_edition_id_wout_release_type);
            if (found_rec2edition == NULL)
            {
                /*gt_log("Warning, could not find edition for recording id %s!", recording_id);*/
                continue;
            }

            /*walk back to find first in list with matching recording id to get best edition based on release type*/
            while(found_rec2edition > rec2editions && strcmp(recording_id-1, found_rec2edition->gmd_recording_id) == 0)
            {
                found_rec2edition--;
            }

            /* look up edition to get album name and album id */
            strncpy(search_edition.gmd_id, found_rec2edition->gmd_edition_id, GMD_GLOBAL_ID_SIZE);
            found_edition = (gmd_preferred_edition_t *) bsearch(&search_edition, editions_list, cnt_editions, sizeof(gmd_preferred_edition_t), cmp_gmd_preferred_editions);
            if (found_edition == NULL)
            {
                gt_log("Error: could not find edition info for edition id %s!", found_rec2edition->gmd_edition_id);
                break;
            }

            /* look up artist id num */
            strncpy(search_mapping.gmd_id, artist_id, GMD_GLOBAL_ID_SIZE);
            found_mapping = (gmd_id_mapping_t *) bsearch(&search_mapping, artist_album_id_mappings, cnt_artist_album_id_mappings, sizeof(gmd_id_mapping_t), cmp_gmd_id_mapping);
            if (found_mapping == NULL)
            {
                /* there are going to be tracks without artist ids since we only load artists with genres */
                /*gt_log("Warning: could not find artist id: %s", artist_id);*/
                artist_id_num = add_gmd_id(artist_id);
            }
            else
                artist_id_num = found_mapping->numerical_id;

            /* look up album id */
            strncpy(search_mapping.gmd_id, found_edition->gmd_id, GMD_GLOBAL_ID_SIZE);
            found_mapping = (gmd_id_mapping_t *) bsearch(&search_mapping, artist_album_id_mappings, cnt_artist_album_id_mappings, sizeof(gmd_id_mapping_t), cmp_gmd_id_mapping);
            if (found_mapping == NULL)
            {
                gt_log("Warning: could not find album id: %s", found_edition->gmd_id);
                break;
            }
            album_id_num = found_mapping->numerical_id;

            /* add recording id to global id mapping list */
            /* if using one recording per song, use song id */
            if (do_one_recording_per_song)
                recording_id_num = add_gmd_id(current_recording->gmd_song_id);
            else
                recording_id_num = add_gmd_id(current_recording->gmd_recording_id);

            /* take release year from recording then edition */
            release_year_num = 0;
            if (strlen(release_year) > 0 && strcmp(release_year, "null") != 0)
            {
                release_year_num = atoi(release_year);
            }
            if (release_year_num == 0 && strlen(found_edition->gmd_release_year) > 0 && strcmp(release_year, "null") != 0)
            {
                release_year_num = atoi(found_edition->gmd_release_year);
            }

            /* set goet array */
            for (i = 0; i < cnt_descriptors; ++i)
            {
                goets[i].gc_id = strtoul(descriptors[i].id, NULL, 10);
                goets[i].gc_type = descriptors[i].type;
                goets[i].gc_weight = atoi(descriptors[i].weight);
            }

            /* add recording to global data structures */
            error = add_track_goet_and_mdata(recording_id_num,
                                             artist_id_num,
                                             artist_name,
                                             recording_name,
                                             album_id_num,
                                             found_edition->gmd_name,
                                             release_year_num,
                                             goets,
                                             cnt_descriptors);

            if (error)
                break;

        }
        recording_index++;
    }

cleanup:
    if (reader)
    {
        gmd_json_reader_uninit(reader);
    }

    if (artist_album_id_mappings)
        free(artist_album_id_mappings);

    if (error == 0)
    {
        gt_log("Loaded %d recordings, %d GOET values.", gt_tracks_cnt,
            gt_trkgoet_allcnt);

        gt_log("Sorting track goet values.");
        for (i = 0; i < gt_tracks_cnt; ++i)
        {
            qsort(gt_tracks[i].gt_goetcorr, gt_tracks[i].gt_goetcorr_cnt, sizeof(gt_goetcorr_t), cmp_goetcorr_by_weight_type);
        }
        gt_log("Done sorting track goet values.");
        gt_log("Loaded metadata for %d recordings.", gt_trkmdata_allcnt);
    }

    return error;
}

static int
gt_load_table_recording_names()
{
    FILE		*f;
	int		    err;
	char		line[GT_MAXFILELINE];
	unsigned	artistid;
	unsigned	trackid;
	unsigned    albumid;
	int		    year;
	int		    ret;
	char		trackname[GT_MAX_SIZE_INPUTSTR];
	char		albumname[GT_MAX_SIZE_INPUTSTR];
	char        artistname[GT_MAX_SIZE_INPUTSTR];
	char        **headers = NULL;
	char        **data_row = NULL;
	char        *tail = NULL;
	int         cnt_columns = 0;
	int         artistid_index = 0;
	int         trackid_index = 0;
	int         trackname_index = 0;
	int         albumname_index = 0;
	int         year_index = 0;
	int         albumid_index = 0;
    int         artistname_index = 0;

	err = 0;

	gt_log("Loading recording metadata '%s'", gt_file_track_list);

    f = fopen(gt_file_track_list, "r");
    if(f == NULL) {
        gt_log("Can't open file: %s", strerror(errno));
        return ENOENT;
    }

    while(fgets(line, GT_MAXFILELINE, f)) {

        gt_trim_string(line);

        if (headers == NULL) {
            cnt_columns = gt_get_split_text_cnt(line, GT_DATA_DELIM);

            if (cnt_columns < 3) {
                gt_log("Error parsing headers in recording metadata file.");
                err = EINVAL;
                break;
            }

            /*allocating once for performance*/
            headers = gt_allocate_split_text(cnt_columns);
            data_row = gt_allocate_split_text(cnt_columns);

            if (headers == NULL || data_row == NULL) {
                gt_log("Error: out of memory!");
                err = ENOMEM;
                break;
            }

            if ((ret = gt_split_text(line, GT_DATA_DELIM, headers, cnt_columns)) != 0) {
                gt_log("Error parsing headers in recording metadata file.");
                err = EINVAL;
                break;
            }

            trackid_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_LIST, GT_TABLE_COLUMN_TYPE_RECORDING_ID, headers, cnt_columns);
            artistid_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_LIST, GT_TABLE_COLUMN_TYPE_ARTIST_ID, headers, cnt_columns);
            trackname_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_LIST, GT_TABLE_COLUMN_TYPE_RECORDING_NAME, headers, cnt_columns);
            albumname_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_LIST, GT_TABLE_COLUMN_TYPE_ALBUM_NAME, headers, cnt_columns);
            year_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_LIST, GT_TABLE_COLUMN_TYPE_YEAR, headers, cnt_columns);
            albumid_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_LIST, GT_TABLE_COLUMN_TYPE_ALBUM_ID, headers, cnt_columns);
            artistname_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_LIST, GT_TABLE_COLUMN_TYPE_ARTIST_NAME, headers, cnt_columns);


            /*check txt file column names if column not found*/
            if (trackid_index < 0) {
                trackid_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_LIST, GT_TABLE_COLUMN_TYPE_TRACK_ID, headers, cnt_columns);
            }
            if (trackname_index < 0) {
                trackname_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_LIST, GT_TABLE_COLUMN_TYPE_TRACK_NAME, headers, cnt_columns);
            }
            if (year_index < 0) {
                year_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_LIST, GT_TABLE_COLUMN_TYPE_RELEASE_YEAR, headers, cnt_columns);
            }

            if (trackid_index < 0 ||
                artistid_index < 0 ||
                trackname_index < 0) {

                    gt_log("Error parsing headers in recording metadata table");
                    err = EINVAL;
                    break;
                }

            continue;
        }

        if ((ret = gt_split_text(line, GT_DATA_DELIM, data_row, cnt_columns)) != 0) {
            gt_log("Warning, invalid row in recording metadata table");
            continue;
        }

        trackid = strtoul(data_row[trackid_index], &tail, 10);
        artistid = strtoul(data_row[artistid_index], &tail, 10);
        strncpy(trackname, data_row[trackname_index], GT_MAX_SIZE_INPUTSTR - 1);

        if (albumname_index < 0)
            albumname[0] = 0;
        else
            strncpy(albumname, data_row[albumname_index], GT_MAX_SIZE_INPUTSTR - 1);

        if (year_index < 0)
            year = 0;
        else
            year = strtol(data_row[year_index], &tail, 10);

        if (albumid_index < 0)
            albumid = 0;
        else
            albumid = strtoul(data_row[albumid_index], &tail, 10);

        if (artistname_index < 0)
            artistname[0] = 0;
        else
            strncpy(artistname, data_row[artistname_index], GT_MAX_SIZE_INPUTSTR - 1);

        ret = add_track_mdata(trackid, artistid, artistname,
            trackname, albumid, albumname, year);

        /* Ignore ENOENT */
        if(ret != 0 && ret != ENOENT) {
            err = ret;
            break;
        }
    }

    fclose(f);

    if (data_row != NULL) {
        gt_free_split_text(data_row, cnt_columns);
        data_row = NULL;
    }

    if (headers != NULL) {
        gt_free_split_text(headers, cnt_columns);
        headers = NULL;
    }

    if (err == 0) {
        gt_log("Loaded metadata for %d recordings.", gt_trkmdata_allcnt);
    }

    return err;
}

static int
gt_load_table_recording_popularities()
{
    FILE		*f;
	int		    err;
	char		line[GT_MAXFILELINE];
	unsigned	trackid;
	int		    ret;
	unsigned	pop;
	char        **headers = NULL;
	char        **data_row = NULL;
	char        *tail = NULL;
	int         cnt_columns = 0;
	int         pop_index = 0;
	int         trackid_index = 0;

	err = 0;

    gt_log("Loading recording popularities '%s'",
           gt_file_track_popularity);

    f = fopen(gt_file_track_popularity, "r");
    if(f == NULL) {
        gt_log("Can't open file: %s", strerror(errno));
        return ENOENT;
    }

    while(fgets(line, GT_MAXFILELINE, f)) {

        gt_trim_string(line);

        if (headers == NULL) {
            cnt_columns = gt_get_split_text_cnt(line, GT_DATA_DELIM);

            if (cnt_columns < 2) {
                gt_log("Error parsing headers in recording popularity file");
                err = EINVAL;
                break;
            }

            /*allocating once for performance*/
            headers = gt_allocate_split_text(cnt_columns);
            data_row = gt_allocate_split_text(cnt_columns);

            if (headers == NULL || data_row == NULL) {
                gt_log("Error: out of memory!");
                err = ENOMEM;
                break;
            }

            if ((ret = gt_split_text(line, GT_DATA_DELIM, headers, cnt_columns)) != 0) {
                gt_log("Error parsing headers in recording popularity file");
                err = EINVAL;
                break;
            }

            trackid_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_POPULARITY, GT_TABLE_COLUMN_TYPE_RECORDING_ID, headers, cnt_columns);
            pop_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_POPULARITY, GT_TABLE_COLUMN_TYPE_POPULARITY, headers, cnt_columns);

            if (trackid_index < 0) {
                trackid_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_POPULARITY, GT_TABLE_COLUMN_TYPE_TRACK_ID, headers, cnt_columns);
            }

            if (trackid_index < 0) {
                trackid_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_POPULARITY, GT_TABLE_COLUMN_TYPE_GN_ID, headers, cnt_columns);
            }

            if (pop_index < 0) {
                pop_index = gt_get_column_index(GT_TABLE_TYPE_TRACK_POPULARITY, GT_TABLE_COLUMN_TYPE_NEW_POPULARITY, headers, cnt_columns);
            }

            if (trackid_index < 0 ||
                pop_index < 0) {

                    gt_log("Error parsing headers in recording popularity table");
                    err = EINVAL;
                    break;
                }

            continue;
        }

        if ((ret = gt_split_text(line, GT_DATA_DELIM, data_row, cnt_columns)) != 0) {
            gt_log("Warning, invalid row in recording popularity table");
            continue;
        }

        trackid = strtoul(data_row[trackid_index], &tail, 10);
        pop = strtoul(data_row[pop_index], &tail, 10);

        ret = add_track_popularity(trackid, pop);
        /* Ignore ENOENT. */
        if(ret != 0 && ret != ENOENT) {
            err = ret;
            break;
        }
    }

    fclose(f);

    if (data_row != NULL) {
        gt_free_split_text(data_row, cnt_columns);
        data_row = NULL;
    }

    if (headers != NULL) {
        gt_free_split_text(headers, cnt_columns);
        headers = NULL;
    }

    if (err == 0) {
        gt_log("Loaded %d recording popularities.",
            gt_trkpop_allcnt);
    }

    return err;
}

static int
gt_load_table_hierarchies()
{
    FILE		*f;
	int		    err;
	char		line[GT_MAXFILELINE];
	int		    ret;
	unsigned	hierid;
	char		hiername[GT_MAX_SIZE_INPUTSTR];
	char        **headers = NULL;
	char        **data_row = NULL;
	char        *tail = NULL;
	int         cnt_columns = 0;
	int         hierid_index = 0;
	int         hiername_index = 0;

	err = 0;

    gt_log("Loading hierarchies '%s'", gt_file_hierarchy_type);

    f = fopen(gt_file_hierarchy_type, "r");
    if(f == NULL) {
        gt_log("Can't open file: %s", strerror(errno));
        return ENOENT;
    }

    while(fgets(line, GT_MAXFILELINE, f)) {

        gt_trim_string(line);

        if (headers == NULL) {
            cnt_columns = gt_get_split_text_cnt(line, GT_DATA_DELIM);

            if (cnt_columns < 2) {
                gt_log("Error parsing headers in hierarchies file");
                err = EINVAL;
                break;
            }

            /*allocating once for performance*/
            headers = gt_allocate_split_text(cnt_columns);
            data_row = gt_allocate_split_text(cnt_columns);

            if (headers == NULL || data_row == NULL) {
                gt_log("Error: out of memory!");
                err = ENOMEM;
                break;
            }

            if ((ret = gt_split_text(line, GT_DATA_DELIM, headers, cnt_columns)) != 0) {
                gt_log("Error parsing headers in in hierarchies file");
                err = EINVAL;
                break;
            }

            hierid_index = gt_get_column_index(GT_TABLE_TYPE_HIERARCHY_TYPE, GT_TABLE_COLUMN_TYPE_HIERARCHY_TYPE_ID, headers, cnt_columns);
            hiername_index = gt_get_column_index(GT_TABLE_TYPE_HIERARCHY_TYPE, GT_TABLE_COLUMN_TYPE_HIERARCHY_NAME, headers, cnt_columns);

            if (hierid_index < 0 ||
                hiername_index < 0) {

                    gt_log("Error parsing headers in hierarchies table");
                    err = EINVAL;
                    break;
                }

            continue;
        }

        if ((ret = gt_split_text(line, GT_DATA_DELIM, data_row, cnt_columns)) != 0) {
            gt_log("Warning, invalid row in hierarchies table");
            continue;
        }

        hierid = strtoul(data_row[hierid_index], &tail, 10);
        strncpy(hiername, data_row[hiername_index], GT_MAX_SIZE_INPUTSTR - 1);

        ret = add_hier(hierid, hiername);
        if(ret != 0) {
            err = ENOEXEC;
            break;
        }
    }

    fclose(f);

    if (data_row != NULL) {
        gt_free_split_text(data_row, cnt_columns);
        data_row = NULL;
    }

    if (headers != NULL) {
        gt_free_split_text(headers, cnt_columns);
        headers = NULL;
    }

    if (err == 0) {
        gt_log("Loaded %d hierarchies.", gt_hiers_cnt);
    }

    return err;
}

static int
gt_load_table_hierarchy_nodes()
{
    FILE		*f;
	int		    err;
	char		line[GT_MAXFILELINE];
	int		    ret;
	unsigned	hierid;
	unsigned	nodeid;
	char		nodename[GT_MAX_SIZE_INPUTSTR];
	unsigned	parentid;
	unsigned	level;
	char        **headers = NULL;
	char        **data_row = NULL;
	char        *tail = NULL;
	int         cnt_columns = 0;
	int         hierid_index = 0;
	int         nodeid_index = 0;
	int         nodename_index = 0;
	int         level_index = 0;
	int         parentid_index = 0;

	err = 0;

    gt_log("Loading hierarchy nodes '%s'", gt_file_hierarchy_node);
    f = fopen(gt_file_hierarchy_node, "r");
    if(f == NULL) {
        gt_log("Can't open file: %s", strerror(errno));
        return ENOENT;
    }

    while(fgets(line, GT_MAXFILELINE, f)) {

        gt_trim_string(line);

        if (headers == NULL) {
            cnt_columns = gt_get_split_text_cnt(line, GT_DATA_DELIM);

            if (cnt_columns < 4) {
                gt_log("Error parsing headers in hierarchy nodes file");
                err = EINVAL;
                break;
            }

            /*allocating once for performance*/
            headers = gt_allocate_split_text(cnt_columns);
            data_row = gt_allocate_split_text(cnt_columns);

            if (headers == NULL || data_row == NULL) {
                gt_log("Error: out of memory!");
                err = ENOMEM;
                break;
            }

            if ((ret = gt_split_text(line, GT_DATA_DELIM, headers, cnt_columns)) != 0) {
                gt_log("Error parsing headers in hierarchy nodes file");
                err = EINVAL;
                break;
            }

            nodeid_index = gt_get_column_index(GT_TABLE_TYPE_HIERARCHY_NODE, GT_TABLE_COLUMN_TYPE_HIERARCHY_NODE_ID, headers, cnt_columns);
            hierid_index = gt_get_column_index(GT_TABLE_TYPE_HIERARCHY_NODE, GT_TABLE_COLUMN_TYPE_HIERARCHY_TYPE_ID, headers, cnt_columns);
            nodename_index = gt_get_column_index(GT_TABLE_TYPE_HIERARCHY_NODE, GT_TABLE_COLUMN_TYPE_HIERARCHY_NODE_NAME, headers, cnt_columns);
            level_index = gt_get_column_index(GT_TABLE_TYPE_HIERARCHY_NODE, GT_TABLE_COLUMN_TYPE_LEVEL, headers, cnt_columns);
            parentid_index = gt_get_column_index(GT_TABLE_TYPE_HIERARCHY_NODE, GT_TABLE_COLUMN_TYPE_PARENT_NODE_ID, headers, cnt_columns);

            if (nodeid_index < 0 ||
                hierid_index < 0 ||
                nodename_index < 0 ||
                level_index < 0) {

                    gt_log("Error parsing headers in hierarchy nodes table");
                    err = EINVAL;
                    break;
                }

            continue;
        }

        if ((ret = gt_split_text(line, GT_DATA_DELIM, data_row, cnt_columns)) != 0) {
            gt_log("Warning, invalid row in hierarchy nodes table");
            continue;
        }

        nodeid = strtoul(data_row[nodeid_index], &tail, 10);
        hierid = strtoul(data_row[hierid_index], &tail, 10);
        strncpy(nodename, data_row[nodename_index], GT_MAX_SIZE_INPUTSTR - 1);
        level = strtoul(data_row[level_index], &tail, 10);

        if (parentid_index < 0)
            parentid = 0;
        else
            parentid = strtoul(data_row[parentid_index], &tail, 10);

        ret = add_hier_node(nodeid, nodename, hierid,
            level, parentid);
        if(ret != 0) {
            err = ENOEXEC;
            break;
        }
    }
    fclose(f);

    if (data_row != NULL) {
        gt_free_split_text(data_row, cnt_columns);
        data_row = NULL;
    }

    if (headers != NULL) {
        gt_free_split_text(headers, cnt_columns);
        headers = NULL;
    }

    if (err == 0) {
        gt_log("Loaded %d hierarchy nodes.", gt_hier_nodes_cnt);

        gt_log("Adding parent/child/sibling links to hierarchy nodes.");
        gt_add_hier_node_links();
        gt_log("Done adding links to hierarchy nodes.");
    }

    return err;
}

static int
gt_load_table_descriptor_hierarchy_nodes()
{
    FILE		*f;
	int		    err;
	char		line[GT_MAXFILELINE];
	unsigned	goetid;
	char		typestr[128];
	int		    ret;
	/*unsigned	hierid;*/
	unsigned	nodeid;
	char        **headers = NULL;
	char        **data_row = NULL;
	char        *tail = NULL;
	int         cnt_columns = 0;
	int         goetid_index = 0;
	int         type_index = 0;
	int         hierid_index = 0;
	int         nodeid_index = 0;

	err = 0;

	gt_log("Loading descriptor to hierarchy node links '%s'",
        gt_file_descriptor_to_hierarchy_node);

    f = fopen(gt_file_descriptor_to_hierarchy_node, "r");
    if(f == NULL) {
        gt_log("Can't open file: %s", strerror(errno));
        return ENOENT;
    }

    while(fgets(line, GT_MAXFILELINE, f)) {

        gt_trim_string(line);

        if (headers == NULL) {
            cnt_columns = gt_get_split_text_cnt(line, GT_DATA_DELIM);

            if (cnt_columns < 4) {
                gt_log("Error parsing headers in descriptor to hierarchy node links file");
                err = EINVAL;
                break;
            }

            /*allocating once for performance*/
            headers = gt_allocate_split_text(cnt_columns);
            data_row = gt_allocate_split_text(cnt_columns);

            if (headers == NULL || data_row == NULL) {
                gt_log("Error: out of memory!");
                err = ENOMEM;
                break;
            }

            if ((ret = gt_split_text(line, GT_DATA_DELIM, headers, cnt_columns)) != 0) {
                gt_log("Error parsing headers in descriptor to hierarchy node links file");
                err = EINVAL;
                break;
            }

            goetid_index = gt_get_column_index(GT_TABLE_TYPE_DESCRIPTOR_TO_HIERARCHY_NODE, GT_TABLE_COLUMN_TYPE_DESCRIPTOR_ID, headers, cnt_columns);
            type_index = gt_get_column_index(GT_TABLE_TYPE_DESCRIPTOR_TO_HIERARCHY_NODE, GT_TABLE_COLUMN_TYPE_DESCRIPTOR_TYPE, headers, cnt_columns);
            nodeid_index = gt_get_column_index(GT_TABLE_TYPE_DESCRIPTOR_TO_HIERARCHY_NODE, GT_TABLE_COLUMN_TYPE_HIERARCHY_NODE_ID, headers, cnt_columns);
            hierid_index = gt_get_column_index(GT_TABLE_TYPE_DESCRIPTOR_TO_HIERARCHY_NODE, GT_TABLE_COLUMN_TYPE_HIERARCHY_TYPE_ID, headers, cnt_columns);

            if (goetid_index < 0 ||
                type_index < 0 ||
                nodeid_index < 0 ||
                hierid_index < 0) {

                    gt_log("Error parsing headers in descriptor to hierarchy node links table");
                    err = EINVAL;
                    break;
                }

            continue;
        }

        if ((ret = gt_split_text(line, GT_DATA_DELIM, data_row, cnt_columns)) != 0) {
            gt_log("Warning, invalid row in descriptor to hierarchy node links table");
            continue;
        }

        goetid = strtoul(data_row[goetid_index], &tail, 10);
        strncpy(typestr, data_row[type_index], 127);
        nodeid = strtoul(data_row[nodeid_index], &tail, 10);
        /*hierid = strtoul(data_row[hierid_index], &tail, 10);*/

        ret = add_goet_to_hier_node(goetid, nodeid);
        if(ret != 0) {
            err = ENOEXEC;
            break;
        }
    }
    fclose(f);

    if (data_row != NULL) {
        gt_free_split_text(data_row, cnt_columns);
        data_row = NULL;
    }

    if (headers != NULL) {
        gt_free_split_text(headers, cnt_columns);
        headers = NULL;
    }

    if (err == 0) {
        gt_log("Loaded %d descriptor to hierarchy node links.",
            gt_goet_to_hier_node_allcnt);
    }

    return err;
}

static int
gt_load_table_languages()
{
    FILE		*f;
	int		    err;
	char		line[GT_MAXFILELINE];
	int		    ret;
	int		    langid;
	char		langname[GT_MAX_SIZE_INPUTSTR];
	char		langiso[GT_MAX_SIZE_INPUTSTR];
	char        **headers = NULL;
	char        **data_row = NULL;
	char        *tail = NULL;
	int         cnt_columns = 0;
	int         langid_index = 0;
	int         langname_index = 0;
	int         langiso_index = 0;

	err = 0;

	gt_log("Loading languages '%s'", gt_file_language);
    f = fopen(gt_file_language, "r");
    if(f == NULL) {
        gt_log("Can't open file: %s", strerror(errno));
        return ENOENT;
    }

    while(fgets(line, GT_MAXFILELINE, f)) {

        gt_trim_string(line);

        if (headers == NULL) {
            cnt_columns = gt_get_split_text_cnt(line, GT_DATA_DELIM);

            if (cnt_columns < 3) {
                gt_log("Error parsing headers in languages file");
                err = EINVAL;
                break;
            }

            /*allocating once for performance*/
            headers = gt_allocate_split_text(cnt_columns);
            data_row = gt_allocate_split_text(cnt_columns);

            if (headers == NULL || data_row == NULL) {
                gt_log("Error: out of memory!");
                err = ENOMEM;
                break;
            }

            if ((ret = gt_split_text(line, GT_DATA_DELIM, headers, cnt_columns)) != 0) {
                gt_log("Error parsing headers in languages file");
                err = EINVAL;
                break;
            }

            langid_index = gt_get_column_index(GT_TABLE_TYPE_LANGUAGE, GT_TABLE_COLUMN_TYPE_LANGUAGE_ID, headers, cnt_columns);
            langname_index = gt_get_column_index(GT_TABLE_TYPE_LANGUAGE, GT_TABLE_COLUMN_TYPE_LANGUAGE_NAME, headers, cnt_columns);
            langiso_index = gt_get_column_index(GT_TABLE_TYPE_LANGUAGE, GT_TABLE_COLUMN_TYPE_LANGUAGE_ISO, headers, cnt_columns);

            if (langid_index < 0 ||
                langname_index < 0 ||
                langiso_index < 0) {

                    gt_log("Error parsing headers in languages table");
                    err = EINVAL;
                    break;
                }

            continue;
        }

        if ((ret = gt_split_text(line, GT_DATA_DELIM, data_row, cnt_columns)) != 0) {
            gt_log("Warning, invalid row in languages table");
            continue;
        }

        langid = strtol(data_row[langid_index], &tail, 10);
        strncpy(langname, data_row[langname_index], GT_MAX_SIZE_INPUTSTR - 1);
        strncpy(langiso, data_row[langiso_index], GT_MAX_SIZE_INPUTSTR - 1);

        ret = add_language(langid, langname, langiso);
        if(ret != 0) {
            err = ENOEXEC;
            break;
        }
    }
    fclose(f);

    if (data_row != NULL) {
        gt_free_split_text(data_row, cnt_columns);
        data_row = NULL;
    }

    if (headers != NULL) {
        gt_free_split_text(headers, cnt_columns);
        headers = NULL;
    }

    if (err == 0) {
        gt_log("Loaded %d languages.", gt_langs_cnt);
    }

    return err;
}

static int
gt_load_table_hierarchy_localizations()
{
    FILE		*f;
	int		    err;
	char		line[GT_MAXFILELINE];
	int		    ret;
	unsigned	nodeid;
	int		    langid;
	char		locstr[GT_MAX_SIZE_INPUTSTR];
	char        **headers = NULL;
	char        **data_row = NULL;
	char        *tail = NULL;
	int         cnt_columns = 0;
	int         nodeid_index = 0;
	int         langid_index = 0;
	int         locstr_index = 0;

	err = 0;

	gt_log("Loading hierarchy node localizations '%s'",
		    gt_file_hierarchy_node_language);

    f = fopen(gt_file_hierarchy_node_language, "r");
    if(f == NULL) {
        gt_log("Can't open file: %s", strerror(errno));
        return ENOENT;
    }

    while(fgets(line, GT_MAXFILELINE, f)) {

        gt_trim_string(line);

        if (headers == NULL) {
            cnt_columns = gt_get_split_text_cnt(line, GT_DATA_DELIM);

            if (cnt_columns < 3) {
                gt_log("Error parsing headers in hierarchy node localizations file");
                err = EINVAL;
                break;
            }

            /*allocating once for performance*/
            headers = gt_allocate_split_text(cnt_columns);
            data_row = gt_allocate_split_text(cnt_columns);

            if (headers == NULL || data_row == NULL) {
                gt_log("Error: out of memory!");
                err = ENOMEM;
                break;
            }

            if ((ret = gt_split_text(line, GT_DATA_DELIM, headers, cnt_columns)) != 0) {
                gt_log("Error parsing headers in hierarchy node localizations file");
                err = EINVAL;
                break;
            }

            nodeid_index = gt_get_column_index(GT_TABLE_TYPE_HIERARCHY_NODE_LANGUAGE, GT_TABLE_COLUMN_TYPE_HIERARCHY_NODE_ID, headers, cnt_columns);
            langid_index = gt_get_column_index(GT_TABLE_TYPE_HIERARCHY_NODE_LANGUAGE, GT_TABLE_COLUMN_TYPE_LANGUAGE_ID, headers, cnt_columns);
            locstr_index = gt_get_column_index(GT_TABLE_TYPE_HIERARCHY_NODE_LANGUAGE, GT_TABLE_COLUMN_TYPE_LOCALIZED_NAME, headers, cnt_columns);

            if (locstr_index < 0)
                locstr_index = gt_get_column_index(GT_TABLE_TYPE_HIERARCHY_NODE_LANGUAGE, GT_TABLE_COLUMN_TYPE_LOCALIZED_NAME2, headers, cnt_columns);

            if (nodeid_index < 0 ||
                langid_index < 0 ||
                locstr_index < 0) {

                    gt_log("Error parsing headers in hierarchy node localizations table");
                    err = EINVAL;
                    break;
                }

            continue;
        }

        if ((ret = gt_split_text(line, GT_DATA_DELIM, data_row, cnt_columns)) != 0) {
            gt_log("Warning, invalid row in hierarchy node localizations table");
            continue;
        }

        nodeid = strtoul(data_row[nodeid_index], &tail, 10);
        langid = strtol(data_row[langid_index], &tail, 10);
        strncpy(locstr, data_row[locstr_index], GT_MAX_SIZE_INPUTSTR - 1);

        ret = add_hier_node_locstr(nodeid, langid, locstr);
        if(ret != 0) {
            err = ENOEXEC;
            break;
        }
    }
    fclose(f);

    if (data_row != NULL) {
        gt_free_split_text(data_row, cnt_columns);
        data_row = NULL;
    }

    if (headers != NULL) {
        gt_free_split_text(headers, cnt_columns);
        headers = NULL;
    }

    if (err == 0) {
        gt_log("Loaded %d hierarchy node localizations.",
            gt_hier_nodes_alllocstr);
    }

    return err;
}

#define GT_COLUMN_OVERRIDE_INITSIZE 8
#define GT_COLUMN_OVERRIDE_GROW		8
int
gt_set_column_overrides(char *path_to_file)
{
    int         ireturn = 0;
    FILE		*f;
    char		line[GT_MAXFILELINE];
    char        **data = NULL;
    int         data_cnt = 0;
    int         table_id = 0;
    int         column_id = 0;
    int         list_size = 0;

    gt_log("Adding column overrides from %s", path_to_file);

    gt_column_override_cnt = 0;
    gt_column_override_list = (gt_column_override_t *) malloc (sizeof(gt_column_override_t) * GT_COLUMN_OVERRIDE_INITSIZE);

    if (gt_column_override_list == NULL) {
        gt_log("Error: out of memory!");
        return ENOMEM;
    }
    list_size = GT_COLUMN_OVERRIDE_INITSIZE;

    /*only interested in 3 columns*/
    data = gt_allocate_split_text(3);
    if (data == NULL) {
        gt_log("Error: out of memory!");
        return ENOMEM;
    }

    f = fopen(path_to_file, "r");
    if(f == NULL) {
        gt_log("Can't open file: %s", strerror(errno));
        return ENOENT;
    }

    while(fgets(line, GT_MAXFILELINE, f)) {

        gt_trim_string(line);

        /* Ignore newlines. */
		if(!strlen(line))
			continue;

		/* Ignore comments. */
		if(line[0] == '#')
			continue;

        data_cnt = gt_get_split_text_cnt(line, "\t");

        if (data_cnt < 3) {
            gt_log("Warning, error in column override file.");
            continue;
        }

        if (gt_split_text(line, "\t", data, 3) != 0) {
            gt_log("Warning, error in column override file");
            continue;
        }

        table_id = gt_get_tableid_from_name(data[0]);
        column_id = gt_get_columnid_from_name(data[1]);

        if (column_id < 0 || table_id < 0) {
            gt_log("Warning, did not recognize column %s in table %s", data[1], data[0]);
            continue;
        }

        if (gt_column_override_cnt >= list_size){
            gt_column_override_list = (gt_column_override_t *) realloc (gt_column_override_list, sizeof(gt_column_override_t) * (list_size + GT_COLUMN_OVERRIDE_GROW) );
            list_size += GT_COLUMN_OVERRIDE_GROW;

            if (gt_column_override_list == NULL) {
                gt_log("Error: memory allocation failed in gt_set_column_overrides");
                ireturn = ENOMEM;
                break;
            }
        }

        gt_column_override_list[gt_column_override_cnt].gt_table_id = table_id;
        gt_column_override_list[gt_column_override_cnt].gt_column_id = column_id;
        gt_column_override_list[gt_column_override_cnt].column_override = (char *) malloc(sizeof(char) * (strlen(data[2]) + 1));

        if (gt_column_override_list[gt_column_override_cnt].column_override == NULL)
        {
            gt_log("Error: memory allocation failed in gt_set_column_overrides");
            ireturn = ENOMEM;
            break;
        }

        memset(gt_column_override_list[gt_column_override_cnt].column_override, 0, sizeof(char) * (strlen(data[2]) + 1));
        strncpy(gt_column_override_list[gt_column_override_cnt].column_override, data[2], strlen(data[2]));

        gt_column_override_cnt++;
    }

    fclose(f);

    if (data != NULL) {
        gt_free_split_text(data, 3);
        data = NULL;
    }

    return ireturn;
}

int
gt_get_tableid_from_name(char *tablename)
{
    int tableid = -1;

    if (!strcmp(tablename, "all")) {
        tableid = GT_TABLE_TYPE_ALL;
    } else
    if(!strcmp(tablename, "artist_file")) {
        tableid = GT_TABLE_TYPE_ARTIST_LIST;
    } else
    if(!strcmp(tablename, "artist_descriptor_file")) {
        tableid = GT_TABLE_TYPE_ARTIST_CATEGORY;
    } else
    if(!strcmp(tablename, "artist_popularity_file")) {
        tableid = GT_TABLE_TYPE_ARTIST_POPULARITY;
    } else
    if(!strcmp(tablename, "descriptor_file")) {
        tableid = GT_TABLE_TYPE_CATEGORY;
    } else
    if(!strcmp(tablename, "descriptor_correlate_file")) {
        tableid = GT_TABLE_TYPE_CORRELATE;
    } else
    if(!strcmp(tablename, "recording_file")) {
        tableid = GT_TABLE_TYPE_TRACK_LIST;
    } else
    if(!strcmp(tablename, "recording_descriptor_file")) {
        tableid = GT_TABLE_TYPE_TRACK_CATEGORY;
    } else
    if(!strcmp(tablename, "recording_popularity_file")) {
        tableid = GT_TABLE_TYPE_TRACK_POPULARITY;
    } else
    if(!strcmp(tablename, "hierarchy_type_file")) {
        tableid = GT_TABLE_TYPE_HIERARCHY_TYPE;
    } else
    if(!strcmp(tablename, "hierarchy_node_file")) {
        tableid = GT_TABLE_TYPE_HIERARCHY_NODE;
    } else
    if(!strcmp(tablename, "hierarchy_node_language_file")) {
        tableid = GT_TABLE_TYPE_HIERARCHY_NODE_LANGUAGE;
    } else
    if(!strcmp(tablename, "descriptor_to_hierarchy_node_file")) {
        tableid = GT_TABLE_TYPE_DESCRIPTOR_TO_HIERARCHY_NODE;
    } else
    if(!strcmp(tablename, "language_file")) {
        tableid = GT_TABLE_TYPE_LANGUAGE;
    } else
    if(!strcmp(tablename, "album_master")) {
        tableid = GT_TABLE_TYPE_ALBUM_MASTER;
    } else
    if(!strcmp(tablename, "album_edition")) {
        tableid = GT_TABLE_TYPE_ALBUM_EDITION;
    } else
    if(!strcmp(tablename, "song_file")) {
        tableid = GT_TABLE_TYPE_SONG;
    }
    return tableid;
}

int
gt_get_columnid_from_name(char *columnname)
{
    int columnid = -1;

    if (!strcasecmp(columnname, "ARTIST_ID")) {
        columnid = GT_TABLE_COLUMN_TYPE_ARTIST_ID;
    } else
    if (!strcasecmp(columnname, "DESCRIPTOR_ID")) {
        columnid = GT_TABLE_COLUMN_TYPE_DESCRIPTOR_ID;
    } else
    if (!strcasecmp(columnname, "DESCRIPTOR_TYPE")) {
        columnid = GT_TABLE_COLUMN_TYPE_DESCRIPTOR_TYPE;
    } else
    if (!strcasecmp(columnname, "WEIGHT")) {
        columnid = GT_TABLE_COLUMN_TYPE_WEIGHT;
    } else
    if (!strcasecmp(columnname, "ARTIST_NAME")) {
        columnid = GT_TABLE_COLUMN_TYPE_ARTIST_NAME;
    } else
    if (!strcasecmp(columnname, "VOLUME")) {
        columnid = GT_TABLE_COLUMN_TYPE_POPULARITY;
    } else
    if (!strcasecmp(columnname, "DESCRIPTOR_NAME")) {
        columnid = GT_TABLE_COLUMN_TYPE_DESCRIPTOR_NAME;
    } else
    if (!strcasecmp(columnname, "DESCRIPTOR_ID_1")) {
        columnid = GT_TABLE_COLUMN_TYPE_DESCRIPTOR_ID_1;
    } else
    if (!strcasecmp(columnname, "DESCRIPTOR_ID_2")) {
        columnid = GT_TABLE_COLUMN_TYPE_DESCRIPTOR_ID_2;
    } else
    if (!strcasecmp(columnname, "RECORDING_ID")) {
        columnid = GT_TABLE_COLUMN_TYPE_RECORDING_ID;
    } else
    if (!strcasecmp(columnname, "NAME")) {
        columnid = GT_TABLE_COLUMN_TYPE_RECORDING_NAME;
    } else
    if (!strcasecmp(columnname, "ALBUM_NAME")) {
        columnid = GT_TABLE_COLUMN_TYPE_ALBUM_NAME;
    } else
    if (!strcasecmp(columnname, "YEAR")) {
        columnid = GT_TABLE_COLUMN_TYPE_YEAR;
    } else
    if (!strcasecmp(columnname, "HIERARCHY_TYPE_ID")) {
        columnid = GT_TABLE_COLUMN_TYPE_HIERARCHY_TYPE_ID;
    } else
    if (!strcasecmp(columnname, "HIERARCHY_NAME")) {
        columnid = GT_TABLE_COLUMN_TYPE_HIERARCHY_NAME;
    } else
    if (!strcasecmp(columnname, "HIERARCHY_NODE_ID")) {
        columnid = GT_TABLE_COLUMN_TYPE_HIERARCHY_NODE_ID;
    } else
    if (!strcasecmp(columnname, "HIERARCHY_NODE_NAME")) {
        columnid = GT_TABLE_COLUMN_TYPE_HIERARCHY_NODE_NAME;
    } else
    if (!strcasecmp(columnname, "LEVEL")) {
        columnid = GT_TABLE_COLUMN_TYPE_LEVEL;
    } else
    if (!strcasecmp(columnname, "PARENT_NODE_ID")) {
        columnid = GT_TABLE_COLUMN_TYPE_PARENT_NODE_ID;
    } else
    if (!strcasecmp(columnname, "LANGUAGE_ID")) {
        columnid = GT_TABLE_COLUMN_TYPE_LANGUAGE_ID;
    } else
    if (!strcasecmp(columnname, "LANGUAGE_NAME")) {
        columnid = GT_TABLE_COLUMN_TYPE_LANGUAGE_NAME;
    } else
    if (!strcasecmp(columnname, "ISO_CODE")) {
        columnid = GT_TABLE_COLUMN_TYPE_LANGUAGE_ISO;
    } else
    if (!strcasecmp(columnname, "HIERARCHY_NODED_NAME_LOCALIZED")) {
        columnid = GT_TABLE_COLUMN_TYPE_LOCALIZED_NAME;
    } else
    if (!strcasecmp(columnname, "HIERARCHY_NODE_NAME_LOCALIZED")) {
        columnid = GT_TABLE_COLUMN_TYPE_LOCALIZED_NAME2;
    } else
    if (!strcasecmp(columnname, "CORRELATE_TYPE")) {
        columnid = GT_TABLE_COLUMN_TYPE_CORRELATE_TYPE;
    } else
    if (!strcasecmp(columnname, "CORRELATE_VALUE")) {
        columnid = GT_TABLE_COLUMN_TYPE_CORRELATE_VALUE;
    } else
    if (!strcasecmp(columnname, "CATEGORY_ID")) {
        columnid = GT_TABLE_COLUMN_TYPE_CATEGORY_ID;
    } else
    if (!strcasecmp(columnname, "CATEGORY_NAME")) {
        columnid = GT_TABLE_COLUMN_TYPE_CATEGORY_NAME;
    } else
    if (!strcasecmp(columnname, "CATEGORY_TYPE")) {
        columnid = GT_TABLE_COLUMN_TYPE_CATEGORY_TYPE;
    } else
    if (!strcasecmp(columnname, "TRACK_ID")) {
        columnid = GT_TABLE_COLUMN_TYPE_TRACK_ID;
    } else
    if (!strcasecmp(columnname, "TRACK_NAME")) {
        columnid = GT_TABLE_COLUMN_TYPE_TRACK_NAME;
    } else
    if (!strcasecmp(columnname, "SRC_CATEGORY_ID")) {
        columnid = GT_TABLE_COLUMN_TYPE_SRC_CATEGORY_ID;
    } else
    if (!strcasecmp(columnname, "TGT_CATEGORY_ID")) {
        columnid = GT_TABLE_COLUMN_TYPE_TGT_CATEGORY_ID;
    } else
    if (!strcasecmp(columnname, "ALBUM_ID")) {
        columnid = GT_TABLE_COLUMN_TYPE_ALBUM_ID;
    } else
    if (!strcasecmp(columnname, "RELEASE_YEAR")) {
        columnid = GT_TABLE_COLUMN_TYPE_RELEASE_YEAR;
    } else
    if (!strcasecmp(columnname, "NEWPOPULARITY")) {
        columnid = GT_TABLE_COLUMN_TYPE_NEW_POPULARITY;
    } else
    if (!strcasecmp(columnname, "GN_ID")) {
        columnid = GT_TABLE_COLUMN_TYPE_GN_ID;
    }

    return columnid;
}

/*returns 1 (TRUE) if override found, 0 (FALSE) otherwise*/
static int
gt_get_column_override(int tableid, int columnid, char *columnheader)
{
    int     i=0;
    int     ireturn = 0;

    /*first check if there is an exact match, then check if there is an override for all tables*/
    for (i = 0; i < gt_column_override_cnt; ++i) {

        if (gt_column_override_list[i].gt_table_id == tableid &&
            gt_column_override_list[i].gt_column_id == columnid) {

            strncpy(columnheader, gt_column_override_list[i].column_override, GT_MAXFILELINE);
            ireturn = 1;

            break;
        }
    }

    if (ireturn == 0) {

        for (i = 0; i < gt_column_override_cnt; ++i) {

            if (gt_column_override_list[i].gt_table_id == GT_TABLE_TYPE_ALL &&
                gt_column_override_list[i].gt_column_id == columnid) {

                    strncpy(columnheader, gt_column_override_list[i].column_override, GT_MAXFILELINE);
                    ireturn = 1;

                    break;
                }
        }
    }

    return ireturn;
}

static int gt_init_artists_by_id_list()
{
    int i = 0;

    if (gt_artists_cnt <= 0)
        return 0;

    gt_artists_by_id = (gt_artist_t **) malloc(sizeof(gt_artist_t *) * gt_artists_cnt);
    if (gt_artists_by_id == NULL)
        return ENOMEM;

    for (i = 0; i < gt_artists_cnt; ++i)
    {
        gt_artists_by_id[i] = &gt_artists[i];
    }

    qsort(gt_artists_by_id, gt_artists_cnt, sizeof(gt_artist_t *), comp_partist_by_id);

    return 0;
}

/*array of pointer to artist*/
static int comp_partist_by_id(const void *arg1, const void *arg2)
{
    gt_artist_t *pArt1 = *((gt_artist_t **) arg1);
    gt_artist_t *pArt2 = *((gt_artist_t **) arg2);

    return pArt1->ga_id - pArt2->ga_id;
}

static unsigned int add_gmd_id(char *id) {
	if (id_count == id_size) {
		id_size += ID_SIZE_GROW;
		id_to_gmd = (gmd_id_mapping_t *)realloc(id_to_gmd, sizeof(gmd_id_mapping_t) * id_size);
		if (id_to_gmd == NULL)
			return ENOMEM;
	}
	strcpy(id_to_gmd[id_count].gmd_id, id);
	id_to_gmd[id_count].numerical_id = id_count;
	id_count++;
	return (id_count-1);
}

static int cmp_gmd_id_mapping(const void *arg1, const void *arg2) {
	gmd_id_mapping_t *map1 = (gmd_id_mapping_t *) arg1;
	gmd_id_mapping_t *map2 = (gmd_id_mapping_t *) arg2;

	return (strcmp(map1->gmd_id, map2->gmd_id));
}

int gt_get_numerical_id_from_gmd(char *gmd_in, unsigned int *id_out) {
	gmd_id_mapping_t *found = NULL;
	gmd_id_mapping_t target;

	strcpy(target.gmd_id, gmd_in);
	found = (gmd_id_mapping_t *) bsearch(&target, id_to_gmd_sort_by_string, id_count,
		sizeof(gmd_id_mapping_t), cmp_gmd_id_mapping);
	if (found == NULL) {
		return ENOENT;
	}
	*id_out = (unsigned int) found->numerical_id;
	return 0;

}

int gt_get_gmd_id_from_numerical(unsigned int id_in, char *gmd_out) {
	if (id_count <= id_in) {
		return ENOENT;
	}
	strcpy(gmd_out, id_to_gmd[id_in].gmd_id);
	return 0;
}

static int initialize_sorted_id_list() {
	id_to_gmd_sort_by_string = malloc(sizeof(gmd_id_mapping_t) * id_count);
	if (id_to_gmd_sort_by_string == NULL)
		return ENOMEM;
	memcpy(id_to_gmd_sort_by_string, id_to_gmd, sizeof(gmd_id_mapping_t) * id_count);
	qsort(id_to_gmd_sort_by_string, id_count, sizeof(gmd_id_mapping_t), cmp_gmd_id_mapping);
	return 0;
}

/*sort by song id, release year ascending, recording id*/
static int cmp_gmd_recording_data(const void *arg1, const void *arg2)
{
    int result = 0;
    gmd_recording_data_t *r1 = (gmd_recording_data_t *) arg1;
    gmd_recording_data_t *r2 = (gmd_recording_data_t *) arg2;

    result = strcmp(r1->gmd_song_id, r2->gmd_song_id);
    if (result != 0)
        return result;

    /*release year ascending...yes! null comes after numbers*/
    result = strcmp(r1->gmd_release_year, r2->gmd_release_year);
    if (result != 0)
        return result;

    return strcmp(r1->gmd_recording_id, r2->gmd_recording_id);
}

/*sort preferred editions by id */
static int cmp_gmd_preferred_editions(const void *arg1, const void *arg2)
{
    gmd_preferred_edition_t *pe1 = (gmd_preferred_edition_t *) arg1;
    gmd_preferred_edition_t *pe2 = (gmd_preferred_edition_t *) arg2;

    return strcmp(pe1->gmd_id, pe2->gmd_id);
}

static int cmp_gmd_rec_2_preferred_edition_id_w_release_type(const void *arg1, const void *arg2)
{
    gmd_rec_id_2_pref_edition_id_t *mapping1 = (gmd_rec_id_2_pref_edition_id_t *) arg1;
    gmd_rec_id_2_pref_edition_id_t *mapping2 = (gmd_rec_id_2_pref_edition_id_t *) arg2;
    int result = strcmp(mapping1->gmd_recording_id, mapping2->gmd_recording_id);
    if (result == 0)
    {
        result = get_sort_rank_from_release_type(mapping1->release_type) - get_sort_rank_from_release_type(mapping2->release_type);
    }

    return result;
}

static int cmp_gmd_rec_2_preferred_edition_id_wout_release_type(const void *arg1, const void *arg2)
{
    gmd_rec_id_2_pref_edition_id_t *mapping1 = (gmd_rec_id_2_pref_edition_id_t *) arg1;
    gmd_rec_id_2_pref_edition_id_t *mapping2 = (gmd_rec_id_2_pref_edition_id_t *) arg2;
    return strcmp(mapping1->gmd_recording_id, mapping2->gmd_recording_id);
}

static int get_sort_rank_from_release_type(int release_type)
{
    /* release types
        1 = main canon
        2 = main canon collection
        3 = single artist collection
        4 = multi artist collection
        5 = music service compilation
        8 = other
        9 = personal burn
        10 = single

        sort in this order:
        main canon = 1
        main canon collection = 2
        single = 10
        single artist collection = 3
        multi artist collection = 4
        music service comp = 5
        other = 8
        personal burn = 9
    */

    if (release_type == 1)
        return 1;
    else if  (release_type == 2)
        return 2;
    else if (release_type == 10)
        return 3;
    else if (release_type == 3)
        return 4;
    else if (release_type == 4)
        return 5;
    else if (release_type == 5)
        return 6;
    else if (release_type == 8)
        return 100;
    else if (release_type == 9)
        return 1000;

    return 20;
}

static int cmp_gmd_recording_index(const void *arg1, const void *arg2)
{
    gmd_recording_data_t *d1 = (gmd_recording_data_t *) arg1;
    gmd_recording_data_t *d2 = (gmd_recording_data_t *) arg2;

    return d1->index - d2->index;
}

static int cmp_edition_master(const void *arg1, const void *arg2)
{
    gmd_edition_master_t *em1 = (gmd_edition_master_t *) arg1;
    gmd_edition_master_t *em2 = (gmd_edition_master_t *) arg2;

    return strcmp(em1->gmd_master_id, em2->gmd_master_id);
}

static int cmp_master_data(const void *arg1, const void *arg2)
{
    gmd_master_data_t *m1 = (gmd_master_data_t *) arg1;
    gmd_master_data_t *m2 = (gmd_master_data_t *) arg2;

    return strcmp(m1->gmd_master_id, m2->gmd_master_id);
}

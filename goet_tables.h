/***********************************************************************
 ** Gracenote GOET Tables library                                     **
 **                                                                   **
 ** Copyright (c) Gracenote, 2013                                     **
 ***********************************************************************/
#ifndef GOET_TABLES_H
#define GOET_TABLES_H

#include "platform.h"

#include <stddef.h>
#include <ctype.h>

#include <assert.h>

#define GT_DATA_FORMAT_TXT	0	/* TXT is legacy data format. */
#define GT_DATA_FORMAT_TSV	1	/* TSV is current format. */
#define GT_DATA_FORMAT_GMD  2   /* TSV is current format. */

#define GT_REC_FORMAT_EXPORT	0	/* Used for data exports. */
#define GT_REC_FORMAT_CUSTOM	1	/* Rarely used. */


#define GT_TYPE_GENRE	0
#define GT_TYPE_ORIGIN	1
#define GT_TYPE_ERA	2
#define GT_TYPE_ATYPE	3
#define GT_TYPE_MOOD	4
#define GT_TYPE_TEMPO_M	5
#define GT_NUM_TYPE	6
#define GT_TYPE_LANG 7

#define GT_TABLE_TYPE_ARTIST_CATEGORY			0
#define GT_TABLE_TYPE_ARTIST_LIST			1
#define GT_TABLE_TYPE_ARTIST_POPULARITY			2
#define GT_TABLE_TYPE_CATEGORY				3
#define GT_TABLE_TYPE_CORRELATE				4
#define GT_TABLE_TYPE_TRACK_CATEGORY			5
#define GT_TABLE_TYPE_TRACK_LIST			6
#define GT_TABLE_TYPE_TRACK_POPULARITY			7
#define GT_TABLE_TYPE_HIERARCHY_TYPE			8
#define GT_TABLE_TYPE_HIERARCHY_NODE			9
#define GT_TABLE_TYPE_HIERARCHY_NODE_LANGUAGE		10
#define GT_TABLE_TYPE_DESCRIPTOR_TO_HIERARCHY_NODE	11
#define GT_TABLE_TYPE_LANGUAGE				12
#define GT_TABLE_TYPE_ALL                   13
#define GT_TABLE_TYPE_ALBUM_EDITION         14
#define GT_TABLE_TYPE_ALBUM_MASTER          15
#define GT_TABLE_TYPE_SONG             16

#define GT_TABLE_COLUMN_TYPE_ARTIST_ID              0
#define GT_TABLE_COLUMN_TYPE_DESCRIPTOR_ID          1
#define GT_TABLE_COLUMN_TYPE_WEIGHT                 2
#define GT_TABLE_COLUMN_TYPE_DESCRIPTOR_TYPE        3
#define GT_TABLE_COLUMN_TYPE_ARTIST_NAME            4
#define GT_TABLE_COLUMN_TYPE_POPULARITY             5
#define GT_TABLE_COLUMN_TYPE_DESCRIPTOR_NAME        6
#define GT_TABLE_COLUMN_TYPE_DESCRIPTOR_ID_1        7
#define GT_TABLE_COLUMN_TYPE_DESCRIPTOR_ID_2        8
#define GT_TABLE_COLUMN_TYPE_RECORDING_ID           9
#define GT_TABLE_COLUMN_TYPE_RECORDING_NAME         10
#define GT_TABLE_COLUMN_TYPE_ALBUM_NAME             11
#define GT_TABLE_COLUMN_TYPE_YEAR                   12
#define GT_TABLE_COLUMN_TYPE_HIERARCHY_TYPE_ID      13
#define GT_TABLE_COLUMN_TYPE_HIERARCHY_NAME         14
#define GT_TABLE_COLUMN_TYPE_HIERARCHY_NODE_ID      15
#define GT_TABLE_COLUMN_TYPE_HIERARCHY_NODE_NAME    16
#define GT_TABLE_COLUMN_TYPE_LEVEL                  17
#define GT_TABLE_COLUMN_TYPE_PARENT_NODE_ID         18
#define GT_TABLE_COLUMN_TYPE_LANGUAGE_ID            19
#define GT_TABLE_COLUMN_TYPE_LANGUAGE_NAME          20
#define GT_TABLE_COLUMN_TYPE_LANGUAGE_ISO           21
#define GT_TABLE_COLUMN_TYPE_LOCALIZED_NAME         22
#define GT_TABLE_COLUMN_TYPE_CORRELATE_TYPE         23
#define GT_TABLE_COLUMN_TYPE_CORRELATE_VALUE        24
#define GT_TABLE_COLUMN_TYPE_CATEGORY_ID            25
#define GT_TABLE_COLUMN_TYPE_CATEGORY_TYPE          26
#define GT_TABLE_COLUMN_TYPE_CATEGORY_NAME          27
#define GT_TABLE_COLUMN_TYPE_TRACK_ID               28
#define GT_TABLE_COLUMN_TYPE_TRACK_NAME             29
#define GT_TABLE_COLUMN_TYPE_SRC_CATEGORY_ID        30
#define GT_TABLE_COLUMN_TYPE_TGT_CATEGORY_ID        31
#define GT_TABLE_COLUMN_TYPE_ALBUM_ID               32
#define GT_TABLE_COLUMN_TYPE_RELEASE_YEAR           33
#define GT_TABLE_COLUMN_TYPE_NEW_POPULARITY         34
#define GT_TABLE_COLUMN_TYPE_GN_ID                  35
#define GT_TABLE_COLUMN_TYPE_LOCALIZED_NAME2        36

#define GT_SIMFLAG_NO_EARLY_BAILOUT	0x00000001
#define GT_SIMFLAG_PRIMARY_GENRE_ONLY	0x00000002
#define GT_SIMFLAG_NO_SEED_ARTIST       0x00000004

#define GMD_DESCRIPTOR_MAX_SIZE 128
#define GMD_MAX_RECORDINGS_PER_EDITION  1000
#define GMD_GLOBAL_ID_SIZE 16

#define GT_MAX_SIZE_INPUTSTR	    1024
#define GT_MAX_SIZE_RELEASEYEAR     8

typedef struct gt_goetcorr {
	char		gc_type;
	unsigned int	gc_id;
	int		gc_weight;
} gt_goetcorr_t;

typedef struct gt_track {
	unsigned	gt_id;

	char		*gt_name;
	char		*gt_albname;
	short		gt_releaseyear;
	unsigned	gt_pop;

	unsigned	gt_art_id;
	unsigned	gt_alb_id;

	char		*gt_artname;	/* For tracks whose artists we
					 * didn't index (custom format). */

	gt_goetcorr_t	*gt_goetcorr;
	int		gt_goetcorr_cnt;
	int		gt_goetcorr_size;

	unsigned	gt_match_score;	/* Used when returning a match list */

} gt_track_t;

typedef struct gt_artist {
	unsigned	ga_id;

	char		*ga_name;
	char		*ga_name_norm;
	char		*ga_name_searchstart;

	unsigned	ga_pop;
	unsigned	ga_rank;

	gt_goetcorr_t	*ga_goetcorr;
	int		ga_goetcorr_cnt;
	int		ga_goetcorr_size;

	unsigned	*ga_tracks;
	int		ga_tracks_cnt;
	int		ga_tracks_size;

	unsigned	ga_match_score;	/* Used when returning a match list */
} gt_artist_t;

typedef struct gt_goet {
	unsigned	gg_id;
	char		gg_type;
	char		*gg_name;

	gt_goetcorr_t	*gg_goetcorr;
	int		gg_goetcorr_cnt;
	int		gg_goetcorr_size;

	unsigned	*gg_hier_node_ids;
	int		gg_hier_node_ids_cnt;
	int		gg_hier_node_ids_size;
} gt_goet_t;


typedef struct gt_search_goet {
	int	gs_goet_id;
	int	gs_weight;
} gt_search_goet_t;


typedef struct gt_similarity {
	int	gs_step1_pass;
	int	gs_score_weighted;
	int	gs_score_genre;
	int	gs_score_mood;
	int	gs_score_era;
	int	gs_score_origin;
	int	gs_score_artist_type;
	int	gs_similarity_value;
} gt_similarity_t;

#define GT_GOETCORRIDX_BUCKETS	5000000

typedef struct gt_goetcorr_ent {
	int			ge_key1;
	int			ge_key2;
	int			ge_corr;
	struct gt_goetcorr_ent	*ge_next;
} gt_goetcorr_ent_t;

typedef struct gt_goetcorr_index {
	gt_goetcorr_ent_t	**gi_buckets;
	int			gi_maxdepth;
} gt_goetcorr_index_t;

typedef struct gt_goetcorr_match {
    int gm_corr;
    int gm_index1;
    int gm_index2;
    unsigned int gm_order;
} gt_goetcorr_match_t;


typedef struct gt_lang {
	int	gl_id;
	char	*gl_name;
	char	*gl_iso;
} gt_lang_t;


typedef struct gt_locstr {
	int	go_langid;
	char	*go_str;
} gt_locstr_t;


typedef struct gt_hier_node {
	int			gn_id;

	char			*gn_name;

	int			gn_level;
	int			gn_hier_id;

	int			gn_parentid;
	struct gt_hier_node	*gn_parent;
	struct gt_hier_node	*gn_next_sibling;
	struct gt_hier_node	*gn_first_child;
	struct gt_hier_node	*gn_last_child;

	gt_locstr_t		*gn_locs;
	int			gn_locs_cnt;
	int			gn_locs_size;

	unsigned		*gn_goet_ids;
	int			gn_goet_ids_cnt;
	int			gn_goet_ids_size;
} gt_hier_node_t;


typedef struct gt_hier {
	int		gh_id;
	char		*gh_name;
	gt_hier_node_t	*gh_first_top;	/* First top level node. */
	gt_hier_node_t	*gh_last_top;	/* Last top level node. */

} gt_hier_t;

typedef struct gt_column_override {
    int     gt_table_id;
    int     gt_column_id;
    char    *column_override;

} gt_column_override_t;

typedef struct gt_sim_params {
    int genre_cutoff;
    int mood_cutoff;
    int genre_weight;
    int origin_weight;
    int era_weight;
    int atype_weight;
    int mood_weight;
} gt_sim_params_t;

int gt_init();
int gt_set_logger(void (*)(char *));

int gt_set_table_file(int, const char *);
int gt_load_tables(int, int, int, int);
int gt_load_tables_lang(int, int, int);
int gt_load_gmd_json(int, int, int);

int gt_search_artist_by_name(char *, gt_artist_t **, int *);
int gt_search_artist_by_goet(int, int, int, gt_artist_t **, int *);
int gt_search_artist_by_goetlist(gt_search_goet_t *, int, int, int, int,
	gt_artist_t **, int *);
int gt_search_artist_by_similarity(int, int, int, int, gt_artist_t **, int *);

int gt_search_track_by_similarity(int, int, int, int, int,
	gt_track_t **, int *);

int gt_search_artist_by_similarity_prim(gt_artist_t *, int, int, int, gt_artist_t **,
    int *);

void gt_free_artist_reslist(gt_artist_t *);
void gt_free_goet_reslist(gt_goet_t *goets);
void gt_free_track_reslist(gt_track_t *);

int gt_get_goet_correlation(int, int);
int gt_get_artist_goet_vector(gt_artist_t *, int, gt_goetcorr_t *, int);
int gt_get_artist_goet_value(gt_artist_t *, int);
int gt_get_artist_primary_goet_value(gt_artist_t *, int);
int gt_get_track_primary_goet_value(gt_track_t *, int);
int gt_get_track_goet_vector(gt_track_t *, int, gt_goetcorr_t *, int);
int gt_compute_vector_similarity(gt_goetcorr_t *, int, gt_goetcorr_t *, int,
    int, int, char *, size_t);
int gt_compute_artist_similarity(int, int, int, int, gt_similarity_t *, char *, size_t);
int gt_compute_artist_to_track_similarity(int, int, int, int,
    gt_similarity_t *, char *, size_t);
int gt_compute_track_similarity(int, int, int, int, gt_similarity_t *, char *, size_t);
int gt_compute_artist_similarity_prim(gt_artist_t *, gt_artist_t *, int, int,
    gt_similarity_t *, char *, size_t);
int gt_compute_track_similarity_prim(gt_track_t *, gt_track_t *, int, int,
    gt_similarity_t *, char *, size_t);
int gt_compute_artist_to_track_similarity_prim(gt_artist_t *, gt_track_t *,
    int, int, gt_similarity_t *, char *, size_t);

int gt_compute_artist_to_track_similarity_prim_params(gt_artist_t *,
                                                      gt_track_t *,
                                                      int,
                                                      int,
                                                      const gt_sim_params_t *,
                                                      gt_similarity_t *,
                                                      char *,
                                                      size_t);

int gt_compute_artist_similarity_prim_params(gt_artist_t *,
                                             gt_artist_t *,
                                             int,
                                             int,
                                             const gt_sim_params_t *,
                                             gt_similarity_t *,
                                             char *,
                                             size_t);

int gt_compute_track_similarity_prim_params(gt_track_t *,
                                            gt_track_t *,
                                            int,
                                            int,
                                            const gt_sim_params_t *,
                                            gt_similarity_t *,
                                            char *,
                                            size_t);

int gt_lookup_artist(int, gt_artist_t **);
int gt_lookup_goet(int, gt_goet_t **);
int gt_lookup_track(int, gt_track_t **);

int gt_get_full_artistlist(gt_artist_t **, int *);
int gt_get_full_goetlist(gt_goet_t **, int *);
int gt_get_full_tracklist(gt_track_t **, int *);

int gt_search_goet_by_name(char *, gt_goet_t **, int *);

char *gt_get_goet_name(int, int);
char *gt_get_goet_typestr(int);
char *gt_get_artist_name(int);

int gt_get_rank_unit();

int gt_lookup_hierarchy(int, gt_hier_t **);
int gt_lookup_hierarchy_node(int, gt_hier_node_t **);
int gt_get_full_hierlist(gt_hier_t **, int *);
int gt_get_full_hier_nodelist(gt_hier_node_t **, int *);

int gt_lookup_lang(int, gt_lang_t **);
int gt_get_full_langlist(gt_lang_t **, int *);

int gt_uninit();

int gt_set_column_overrides(char *);
int gt_get_tableid_from_name(char *);
int gt_get_columnid_from_name(char *);

int gt_get_split_text_cnt(const char *, const char *);
int gt_split_text(char *, const char *, char **, int);
int gt_split_text_extended(char *, const char *, char **, int, int *);
char *gt_strsep(char **, const char *);    /*implementation of strsep since strsep is not included in windows c lib*/
char **gt_allocate_split_text(int);
void gt_free_split_text(char **, int);
void gt_trim_string(char *);

int gt_get_numerical_id_from_gmd(char *gmd_in, unsigned int *id_out);
int gt_get_gmd_id_from_numerical(unsigned int id_in, char *gmd_out);


#endif


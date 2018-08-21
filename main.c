/***********************************************************************
 ** Gracenote MDS Browser                                             **
 **                                                                   **
 ** Copyright (c) Gracenote, 2013                                     **
 ***********************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "goet_tables.h"
#include "buildinfo.h"

#define COPYRIGHT		"Copyright (c) 2013 Gracenote"
#define CONTACT_EMAIL		"Barna Mink <bmink@gracenote.com>"
//#define DEFAULT_CONFIGFILE	"/Users/1100169/prog/gracenote/MDS/GMD/GMD/mdsbrowser.config"
#define DEFAULT_CONFIGFILE    "/Users/1100169/prog/gracenote/MDS/GMD/GMD/ori_mdsbrowser.config"
//#define DEFAULT_CONFIGFILE    "./mdsbrowser.config"


static int simthreshold = 600;
static int popthreshold = 0;
static int maxres = 25;
static int verbose = 0;
static int do_load_recordings = 0;
static int do_load_hierarchies = 0;
static int do_one_recording_per_song = 1;
static int data_format = GT_DATA_FORMAT_TSV;
/*static int rec_format = GT_REC_FORMAT_EXPORT;*/
static int language = 1;
static int scriptmode = 0;


void print_usage(char *);
void print_artist(gt_artist_t *);
void print_track(gt_track_t *);
void print_goet(gt_goet_t *);
void print_hierarchy_node(gt_hier_node_t *,int, int);
void print_hierarchy_node_full(gt_hier_node_t *);
void print_hierarchy(gt_hier_t *, int);


void
logger(char *logline)
{
	if(!scriptmode)
		printf("%s\n", logline);
}


void
print_usage(char *progname)
{
	fprintf(stderr, "Usage: %s [-c configfile] [-s]\n", progname);
}

void
remove_newline(char *str)
{
	char	*ch;

	if(str == NULL || !strlen(str))
		return;

	/* Remove line break at end. */
	ch = &str[strlen(str) - 1];
	while(ch >= str) {
		if(*ch == '\n' || *ch == '\r') {
			*ch = '\0';
		}
		ch--;
	}

}


#define MAXCONFIGLINE	1024

int
load_config(char *configfile)
{
	FILE	*f;
	char	line[MAXCONFIGLINE];
	char	setting[MAXCONFIGLINE];
	char	value[MAXCONFIGLINE];
	int	linenum;
	int	ret;
	int tableid = -1;

	f = fopen(configfile, "r");

	if(f == NULL) {
		fprintf(stderr, "Could not open \"%s\": %s\n", configfile,
			strerror(errno));
		return -1;
	}

	linenum = 0;
	while(fgets(line, MAXCONFIGLINE, f)) {
		++linenum;

		remove_newline(line);

		/* Ignore newlines. */
		if(!strlen(line))
			continue;

		/* Ignore comments. */
		if(line[0] == '#')
			continue;

		ret = sscanf(line, "%s %s", setting, value);
		if(ret != 2 || !strlen(setting) || !strlen(value)) {
			fprintf(stderr, "Config file syntax error"
			    " on line %d.\n---> %s\n", linenum, line);
			fclose(f);
			return -1;
		}

		if(!strcmp(setting, "load_recordings")) {
			if(tolower(value[0]) == 'y')
				do_load_recordings = 1;
			else
				do_load_recordings = 0;
		} else
		if(!strcmp(setting, "load_hierarchies")) {
			if(tolower(value[0]) == 'y')
				do_load_hierarchies = 1;
			else
				do_load_hierarchies = 0;
		} else
		if(!strcmp(setting, "one_recording_per_song")) {
            if (tolower(value[0] == 'y'))
                do_one_recording_per_song = 1;
            else
                do_one_recording_per_song = 0;
		} else
		if(!strcmp(setting, "data_format")) {
			if(!strcmp(value, "txt"))
				data_format = GT_DATA_FORMAT_TXT;
			else
			if(!strcmp(value, "tsv"))
				data_format = GT_DATA_FORMAT_TSV;
			else
			if(!strcmp(value, "gmd"))
				data_format = GT_DATA_FORMAT_GMD;
			else {
				fprintf(stderr, "Config file unsupported format"
				    " on line %d.\n---> %s\n", linenum, line);
				fclose(f);
				return -1;
			}

		} else
		if (!strcmp(setting, "column_override_file")) {
            gt_set_column_overrides(value);
		} else
		if ((tableid = gt_get_tableid_from_name(setting)) >= 0) {
            gt_set_table_file(tableid, value);
		} else {
			fprintf(stderr, "Unknown command in config file"
			    " on line %d.\n---> %s\n", linenum, line);
			fclose(f);
			return -1;
		}
	}

	fclose(f);

	return 0;
}


void
print_artist(gt_artist_t *artist)
{
	int		i;
	gt_goetcorr_t	*goetcorr;
	char		*gtypestr;
	char		*goetname;
	gt_track_t	*trk;
	int			ret;
	char 		gmd_id[GMD_GLOBAL_ID_SIZE];

	if (data_format == GT_DATA_FORMAT_GMD) {
		ret = gt_get_gmd_id_from_numerical(artist->ga_id, gmd_id);
		if (ret != 0)
			return;
		printf("ID=%s\n", gmd_id);
	}
	else
		printf("ID=%d\n", artist->ga_id);
	printf("Name=%s\n", artist->ga_name);
	printf("GOETs (%i):\n", artist->ga_goetcorr_cnt);
	for(i = 0; i < artist->ga_goetcorr_cnt; i++) {
		goetcorr = &artist->ga_goetcorr[i];
		gtypestr = gt_get_goet_typestr(goetcorr->gc_type);
		goetname = gt_get_goet_name(goetcorr->gc_id, goetcorr->gc_type);
		if(gtypestr && goetname) {
			printf("  %d. Type=%s, ID=%d, Weight=%d, Name=%s\n",
			    i + 1, gtypestr, goetcorr->gc_id,
			    goetcorr->gc_weight, goetname);
		} else {
			printf("Invalid! descriptor type = %d, descriptor id = %d, weight = %d\n", goetcorr->gc_type, goetcorr->gc_id, goetcorr->gc_weight);
		}
	}

    if (data_format == GT_DATA_FORMAT_GMD && do_one_recording_per_song)
        printf("Songs (%d):\n", artist->ga_tracks_cnt);
    else
        printf("Recordings (%d):\n", artist->ga_tracks_cnt);
	for(i = 0; i < 5 && i < artist->ga_tracks_cnt; ++i) {
		ret = gt_lookup_track(artist->ga_tracks[i], &trk);
		if(ret != 0)
            continue;
		//printf("  %d. Title=%s, Album=%s, ID=%d\n", i + 1, trk->gt_name,
		//    trk->gt_albname, trk->gt_id);
		if (data_format == GT_DATA_FORMAT_GMD) {
			ret = gt_get_gmd_id_from_numerical(trk->gt_id, gmd_id);
			if (ret != 0)
				continue;
			printf("  %d. Title=%s, ID=%s\n", i + 1, trk->gt_name,
			    gmd_id);
		}
		else
			printf("  %d. Title=%s, ID=%d\n", i + 1, trk->gt_name,
			    trk->gt_id);

	}

}


void
print_track(gt_track_t *track)
{
	int		i;
	int 	ret;
	gt_goetcorr_t	*goetcorr;
	char		*gtypestr;
	char		*goetname;
	gt_artist_t	*art;
	char 		*tempo_name = "BPM";
	char 		gmd_id[GMD_GLOBAL_ID_SIZE];

	art = NULL;
	gt_lookup_artist(track->gt_art_id, &art);

	if (data_format == GT_DATA_FORMAT_GMD) {
		ret = gt_get_gmd_id_from_numerical(track->gt_id, gmd_id);
		if (ret != 0)
			return;
		printf("ID=%s\n", gmd_id);
	}
	else
		printf("ID=%d\n", track->gt_id);
	printf("Title=%s\n", track->gt_name?track->gt_name:"(null)");
	if(art)
		printf("Artist=%s\n", art->ga_name);
	else
		printf("Artist=(unknown)\n");
//	printf("Album=%s\n", track->gt_albname?track->gt_albname:"(null)");
	printf("GOETs (%i):\n", track->gt_goetcorr_cnt);
	for(i = 0; i < track->gt_goetcorr_cnt; i++) {
		goetcorr = &track->gt_goetcorr[i];
		gtypestr = gt_get_goet_typestr(goetcorr->gc_type);

		if (data_format != GT_DATA_FORMAT_GMD && goetcorr->gc_type == GT_TYPE_TEMPO_M)
		{
			goetname = tempo_name;
		}
		else
		{
			goetname = gt_get_goet_name(goetcorr->gc_id, goetcorr->gc_type);
		}

		if(gtypestr && goetname) {
			printf("  %d. Type=%s, ID=%d, Weight=%d, Name=%s\n",
			    i + 1, gtypestr, goetcorr->gc_id,
			    goetcorr->gc_weight, goetname);
		} else {
			printf("Invalid!\n");
		}
	}
}


void
print_goet(gt_goet_t *goet)
{
	int		i;
	gt_goetcorr_t	*goetcorr;
	char		*gtypestr;
	char		*goetname;
	unsigned	nodeid;
	gt_hier_node_t	*node;
	gt_hier_t	*hier;
	int		ret;

	gtypestr = gt_get_goet_typestr(goet->gg_type);
	printf("Type=%s\n", gtypestr?gtypestr:"Invalid!");
	printf("ID=%d\n", goet->gg_id);
	printf("Name=%s\n", goet->gg_name);
	printf("Correlated GOETs (%i):\n", goet->gg_goetcorr_cnt);
	for(i = 0; i < goet->gg_goetcorr_cnt; i++) {
		goetcorr = &goet->gg_goetcorr[i];
		gtypestr = gt_get_goet_typestr(goetcorr->gc_type);
		goetname = gt_get_goet_name(goetcorr->gc_id, goetcorr->gc_type);
		if(goetcorr->gc_weight >= simthreshold) {
			printf("  %d. Type=%s, ID=%d, Weight=%d, Name=%s\n",
			    i + 1, gtypestr?gtypestr:"Invalid!",
			    goetcorr->gc_id, goetcorr->gc_weight,
			    goetname?goetname:"Invalid!");
		}
	}
	printf("Links to hierarchy nodes (%i):\n", goet->gg_hier_node_ids_cnt);
	for(i = 0; i < goet->gg_hier_node_ids_cnt; i++) {
		nodeid = goet->gg_hier_node_ids[i];
		ret = gt_lookup_hierarchy_node(nodeid, &node);
		if(ret != 0)
			continue;

		ret = gt_lookup_hierarchy(node->gn_hier_id, &hier);
		if(ret != 0)
			continue;

		printf(" %d. %d %s | %d %s\n", i + 1,
		    hier->gh_id, hier->gh_name,
		    node->gn_id, node->gn_name);
	}

}


void
print_hierarchy_node(gt_hier_node_t *node, int maxlevel, int printchildren)
{
	int		i;
	gt_hier_node_t	*child;
	char		*name;

	if(node->gn_level > maxlevel)
		return;

	for(i = 0; i < node->gn_level; ++i) {
		printf("   ");
	}

	name = node->gn_name;
	for(i = 0; i < node->gn_locs_cnt; ++i) {
		if(node->gn_locs[i].go_langid == language)
			name = node->gn_locs[i].go_str;
	}

	printf("+ %s | ID=%d\n", name, node->gn_id);

	if(printchildren) {
		for(child = node->gn_first_child; child != NULL;
		    child = child->gn_next_sibling) {
			print_hierarchy_node(child, maxlevel, printchildren);
		}
	}

}


void
print_hierarchy(gt_hier_t *hier, int maxlevel)
{
	gt_hier_node_t	*node;

	printf("%s\n", hier->gh_name);
	for(node = hier->gh_first_top; node != NULL;
	    node = node->gn_next_sibling) {
		print_hierarchy_node(node, maxlevel, 1);
	}
}


void
print_hierarchy_node_full(gt_hier_node_t *node)
{
	gt_hier_t	*hier;
	gt_hier_node_t	*parent;
	int		ret;
	int		i;
	gt_lang_t	*lang;
	gt_locstr_t	*loc;
	unsigned	goetid;
	gt_goet_t	*goet;

	ret = gt_lookup_hierarchy(node->gn_hier_id, &hier);
	if(ret != 0)
		hier = NULL;

	printf("ID=%d\n", node->gn_id);
	printf("Name=%s\n", node->gn_name);
	if(hier != NULL)
		printf("Hierarchy=%d %s\n", hier->gh_id, hier->gh_name);
	else
		printf("No hierarchy!\n");
	printf("Level=%d\n", node->gn_level);

	printf("Parent / children:\n");
	if(node->gn_level > 1) {
		ret = gt_lookup_hierarchy_node(node->gn_parentid, &parent);
		if(ret == 0) {
			print_hierarchy_node(parent, 3, 0);
		}
	}
	print_hierarchy_node(node, node->gn_level + 1, 1);

	printf("Localizations (%d):\n", node->gn_locs_cnt);
	for(i = 0; i < node->gn_locs_cnt; ++i) {
		loc = &node->gn_locs[i];
		ret = gt_lookup_lang(loc->go_langid, &lang);
		if(ret != 0)
			continue;
		printf("(%d %s) %s\n",
		    lang->gl_id, lang->gl_iso, loc->go_str);
	}

	printf("Linked GOETs (%i):\n", node->gn_goet_ids_cnt);
	for(i = 0; i < node->gn_goet_ids_cnt; i++) {
		goetid = node->gn_goet_ids[i];
		ret = gt_lookup_goet(goetid, &goet);
		if(ret != 0)
			continue;

		printf(" %d. %d %s\n", i + 1,
		    goet->gg_id, goet->gg_name);
	}
}

#define MAXFILELEN    2048
#define MAXINPUTLEN    1024
#define MAXSEARCHGOET    20

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void exec_simtrack(char *input, char *output){
    FILE *inF = NULL;
    FILE *outF = NULL;
    
    char        gmd_id[GMD_GLOBAL_ID_SIZE] = {0};
    int         ret;
    int         id;
    unsigned int uid;
    gt_track_t    *track;
    gt_track_t    *trks;
    int        rescnt;
    int        i;
    char        input_file_path[MAXINPUTLEN]= {0};
    char        output_file_path[MAXINPUTLEN]= {0};
    
    //time check
    clock_t startTime, endTime;
    double nProcessExcuteTime;
    
    char strTemp[255];
    char *pStr;
    
    strncpy(input_file_path, input, strlen(input));
    strncpy(output_file_path, output, strlen(output));
    
    printf("%s\n",input_file_path);
    printf("%s\n",output_file_path);
    
    inF = fopen(input_file_path, "r" );
    outF = fopen(output_file_path, "w");

    while( !feof( inF ) )
    {
        startTime = clock(); //현재 시각을 구한다.
        pStr = fgets( strTemp, sizeof(strTemp), inF );
        
        if (!pStr) {
            printf("End of file\n");
            break;
        }
        remove_newline(strTemp);
        
        if (data_format == GT_DATA_FORMAT_GMD) {
            strncpy(gmd_id, pStr, GMD_GLOBAL_ID_SIZE-1);
            
            gt_log(gmd_id);
            ret = gt_get_numerical_id_from_gmd(gmd_id, &uid);
            if (ret != 0)
                id = 0;
            else
                id = (int) uid;
        }
        else
            id = atoi(pStr);
        if(id > 0) {
            ret = gt_lookup_track(id, &track);
            if(ret == ENOENT) {
                printf("Not found.\n");
            } else if(ret != 0) {
                printf("Error.\n");
            } else {
                printf("Searching for recordings similar to %s...\n",track->gt_name);
            }
            
            trks = NULL;
            ret = gt_search_track_by_similarity(0, id, simthreshold, popthreshold, 0, &trks, &rescnt);
            if(ret == ENOENT) {
                printf("Not found.\n");
            } else if(ret != 0) {
                printf("Error.\n");
            } else {
                for(i = 0; i < rescnt; ++i) {
                    if (data_format == GT_DATA_FORMAT_GMD) {
                        ret = gt_get_gmd_id_from_numerical(trks[i].gt_id, gmd_id);
                        if (ret != 0)
                            continue;
                        fprintf(outF,"%s,%s,%4d,%s\n",strTemp,gmd_id,trks[i].gt_match_score,trks[i].gt_name);
                    }
                    else {
                        fprintf(outF,"%s,%10d,%4d,%s\n",strTemp,trks[i].gt_id,trks[i].gt_match_score,trks[i].gt_name);
                    }
                    if(maxres && i > maxres)
                        break;
                }
            }
            if(trks) {
                gt_free_track_reslist(trks);
                trks = NULL;
            }
        }
        
        endTime = clock(); //현재 시각을 구한다.
        nProcessExcuteTime = ( (double)(endTime - startTime) ) / CLOCKS_PER_SEC;
        printf("Excute time: %f\n", nProcessExcuteTime);
        if(ret != 0) {
            fprintf(stderr, "Could not load tables! %s\n", strTemp);
            //exit(-1);
        }
    }
    
    fclose( inF ); inF = NULL;
    fclose( outF ); outF = NULL;
}

void exec_simartist(char *input, char *output){
    FILE *inF = NULL;
    FILE *outF = NULL;
    
    char         gmd_id[GMD_GLOBAL_ID_SIZE] = {0};
    int        ret;
    int        id;
    unsigned int uid;
    gt_artist_t    *artist;
    gt_artist_t    *arts;
    int        rescnt;
    int        i;
    char        input_file_path[MAXINPUTLEN]= {0};
    char        output_file_path[MAXINPUTLEN]= {0};
    
    //time check
    clock_t startTime, endTime;
    double nProcessExcuteTime;
    
    char strTemp[255];
    char *pStr;
    
    strncpy(input_file_path, input, strlen(input));
    strncpy(output_file_path, output, strlen(output));
    
    printf("%s\n",input_file_path);
    printf("%s\n",output_file_path);
    
    inF = fopen(input_file_path, "r" );
    outF = fopen(output_file_path, "w");
    
    while( !feof( inF ) )
    {
        startTime = clock(); //현재 시각을 구한다.
        pStr = fgets( strTemp, sizeof(strTemp), inF );
        if (!pStr) {
            printf("End of file\n");
            break;
        }
        remove_newline(strTemp);
        
        if (data_format == GT_DATA_FORMAT_GMD) {
            strncpy(gmd_id, pStr, GMD_GLOBAL_ID_SIZE-1);
            
            gt_log(gmd_id);
            ret = gt_get_numerical_id_from_gmd(gmd_id, &uid);
            if (ret != 0)
                id = 0;
            else
                id = (int) uid;
        }
        else
            id = atoi(pStr);
        if(id > 0) {
            ret = gt_lookup_artist(id, &artist);
            if(ret == ENOENT) {
                printf("Not found.\n");
            } else if(ret != 0) {
                printf("Error.\n");
            } else {
                printf("Searching for artists similar to %s...\n", artist->ga_name);
            }
            
            arts = NULL;
            ret = gt_search_artist_by_similarity(id, simthreshold, popthreshold, 0, &arts, &rescnt);
            
            if(ret == ENOENT) {
                printf("Not found.\n");
            } else if(ret != 0) {
                printf("Error.\n");
            } else {
                for(i = 0; i < rescnt; ++i) {
                    if (data_format == GT_DATA_FORMAT_GMD) {
                        ret = gt_get_gmd_id_from_numerical(arts[i].ga_id, gmd_id);
                        if (ret != 0)
                            continue;
                        fprintf(outF,"%s,%s,%4d\n",strTemp,gmd_id,arts[i].ga_match_score);
                    }
                    else {
                        fprintf(outF,"%s,%10d,%4d\n",strTemp,arts[i].ga_id,arts[i].ga_match_score);
                    }
                    if(maxres && i > maxres)
                        break;
                }
            }
            if(arts) {
                gt_free_artist_reslist(arts);
                arts = NULL;
            }
        }
        
        endTime = clock(); //현재 시각을 구한다.
        nProcessExcuteTime = ( (double)(endTime - startTime) ) / CLOCKS_PER_SEC;
        printf("Excute time: %f\n", nProcessExcuteTime);
        if(ret != 0) {
            fprintf(stderr, "Could not load tables! %s\n", strTemp);
            //exit(-1);
        }
    }
    
    fclose( inF ); inF = NULL;
    fclose( outF ); outF = NULL;
}



int main(int argc, char **argv)
{
    int        arg;
    char        configfile[MAXFILELEN];
    int        ret;
    int         nMode = 0;
    
    memset(configfile, 0, MAXFILELEN);
  
    while((arg = getopt(argc, argv, "taA")) != -1) {
        switch(arg) {
            case 't':
                nMode = 1;
                break;
            case 'a':
                nMode = 2;
                break;
            case 'A':
                nMode = 3;
                break;
        }
    }
    
    if ( nMode == 1 && argc == 4) {
        printf ("Track similarity.\n");
    } else if ( nMode == 2 && argc == 4) {
        printf ("Artist similarity.\n");
    } else if ( nMode == 3 && argc == 6) {
        printf ("Track&Artist similarity.\n");
    } else {
        printf("Error: Invalid input\n");
        printf("   mdsbrowser -t <input file path> <output file path>\n");
        printf("   mdsbrowser -a <input file path> <output file path>\n");
        printf("   mdsbrowser -A <input track file path> <output track file path> <input artist file path> <output artist file path>\n");
        exit(-1);
    }
    
    if(!strlen(configfile))
        snprintf(configfile, MAXFILELEN - 1, "%s", DEFAULT_CONFIGFILE);
    
    ret = gt_init();
    if(ret != 0) {
        fprintf(stderr, "Could not initialize tables library.\n");
        exit(-1);
    }
    
    (void) gt_set_logger(logger);
    
    ret = load_config(configfile);
    if(ret != 0) {
        fprintf(stderr, "Could not load configuration file.\n");
        gt_uninit();
        exit(-1);
    }

    ret = gt_load_tables(do_load_recordings, do_load_hierarchies, data_format, do_one_recording_per_song);
    if(ret != 0) {
        fprintf(stderr, "Could not load tables!\n");
        exit(-1);
    }
    
    if (nMode == 1) {
        exec_simtrack(argv[2], argv[3]);
    } else if (nMode == 2){
        exec_simartist(argv[2], argv[3]);
    } else if (nMode == 3){
        exec_simtrack(argv[2], argv[3]);
        exec_simartist(argv[4], argv[5]);
    }
}



int
ori_main(int argc, char **argv)
{
	int		arg;
	char		configfile[MAXFILELEN];
	int		ret;
	char		input[MAXINPUTLEN];
	char 		gmd_id[GMD_GLOBAL_ID_SIZE] = {0};
	char		*ch;
	char		*ch2;
	char		*searchstr;
	gt_artist_t	*artist;
	gt_artist_t	*arts;
	gt_goet_t	*goets;
	int		id;
	int		id2;
	unsigned int uid;
	unsigned int uid2;
	int		weight;
	int		rescnt;
	int		i;
	gt_goet_t	*goet;
	gt_goet_t	*goet2;
	int		val;
	gt_search_goet_t searchgoets[MAXSEARCHGOET];
	int		searchgoetcnt;
	gt_similarity_t	sim;
	gt_track_t	*track;
	gt_track_t	*trks;
	gt_hier_t	*hier;
	int		maxlevel;
	gt_hier_node_t	*node;
	gt_lang_t	*lang;

	memset(configfile, 0, MAXFILELEN);


	while((arg = getopt(argc, argv, "c:s")) != -1) {
		switch(arg) {
		case 'c':
			snprintf(configfile, MAXFILELEN - 1, "%s", optarg);
			break;
		case 's':
			scriptmode = 1;
			break;
		case '?':
			if(optopt == 'c') {
				fprintf(stderr,
				    "Option -c requires an argument\n");
				exit(-1);
			} else
			if(isprint(optopt)) {
				fprintf (stderr,
				    "Unknown option `-%c'.\n\n", optopt);
				print_usage(argv[0]);
				exit(-1);
			} else {
				fprintf (stderr,
				    "Unknown option character `\\x%x'.\n\n",
				    optopt);
				print_usage(argv[0]);
				exit(-1);
			}
			break;
		default:
			print_usage(argv[0]);
			exit(-1);
		}
	}

	if(!strlen(configfile))
		snprintf(configfile, MAXFILELEN - 1, "%s", DEFAULT_CONFIGFILE);

	ret = gt_init();
	if(ret != 0) {
		fprintf(stderr, "Could not initialize tables library.\n");
		exit(-1);
	}

	(void) gt_set_logger(logger);

	ret = load_config(configfile);
	if(ret != 0) {
		fprintf(stderr, "Could not load configuration file.\n");
		gt_uninit();
		exit(-1);
	}

	ret = gt_load_tables(do_load_recordings, do_load_hierarchies, data_format, do_one_recording_per_song);
	if(ret != 0) {
		fprintf(stderr, "Could not load tables!\n");
		exit(-1);
	}

	if(!scriptmode)
		printf("> ");
	while(fgets(input, MAXINPUTLEN, stdin)) {

		/* Remove line break at end. */
		remove_newline(input);

		if(!strlen(input) || !strncmp(input, "#", 1)) {
			/* Ignore empty lines and comments. */
			if(!scriptmode)
				printf("> ");
			continue;
		}

		if(scriptmode) {
			printf("> %s\n", input);
		}

		if(!strcmp(input, "exit") || !strcmp(input, "quit") ||
		    !strcmp(input, "q")) {
			break;
		} else
		if(!strcmp(input, "simthreshold")) {
			printf("simthreshold = %d\n", simthreshold);
		} else
		if(!strncmp(input, "simthreshold ", 13) && strlen(input) > 13) {
			val = atoi(input + 13);
			simthreshold = val;
			printf("simthreshold = %d\n", simthreshold);
		} else
		if(!strcmp(input, "popthreshold")) {
			printf("popthreshold = %d\n", popthreshold);
		} else
		if(!strncmp(input, "popthreshold ", 13) && strlen(input) > 13) {
			val = atoi(input + 13);
			if(val >= 0) {
				popthreshold = val;
				printf("popthreshold = %d\n", popthreshold);
			}
		} else
		if(!strcmp(input, "maxres")) {
			printf("maxres = %d\n", maxres);
		} else
		if(!strncmp(input, "maxres ", 7) && strlen(input) > 7) {
			val = atoi(input + 7);
			maxres = val;
			printf("maxres = %d\n", maxres);
		} else
		if(!strcmp(input, "verbose")) {
			printf("verbose is %s\n", verbose?"on":"off");
		} else
		if(!strcmp(input, "verbose on")) {
			verbose = 1;
			printf("verbose turned on\n");
		} else
		if(!strcmp(input, "verbose off")) {
			verbose = 0;
			printf("verbose turned off\n");
		} else
		if(!strncmp(input, "search ", 7) && strlen(input) > 7) {
			searchstr = input + 7;

			printf("Searching for %s...\n", searchstr);

			arts = NULL;
			ret = gt_search_artist_by_name(searchstr, &arts,
			    &rescnt);
			if(ret == ENOENT) {
				printf("Not found.\n");
			} else if(ret != 0) {
				printf("Error.\n");
			} else {
				for(i = 0; i < rescnt; ++i) {
					if (data_format == GT_DATA_FORMAT_GMD) {
						ret = gt_get_gmd_id_from_numerical(arts[i].ga_id, gmd_id);
						if (ret != 0)
							continue;
						printf("%s %s\n",
						    gmd_id, arts[i].ga_name);
					}
					else
						printf("%10d %s\n",
						    arts[i].ga_id, arts[i].ga_name);
					if(maxres && i > maxres)
						break;
				}
			}
			if(arts) {
				gt_free_artist_reslist(arts);
				arts = NULL;
			}

		} else
		if(!strncmp(input, "artist ", 7) && strlen(input) > 7) {
			strncpy(gmd_id, input+7, GMD_GLOBAL_ID_SIZE-1);
			if (data_format == GT_DATA_FORMAT_GMD) {
				ret = gt_get_numerical_id_from_gmd(gmd_id, &uid);
				if (ret != 0)
					id = 0;
				else
					id = (int) uid;
			}
			else
				id = atoi(input + 7);
			if(id > 0) {
				printf("Fetching artist %s...\n", gmd_id);

				ret = gt_lookup_artist(id, &artist);
				if(ret == ENOENT) {
					printf("Not found.\n");
				} else if(ret != 0) {
					printf("Error.\n");
				} else {
					print_artist(artist);
				}
			}

		} else
		if(!strncmp(input, "artistsim ", 10) && strlen(input) > 10) {
			id = id2 = 0;
			ch = input + 10;
			ch2 = strchr(ch, ' ');
			if(ch2) {
				ch2++;
				if (data_format == GT_DATA_FORMAT_GMD) {
					strncpy(gmd_id, ch, GMD_GLOBAL_ID_SIZE-1);
					ret = gt_get_numerical_id_from_gmd(gmd_id, &uid);
					if (ret != 0)
						id = 0;
					else
						id = (int) uid;
					strncpy(gmd_id, ch2, GMD_GLOBAL_ID_SIZE-1);
					ret = gt_get_numerical_id_from_gmd(gmd_id, &uid2);
					if (ret != 0)
						id2 = 0;
					else
						id2 = (int) uid2;
				}
				else {
					id = atoi(ch);
					id2 = atoi(ch2);
				}
			}
			if(id > 0 && id2 > 0) {
				ret = gt_compute_artist_similarity(id, id2,
				    GT_SIMFLAG_NO_EARLY_BAILOUT, verbose, &sim, NULL, 0);
				if(ret == ENOENT) {
						printf("Not found.\n");
				} else if(ret != 0) {
					printf("Error.\n");
				} else {
					printf("RESULT:\n");
					printf("  Step1 = %s\n", sim.gs_step1_pass?"PASS":"FAIL");
					printf("  Step2:\n");
					printf("    * Genre = %d\n", sim.gs_score_genre);
					printf("    * Origin = %d\n", sim.gs_score_origin);
					printf("    * Era = %d\n", sim.gs_score_era);
					printf("    * Artist Type = %d\n", sim.gs_score_artist_type);
					printf("    * Weighted = %d\n", sim.gs_score_weighted);
					printf("  ====================\n");
					printf("  SIMILARITY = %d\n", sim.gs_similarity_value);
				}
			} else
				printf("Invalid artist ID(s).\n");

		} else
		if(!strncmp(input, "rec ", 4) && strlen(input) > 4) {
			strncpy(gmd_id, input+4, GMD_GLOBAL_ID_SIZE-1);
			if (data_format == GT_DATA_FORMAT_GMD) {
				ret = gt_get_numerical_id_from_gmd(gmd_id, &uid);
				if (ret != 0)
					id = 0;
				else
					id = (int) uid;
			}
			else
				id = atoi(input + 4);
			if(id > 0) {
				printf("Fetching recording %s...\n", gmd_id);

				ret = gt_lookup_track(id, &track);
				if(ret == ENOENT) {
					printf("Not found.\n");
				} else if(ret != 0) {
					printf("Error.\n");
				} else {
					print_track(track);
				}
			}

		} else
		if(!strncmp(input, "recsim ", 7) && strlen(input) > 7) {
			id = id2 = 0;
			ch = input + 7;
			ch2 = strchr(ch, ' ');
			if(ch2) {
				ch2++;
				if (data_format == GT_DATA_FORMAT_GMD) {
					strncpy(gmd_id, ch, GMD_GLOBAL_ID_SIZE-1);
					ret = gt_get_numerical_id_from_gmd(gmd_id, &uid);
					if (ret != 0)
						id = 0;
					else
						id = (int) uid;
					strncpy(gmd_id, ch2, GMD_GLOBAL_ID_SIZE-1);
					ret = gt_get_numerical_id_from_gmd(gmd_id, &uid2);
					if (ret != 0)
						id2 = 0;
					else
						id2 = (int) uid2;
				}
				else {
					id = atoi(ch);
					id2 = atoi(ch2);
				}
			}
			if(id > 0 && id2 > 0) {
				ret = gt_compute_track_similarity(id, id2,
				    GT_SIMFLAG_NO_EARLY_BAILOUT, verbose, &sim, NULL, 0);
				if(ret == ENOENT) {
						printf("Not found.\n");
				} else if(ret != 0) {
					printf("Error.\n");
				} else {
					printf("RESULT:\n");
					printf("  Step1 = %s\n", sim.gs_step1_pass?"PASS":"FAIL");
					printf("  Step2:\n");
					printf("    * Genre = %d\n", sim.gs_score_genre);
					printf("    * Origin = %d\n", sim.gs_score_origin);
					printf("    * Era = %d\n", sim.gs_score_era);
					printf("    * Artist Type = %d\n", sim.gs_score_artist_type);
					printf("    * Mood = %d\n", sim.gs_score_mood);
					printf("    * Weighted = %d\n", sim.gs_score_weighted);
					printf("  ====================\n");
					printf("  SIMILARITY = %d\n", sim.gs_similarity_value);
				}
			} else
				printf("Invalid recording ID(s).\n");

		} else
		if(!strncmp(input, "a2recsim ", 9) && strlen(input) > 9) {
			id = id2 = 0;
			ch = input + 9;
			ch2 = strchr(ch, ' ');
			if(ch2) {
				ch2++;
				if (data_format == GT_DATA_FORMAT_GMD) {
					strncpy(gmd_id, ch, GMD_GLOBAL_ID_SIZE-1);
					ret = gt_get_numerical_id_from_gmd(gmd_id, &uid);
					if (ret != 0)
						id = 0;
					else
						id = (int) uid;
					strncpy(gmd_id, ch2, GMD_GLOBAL_ID_SIZE-1);
					ret = gt_get_numerical_id_from_gmd(gmd_id, &uid2);
					if (ret != 0)
						id2 = 0;
					else
						id2 = (int) uid2;
				}
				else {
					id = atoi(ch);
					id2 = atoi(ch2);
				}
			}
			if(id > 0 && id2 > 0) {
				ret = gt_compute_artist_to_track_similarity(
				    id, id2,
				    GT_SIMFLAG_NO_EARLY_BAILOUT, verbose, &sim, NULL, 0);
				if(ret == ENOENT) {
						printf("Not found.\n");
				} else if(ret != 0) {
					printf("Error.\n");
				} else {
					printf("RESULT:\n");
					printf("  Step1 = %s\n", sim.gs_step1_pass?"PASS":"FAIL");
					printf("  Step2:\n");
					printf("    * Genre = %d\n", sim.gs_score_genre);
					printf("    * Origin = %d\n", sim.gs_score_origin);
					printf("    * Era = %d\n", sim.gs_score_era);
					printf("    * Artist Type = %d\n", sim.gs_score_artist_type);
					printf("    * Weighted = %d\n", sim.gs_score_weighted);
					printf("  ====================\n");
					printf("  SIMILARITY = %d\n", sim.gs_similarity_value);
				}
			} else
				printf("Invalid artist/recording ID(s).\n");

		} else
		if(!strncmp(input, "goetcorr ", 9) && strlen(input) > 9) {
			id = id2 = 0;
			ch = input + 9;
			ch2 = strchr(ch, ' ');
			if(ch2) {
				ch2++;
				id = atoi(ch);
				id2 = atoi(ch2);
			}
			if(id > 0 && id2 > 0) {
				ret = gt_lookup_goet(id, &goet);
				if(ret == ENOENT) {
					printf("Not found.\n");
				} else if(ret != 0) {
					printf("Error.\n");
				} else {
					printf(" %s\n", goet->gg_name);
				}

				if(!ret) {
					ret = gt_lookup_goet(id2, &goet2);
					if(ret == ENOENT) {
						printf("Not found.\n");
					} else if(ret != 0) {
						printf("Error.\n");
					} else {
						printf(" %s\n", goet2->gg_name);
					}
					ret = gt_get_goet_correlation(id, id2);
					printf(" Correlation = %d\n", ret);
				}
			} else
				printf("Invalid GOET ID(s).\n");

		} else
		if(!strncmp(input, "goet ", 5) && strlen(input) > 5) {
			id = atoi(input + 5);
			if(id > 0) {
				printf("Fetching GOET %d...\n", id);

				ret = gt_lookup_goet(id, &goet);
				if(ret == ENOENT) {
					printf("Not found.\n");
				} else if(ret != 0) {
					printf("Error.\n");
				} else {
					print_goet(goet);
				}
			}
		} else
		if(!strncmp(input, "goetart ", 8) && strlen(input) > 8) {
			id = atoi(input + 8);
			if(id > 0) {
				printf("Searching for artists with %d...\n", id);
				arts = NULL;
				ret = gt_search_artist_by_goet(id, simthreshold,
				    popthreshold, &arts, &rescnt);
				if(ret == ENOENT) {
					printf("Not found.\n");
				} else if(ret != 0) {
					printf("Error.\n");
				} else {
					for(i = 0; i < rescnt; ++i) {
						if (data_format == GT_DATA_FORMAT_GMD) {
							ret = gt_get_gmd_id_from_numerical(arts[i].ga_id, gmd_id);
							if (ret != 0)
								continue;
							printf("%s %4d %s\n",
							    gmd_id,
							    arts[i].ga_match_score,
							    arts[i].ga_name);
						}
						else
							printf("%10d %4d %s\n",
							    arts[i].ga_id,
							    arts[i].ga_match_score,
							    arts[i].ga_name);
						if(maxres && i > maxres)
							break;
					}
				}
				if(arts) {
					gt_free_artist_reslist(arts);
					arts = NULL;
				}
			}

		} else
		if(!strncmp(input, "searchsim ", 10) && strlen(input) > 10) {
			if (data_format == GT_DATA_FORMAT_GMD) {
				strncpy(gmd_id, input+10, GMD_GLOBAL_ID_SIZE-1);
				ret = gt_get_numerical_id_from_gmd(gmd_id, &uid);
				if (ret != 0)
					id = 0;
				else
					id = (int) uid;
			}
			else
				id = atoi(input + 10);
			if(id > 0) {
				ret = gt_lookup_artist(id, &artist);
				if(ret == ENOENT) {
					printf("Not found.\n");
				} else if(ret != 0) {
					printf("Error.\n");
				} else {
					printf("Searching for artists similar to %s...\n", artist->ga_name);
				}

				arts = NULL;
				ret = gt_search_artist_by_similarity(id,
				    simthreshold, popthreshold, 0,
				    &arts, &rescnt);
				if(ret == ENOENT) {
					printf("Not found.\n");
				} else if(ret != 0) {
					printf("Error.\n");
				} else {
					for(i = 0; i < rescnt; ++i) {
						if (data_format == GT_DATA_FORMAT_GMD) {
							ret = gt_get_gmd_id_from_numerical(arts[i].ga_id, gmd_id);
							if (ret != 0)
								continue;
							printf("%s %4d %s\n",
							    gmd_id,
							    arts[i].ga_match_score,
							    arts[i].ga_name);
						}
						else
							printf("%10d %4d %s\n",
							    arts[i].ga_id,
							    arts[i].ga_match_score,
							    arts[i].ga_name);
						if(maxres && i > maxres)
							break;
					}
				}
				if(arts) {
					gt_free_artist_reslist(arts);
					arts = NULL;
				}
			}

		} else
		if(!strncmp(input, "searchsimrec ", 13) && strlen(input) > 13) {
			if (data_format == GT_DATA_FORMAT_GMD) {
				strncpy(gmd_id, input+13, GMD_GLOBAL_ID_SIZE-1);
				ret = gt_get_numerical_id_from_gmd(gmd_id, &uid);
				if (ret != 0)
					id = 0;
				else
					id = (int) uid;
			}
			else
				id = atoi(input + 13);
			if(id > 0) {
				ret = gt_lookup_track(id, &track);
				if(ret == ENOENT) {
					printf("Not found.\n");
				} else if(ret != 0) {
					printf("Error.\n");
				} else {
					printf("Searching for recordings"
					    " similar to %s...\n",
					    track->gt_name);
				}

				trks = NULL;
				ret = gt_search_track_by_similarity(0, id,
				    simthreshold, popthreshold, 0,
				    &trks, &rescnt);
				if(ret == ENOENT) {
					printf("Not found.\n");
				} else if(ret != 0) {
					printf("Error.\n");
				} else {
					for(i = 0; i < rescnt; ++i) {
						if (data_format == GT_DATA_FORMAT_GMD) {
							ret = gt_get_gmd_id_from_numerical(trks[i].gt_id, gmd_id);
							if (ret != 0)
								continue;
							printf("%s %4d %s\n",
							    gmd_id,
							    trks[i].gt_match_score,
							    trks[i].gt_name);
						}
						else
							printf("%10d %4d %s\n",
							    trks[i].gt_id,
							    trks[i].gt_match_score,
							    trks[i].gt_name);
						if(maxres && i > maxres)
							break;
					}
				}
				if(trks) {
					gt_free_track_reslist(trks);
					trks = NULL;
				}
			}
		} else
		if(!strncmp(input, "searchsima2rec ", 15) &&
		    strlen(input) > 15) {
			if (data_format == GT_DATA_FORMAT_GMD) {
				strncpy(gmd_id, input+15, GMD_GLOBAL_ID_SIZE-1);
				ret = gt_get_numerical_id_from_gmd(gmd_id, &uid);
				if (ret != 0)
					id = 0;
				else
					id = (int) uid;
			}
			else
				id = atoi(input + 15);
			if(id > 0) {
				ret = gt_lookup_artist(id, &artist);
				if(ret == ENOENT) {
					printf("Not found.\n");
				} else if(ret != 0) {
					printf("Error.\n");
				} else {
					printf("Searching for recordings"
					    " similar to %s...\n",
					    artist->ga_name);
				}

				trks = NULL;
				ret = gt_search_track_by_similarity(id, 0,
				    simthreshold, popthreshold, 0,
				    &trks, &rescnt);
				if(ret == ENOENT) {
					printf("Not found.\n");
				} else if(ret != 0) {
					printf("Error.\n");
				} else {
					for(i = 0; i < rescnt; ++i) {
						ret = gt_lookup_artist(trks[i].gt_art_id,
						    &artist);
						if(ret != 0)
							artist = NULL;
						if (data_format == GT_DATA_FORMAT_GMD) {
							ret = gt_get_gmd_id_from_numerical(trks[i].gt_id, gmd_id);
							if (ret != 0)
								continue;
							printf("%s %4d %4d %s - %s - %s\n",
								gmd_id,
							    trks[i].gt_match_score,
							    trks[i].gt_pop,
							    artist?artist->ga_name:"(invalid)",
							    trks[i].gt_name,
							    trks[i].gt_albname);
						}
						else
							printf("%10d %4d %4d %s - %s - %s\n",
							    trks[i].gt_id,
							    trks[i].gt_match_score,
							    trks[i].gt_pop,
							    artist?artist->ga_name:"(invalid)",
							    trks[i].gt_name,
							    trks[i].gt_albname);
						if(maxres && i > maxres)
							break;
					}
				}
				if(trks) {
					gt_free_track_reslist(trks);
					trks = NULL;
				}
			}
		} else
		if(!strncmp(input, "goetsim ", 8) && strlen(input) > 8) {
			searchgoetcnt = 0;
			memset(searchgoets, 0,
			    MAXSEARCHGOET * sizeof(gt_search_goet_t));
			ch = strtok(input + 8, ",");
			while(ch) {
				ch2 = strchr(ch, '/');
				if(!ch2) {
					continue;
				}
				*ch2 = 0;
				++ch2;
				id = atoi(ch);
				weight = atoi(ch2);
				if(id <=0 || weight <=0)
					continue;
				searchgoets[searchgoetcnt].gs_goet_id = id;
				searchgoets[searchgoetcnt].gs_weight = weight;
				searchgoetcnt++;
				if(searchgoetcnt >= MAXSEARCHGOET)
					break;

    				ch = strtok(NULL, ",");
			}

			if(searchgoetcnt > 0) {
				printf("Searching for artists similar to specified pie (%d GOETs)...\n", searchgoetcnt);
				arts = NULL;
				ret = gt_search_artist_by_goetlist(
				    searchgoets, searchgoetcnt, simthreshold,
				    popthreshold, 1, &arts, &rescnt);
				if(ret == ENOENT) {
					printf("Not found.\n");
				} else if(ret != 0) {
					printf("Error.\n");
				} else {
					for(i = 0; i < rescnt; ++i) {
						if (data_format == GT_DATA_FORMAT_GMD) {
							ret = gt_get_gmd_id_from_numerical(arts[i].ga_id, gmd_id);
							if (ret != 0)
								continue;
							printf("%s %4d %s\n",
							    gmd_id,
							    arts[i].ga_match_score,
							    arts[i].ga_name);
						}
						else
							printf("%10d %4d %s\n",
							    arts[i].ga_id,
							    arts[i].ga_match_score,
							    arts[i].ga_name);
						if(maxres && i > maxres)
							break;
					}
				}
				if(arts) {
					gt_free_artist_reslist(arts);
					arts = NULL;
				}
			}

		} else if(!strncmp(input, "searchgoet ", 11)
		    && strlen(input) > 11) {
			searchstr = input + 11;

			printf("Searching for %s...\n", searchstr);

			goets = NULL;
			ret = gt_search_goet_by_name(searchstr, &goets,
			    &rescnt);
			if(ret == ENOENT) {
				printf("Not found.\n");
			} else if(ret != 0) {
				printf("Error.\n");
			} else {
				for(i = 0; i < rescnt; ++i) {
					printf("%6d (%s) %s\n",
					    goets[i].gg_id,
					    gt_get_goet_typestr(goets[i].gg_type),
					    goets[i].gg_name);

					if(maxres && i > maxres)
						break;
}


			}
			if(arts) {
				gt_free_goet_reslist(goets);
				arts = NULL;
			}
		} else if(!strncmp(input, "hiers", 5)) {
			ret = gt_get_full_hierlist(&hier, &rescnt);
			if(ret == 0) {
				for(i = 0; i < rescnt; ++i) {
					printf("%4d %s\n", hier[i].gh_id,
					    hier[i].gh_name);
				}
			} else {
				printf("Error.\n");
			}
		} else if(!strncmp(input, "hier ", 5)
		    && strlen(input) > 5) {

			ret = sscanf(input + 5, "%d %d", &id, &maxlevel);
			if(ret == 1 || ret == 2) {
				if(ret == 1)
					maxlevel = 3;

				printf("Retrieving hierarchy %d...\n", id);

				ret = gt_lookup_hierarchy(id, &hier);
				if(ret == ENOENT) {
					printf("Not found.\n");
				} else if(ret != 0) {
					printf("Error.\n");
				} else {
					print_hierarchy(hier, maxlevel);
				}
			} else {
				printf("Invalid input.\n");
			}
		} else if(!strncmp(input, "node ", 5)
		    && strlen(input) > 5) {

			id = atoi(input + 5);

			printf("Retrieving hierarchy node %d...\n", id);

			ret = gt_lookup_hierarchy_node(id, &node);
			if(ret == ENOENT) {
				printf("Not found.\n");
			} else if(ret != 0) {
				printf("Error.\n");
			} else {
				print_hierarchy_node_full(node);
			}
		} else if(!strncmp(input, "langs", 5)) {
			ret = gt_get_full_langlist(&lang, &rescnt);
			if(ret == 0) {
				for(i = 0; i < rescnt; ++i) {
					printf("%4d %s (%s)\n", lang[i].gl_id,
					    lang[i].gl_name, lang[i].gl_iso);
				}
			} else {
				printf("Error.\n");
			}
		} else
		if(!strcmp(input, "lang")) {
			ret = gt_lookup_lang(language, &lang);
			if(ret == 0) {
	 			printf("lang=%d %s (%s)\n", lang->gl_id,
				    lang->gl_name, lang->gl_iso);
			} else {
	 			printf("Error.\n");
			}
		} else if(!strncmp(input, "lang ", 5)
		    && strlen(input) > 5) {

			id = atoi(input + 5);

			ret = gt_lookup_lang(id, &lang);
			if(ret == 0) {
				language = id;
	 			printf("lang=%d %s (%s)\n", lang->gl_id,
				    lang->gl_name, lang->gl_iso);
			} else {
	 			printf("Bad language specified.\n");
			}
		} else if(!strncmp(input, "about", 5)) {
			printf("   MDS Browser\n");
			printf("   %s\n", COPYRIGHT);
			printf("   Build date: %s\n", BUILDDATE);
			printf("   Build host: %s\n", BUILDHOST);
			printf("   Contact: %s\n", CONTACT_EMAIL);
		} else if(!strncmp(input, "help", 4)) {
			printf("   about -- Prints information on this program.\n");
			printf("   search <name> -- Search artists by name.\n");
			printf("   artist <Artist ID> -- Print artist's information.\n");
			printf("   artistsim <Artist ID> <Artist ID> -- Print two artists' similarity score.\n");
			printf("   searchsim <Artist ID> -- Search for artists similar to the one specified.\n");
			printf("   goet <GOET ID> -- Print GOET's information.\n");
			printf("   goetcorr <GOET ID> <GOET ID> -- Print two GOETs' correlation value.\n");
			printf("   searchgoet <name> -- Search GOETs by name.\n");
			printf("   goetart <GOET ID> -- Get artists for a GOET.\n");
			if (do_one_recording_per_song == 1)
            {
                printf("   rec <Song ID> -- Print song's information.\n");
                printf("   searchsimrec <Song ID> -- Search for songs similar to the one specified.\n");
            }
            else
            {
                printf("   rec <Recording ID> -- Print recording's information.\n");
                printf("   searchsimrec <Recording ID> -- Search for recordings similar to the one specified.\n");
            }

			printf("   simthreshold [value] -- View or set similarity / GOET correlate threshold.\n");
			printf("   popthreshold [value] -- View or set popularity threshold.\n");
			printf("   maxres [value] -- View or set number of max search results displayed (0 = no limit).\n");
			printf("   hiers -- View the list of available hierarchies.\n");
			printf("   hier <HIER ID> [maxl] -- View the specified hierarchy. If specified, restrict output to maxl levels.\n");
			printf("   node <NODE ID> -- View the specified hierarchy node.\n");
			printf("   langs -- View the list of supported languages.\n");
			printf("   lang [LANG ID] -- View or set the display language for hierarchies.\n");
			printf("   help -- This help text.\n");
			printf("   quit or exit -- Exits the program.\n");

		} else if(strlen(input) > 0) {
			printf("? Unknown or incomplete command.\n");
		}

		if(!scriptmode)
			printf("> ");
	}


	ret = gt_uninit();

	return 0;
}

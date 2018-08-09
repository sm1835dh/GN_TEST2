#ifndef GMD_JSON_AS_H
#define GMD_JSON_AS_H

#define GMD_MAX_LINE 2048
#define GMD_JSON_BUFFER_SIZE_INIT   268435456
#define GMD_JSON_BUFFER_SIZE_GROW   67108864
#define GMD_JSON_MAX_KEY    128
#define GMD_JSON_NO_MORE_DATA   111

#define GMD_JSON_TOKEN_SIZE_INIT 256
#define GMD_JSON_TOKEN_SIZE_GROW 256

#define GMD_JSON_SIZE_CORRELATE_ID      32
#define GMD_JSON_SIZE_CORRELATE_SCORE   8

#define GMD_TYPE_OBJECT 0
#define GMD_TYPE_ARRAY 1

#define GMD_DESCRIPTOR_TYPE_GENRE       0
#define GMD_DESCRIPTOR_TYPE_ORIGIN      1
#define GMD_DESCRIPTOR_TYPE_ERA         2
#define GMD_DESCRIPTOR_TYPE_ATYPE       3
#define GMD_DESCRIPTOR_TYPE_MOOD        4
#define GMD_DESCRIPTOR_TYPE_TEMPO       5
#define GMD_DESCRIPTOR_TYPE_LANG        7

#define GMD_KEY_ARTISTS                 "artists"
#define GMD_KEY_ARTIST_NAME             "artistName"
#define GMD_KEY_ARTIST_ID               "artistID"
#define GMD_KEY_ID                      "ID"
#define GMD_KEY_NAME                    "name"
#define GMD_KEY_DESCRIPTORS             "descriptors"
#define GMD_KEY_WEIGHT                  "weight"
#define GMD_KEY_GENRES                  "genres"
#define GMD_KEY_LANGUAGES               "languages"
#define GMD_KEY_ORIGINS                 "origins"
#define GMD_KEY_ERAS                    "eras"
#define GMD_KEY_ATYPES                  "artistTypes"
#define GMD_KEY_SONGS                   "songs"
#define GMD_KEY_SONG_ID                 "songID"
#define GMD_KEY_PREFERRED_RECORDING_ID  "preferredRecordingID"
#define GMD_KEY_RECORDINGS              "recordings"
#define GMD_KEY_RECORDING_ID            "recordingID"
#define GMD_KEY_RECORDING_NAME          "recordingName"
#define GMD_KEY_RELEASE_YEAR            "releaseYear"
#define GMD_KEY_MOODS                   "moods"
#define GMD_KEY_TEMPOS                  "tempos"
#define GMD_KEY_MASTERS                 "masters"
#define GMD_KEY_EDITIONS                "editions"
#define GMD_KEY_ALBUM_MASTER_ID         "albumMasterID"
#define GMD_KEY_ALBUM_MASTER_NAME       "albumMasterName"
#define GMD_KEY_PREFERRED_EDITION_ID    "preferredEditionID"
#define GMD_KEY_ALBUM_EDITION_ID        "albumEditionID"
#define GMD_KEY_ALBUM_EDITION_NAME      "albumEditionName"
#define GMD_KEY_RELEASE_TYPE            "releaseType"

#define GMD_CORRELATE_TEMP_FILE         "correlate_tmp.json"

#include <stdio.h>

#include "jsmn.h"

typedef struct {
    FILE *f; /* file pointer */
    int datatype; /* if the target json object is an array or dictionary */

    /* key to match, if NULL then match all keys at the level specified */
    char *key_filter;
    int level_key;
    int level_current;

    char *buffer; /* buffer that holds json string */
    int buffer_size;
    int buffer_cnt;

    char *output_str;
    int output_str_size;

    /* key string for dictionary objects */
    char key_match[GMD_JSON_MAX_KEY];

    char leftover_line[GMD_MAX_LINE];

    jsmn_parser p;
    jsmntok_t *tokens;
    int tokens_cnt;
    int tokens_size;
    int max_string_size;

    /* flags */
    int b_ignore_special_chars;
    int b_ignore_quote;
    int b_copy_on;
    int b_key_match;
    int b_key_match_once;

    int use_input_buffer;
    char *input_buffer;
    int b_input_buffer_new;

    void (*logger)(char *, ...);
} gmd_json_reader_t;

typedef struct {
    int type;
    char id[GMD_MAX_LINE];
    char weight[GMD_MAX_LINE];
} gmd_json_descriptor_t;

typedef struct {
    int type;
    char id1[GMD_JSON_SIZE_CORRELATE_ID];
    char id2[GMD_JSON_SIZE_CORRELATE_ID];
    char score[GMD_JSON_SIZE_CORRELATE_SCORE];
} gmd_json_correlate_t;

int gmd_json_reader_init_w_input_buffer(gmd_json_reader_t **reader, const char *szFilepath, int datatype, const char *szKey, int level_key, int max_string_size, int use_input_buffer, char *input_buffer, void (*logger)(char *, ...));
int gmd_json_reader_init(gmd_json_reader_t **reader, const char *szFilepath, int gmd_type, const char *szKey, int level_key, int max_string_size, void (*logger)(char *, ...));
int gmd_json_reader_uninit(gmd_json_reader_t *reader);
int gmd_json_reader_next_json_str(gmd_json_reader_t *reader);
int gmd_json_reader_get_descriptor_info(gmd_json_reader_t *reader, char *descriptor_name, char *descriptor_id);
int gmd_json_reader_get_artist_info(gmd_json_reader_t *reader, int max_descriptors, char *artist_id, char *artist_name, gmd_json_descriptor_t *descriptor_array, int *descriptor_array_cnt_out);
int gmd_json_reader_get_correlates(gmd_json_reader_t *reader, int type, gmd_json_correlate_t **correlate_array_out, int *correlate_array_cnt_out, int *correlate_array_size_out);
int gmd_json_reader_get_song_preferred_recording(gmd_json_reader_t *reader, char *song_id, char *recording_id);
int gmd_json_reader_get_recording_info(gmd_json_reader_t *reader,
    char *recording_id, char *recording_name, char *artist_id, char *artist_name,
    char *release_year, char *song_id, int max_descriptors,
    gmd_json_descriptor_t *descriptor_array, int *descriptor_array_cnt_out);
int gmd_json_reader_get_master_preferred_edition(gmd_json_reader_t *reader, char *preferred_edition_id, char *master_name, char *master_id, char *release_year);
int gmd_json_reader_get_edition_recordings(gmd_json_reader_t *reader, char *edition_id, int max_recordings, char **recording_ids, int *cnt_recording_ids, int *release_type_out);
int gmd_json_reader_get_edition_data(gmd_json_reader_t *reader, char *edition_id, char *master_id, char *edition_name);
int gmd_json_reader_get_master_data(gmd_json_reader_t *reader, char *master_id, char *master_name, char *release_year);

#endif

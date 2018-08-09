#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "gmd_json.h"
#include "jsmn.h"

static int gmd_json_reader_next_json_str_OBJECT(gmd_json_reader_t *reader);
static int gmd_json_reader_next_json_str_ARRAY(gmd_json_reader_t *reader);
static int process_chunk_OBJECT(gmd_json_reader_t *reader, char *line, int *b_done_out, int length);
static int process_chunk_ARRAY(gmd_json_reader_t *reader, char *line, int *b_done_out, int length);
static int add_char_to_buffer(gmd_json_reader_t *reader, char c);
static void check_for_key_match(gmd_json_reader_t *reader, char *chunk, int index);
static int gmd_json_reader_parse_json(gmd_json_reader_t *reader);
static int gmd_json_reader_token_string_copy(gmd_json_reader_t *reader, jsmntok_t *tok, char *str);
static int gmd_json_reader_token_string_copy_size(gmd_json_reader_t *reader, jsmntok_t *tok, char *str, int max_size);
static int gmd_json_reader_token_string_eq(gmd_json_reader_t *reader, jsmntok_t *tok, const char *str);
static int gmd_json_reader_descriptors_token_to_array(gmd_json_reader_t *reader, int *current_token_index, int max_descriptors, gmd_json_descriptor_t *descriptors, int *cnt_descriptors_out);

static char *get_next_chunk(char **text, const char *delim,  int *chunk_length);

int gmd_json_reader_init(gmd_json_reader_t **reader, const char *szFilepath, int datatype, const char *szKey, int level_key, int max_string_size, void (*logger)(char *, ...))
{
    return gmd_json_reader_init_w_input_buffer(
                                reader,
                                szFilepath,
                                datatype,
                                szKey,
                                level_key,
                                max_string_size,
                                0,
                                NULL,
                                logger
                                );
}

int gmd_json_reader_init_w_input_buffer(gmd_json_reader_t **reader, const char *szFilepath, int datatype, const char *szKey, int level_key, int max_string_size, int use_input_buffer, char *input_buffer, void (*logger)(char *, ...))
{
    int                 error = 0;
    gmd_json_reader_t    *r = NULL;

    /* init reader input to NULL */
    *reader = NULL;

    /* allocate data structure */
    r = (gmd_json_reader_t *) malloc(sizeof(gmd_json_reader_t));
    if (r == NULL)
    {
        error = ENOMEM;
        goto cleanup;
    }

    /* set logger first */
    r->logger = logger;

    /* initialize reader to default values */
    r->f = NULL;
    r->key_filter = NULL;
    r->level_key = level_key;
    r->level_current = 0;
    r->datatype = datatype;
    r->buffer = NULL;
    r->buffer_cnt = 0;
    r->buffer_size = 0;
    r->b_copy_on = 0;
    r->b_ignore_quote = 0;
    r->b_ignore_special_chars = 0;
    r->b_key_match = 0;
    r->b_key_match_once = 0;
    r->leftover_line[0] = '\0';
    r->key_match[0] = '\0';
    r->max_string_size = max_string_size;
    r->use_input_buffer = use_input_buffer;
    r->input_buffer = input_buffer;
    r->output_str = NULL;
    r->tokens = NULL;

    /* open file */
    if (!r->use_input_buffer)
    {
        r->f = fopen(szFilepath, "r");
        if (r->f == NULL)
        {
            r->logger("Error, could not open json file: %s", strerror(errno));
            error = ENOENT;
            goto cleanup;
        }
    }

    if (szKey != NULL)
    {
        if (strlen(szKey) >= GMD_JSON_MAX_KEY)
        {
            r->logger("Error, key too big!");
            error = ENOENT;
            goto cleanup;
        }

        r->key_filter = strdup(szKey);
        if (r->key_filter == NULL)
        {
            r->logger("Error, could not allocate key!");
            error = ENOMEM;
            goto cleanup;
        }
    }

    /* allocate buffer */
    r->buffer = (char *) malloc(sizeof(char) * GMD_JSON_BUFFER_SIZE_INIT);
    if (r->buffer == NULL)
    {
        r->logger("Error, could not allocate buffer!");
        error = ENOMEM;
        goto cleanup;
    }
    r->buffer_size = GMD_JSON_BUFFER_SIZE_INIT;

    /* allocate output str */
    r->output_str = (char *) malloc(sizeof(char) * GMD_JSON_BUFFER_SIZE_INIT);
    if (r->output_str == NULL)
    {
        r->logger("Error, could not allocate output str!");
        error = ENOMEM;
        goto cleanup;
    }
    r->output_str_size = GMD_JSON_BUFFER_SIZE_INIT;
    r->output_str[0] = '\0';

    /* allocate parser tokens */
    r->tokens = malloc(sizeof(jsmntok_t) * GMD_JSON_TOKEN_SIZE_INIT);
    if (r->tokens == NULL)
    {
        r->logger("Error, could not allocate tokens!");
        error = ENOMEM;
        goto cleanup;
    }
    r->tokens_size = GMD_JSON_TOKEN_SIZE_INIT;
    r->tokens_cnt = 0;

cleanup:
    if (error != 0)
    {
        gmd_json_reader_uninit(r);
    }
    else
    {
        *reader = r;
    }

    return error;
}

int gmd_json_reader_uninit(gmd_json_reader_t *reader)
{
    if (reader == NULL)
        return 0;

    if (reader->f)
    {
        fclose(reader->f);
    }

    if (reader->key_filter)
    {
        free(reader->key_filter);
    }

    if (reader->buffer)
    {
        free(reader->buffer);
    }

    if (reader->output_str)
    {
        free(reader->output_str);
    }

    if (reader->tokens)
    {
        free(reader->tokens);
    }

    free(reader);

    return 0;
}

int gmd_json_reader_next_json_str(gmd_json_reader_t *reader)
{
    if (reader->datatype == GMD_TYPE_OBJECT)
    {
        return gmd_json_reader_next_json_str_OBJECT(reader);
    }
    else if (reader->datatype == GMD_TYPE_ARRAY)
    {
        return gmd_json_reader_next_json_str_ARRAY(reader);
    }

    return 1;
}

int gmd_json_reader_get_descriptor_info(gmd_json_reader_t *reader, char *descriptor_name, char *descriptor_id)
{
    int ret = 0;
    int i = 0;
    int id_set = 0;
    int name_set = 0;

    ret = gmd_json_reader_next_json_str(reader);

    if (ret != 0) {
        return ret;
    }

    ret = gmd_json_reader_parse_json(reader);
    if (ret != 0)
    {
        return ret;
    }

    for (i = 0; i < reader->tokens_cnt; i++) {
        if (reader->tokens[i].type == JSMN_STRING) {

            // descriptor id
            if (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_ID))
            {
                ++i;
                ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], descriptor_id);
                if (ret != 0)
                    break;
                id_set = 1;
            }
            // descriptor name
            else if (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_NAME))
            {
                ++i;
                ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], descriptor_name);
                if (ret != 0)
                    break;
                name_set = 1;
            }

        }
    }

    if (id_set && name_set) {
        return 0;
    }

    return 1;
}

int gmd_json_reader_get_artist_info(gmd_json_reader_t *reader, int max_descriptors, char *artist_id, char *artist_name, gmd_json_descriptor_t *descriptor_array, int *descriptor_array_cnt_out)
{
    int ret = 0;
    int i = 0;
    int art_id_set = 0;
    int art_name_set = 0;
    int descriptors_set = 0;

    *descriptor_array_cnt_out = 0;

    ret = gmd_json_reader_next_json_str(reader);

    if (ret != 0) {
        return ret;
    }

    ret = gmd_json_reader_parse_json(reader);
    if (ret != 0)
    {
        return ret;
    }

    for (i = 0; i < reader->tokens_cnt; ++i)
    {
        if (reader->tokens[i].type == JSMN_STRING)
        {
            /* check for artist id */
            if (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_ARTIST_ID))
            {
                ++i;
                ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], artist_id);
                if (ret != 0)
                    break;
                art_id_set = 1;
            }
            // artistName
            else if (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_ARTIST_NAME) && !art_name_set)
            {
                ++i;
                if (reader->tokens[i].type != JSMN_STRING)
                {
                    ++i;
                    while(0 != gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_ARTIST_NAME))
                        ++i;
                    ++i;
                }

                ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], artist_name);
                if (ret != 0)
                    break;
                art_name_set = 1;
            }
            /* check for descriptors key */
            else if (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_DESCRIPTORS))
            {
                ++i;
                ret = gmd_json_reader_descriptors_token_to_array(reader, &i, max_descriptors, descriptor_array, descriptor_array_cnt_out);
                descriptors_set = 1;
                break;
            }
        }
    }

    if (art_id_set && art_name_set && descriptors_set)
        return 0;

    return 1;
}

/* need this to be big or else super slow */
#define CORRELATES_SIZE_INIT 32768
#define CORRELATES_SIZE_GROW 32768
int gmd_json_reader_get_correlates(gmd_json_reader_t *reader, int type, gmd_json_correlate_t **correlate_array_out, int *correlate_array_cnt_out, int *correlate_array_size_out)
{
    int ret = 0;
    int result = 0;
    int i = 0;
    int primary_id_set = 0;
    int correlate_count = 0;
    int correlate_size = *correlate_array_size_out;
    char primary_id[GMD_JSON_MAX_KEY];
    gmd_json_correlate_t *correlate_array = *correlate_array_out;
    gmd_json_reader_t *reader_inner = NULL;

    *correlate_array_cnt_out = 0;

    if (correlate_size == 0)
    {
        correlate_array = (gmd_json_correlate_t *) malloc(sizeof(gmd_json_correlate_t) * CORRELATES_SIZE_INIT);
        if (correlate_array == NULL)
            return ENOMEM;
        correlate_size = CORRELATES_SIZE_INIT;
    }

    ret = gmd_json_reader_next_json_str(reader);
    if (ret != 0) {
        return ret;
    }

    ret = gmd_json_reader_init_w_input_buffer(&reader_inner, NULL, GMD_TYPE_OBJECT, NULL, 2, reader->max_string_size, 1, reader->output_str, reader->logger);
    if (ret != 0) {
        return ret;
    }

    while (result != GMD_JSON_NO_MORE_DATA && ret == 0)
    {
        primary_id_set = 0;
        i = 0;
        result = gmd_json_reader_next_json_str(reader_inner);
        if (result != 0)
            break;

        ret = gmd_json_reader_parse_json(reader_inner);
        if (ret != 0)
            break;

        while (i < reader_inner->tokens_cnt)
        {
            if (reader_inner->tokens[i].type == JSMN_STRING && primary_id_set == 0)
            {
                ret = gmd_json_reader_token_string_copy(reader_inner, &reader_inner->tokens[i], primary_id);
                if (ret != 0)
                    break;

                primary_id_set = 1;
            }
            else if (reader_inner->tokens[i].type == JSMN_STRING && primary_id_set)
            {
                /* check for reallocation */
                if (correlate_count == correlate_size)
                {
                    correlate_array = (gmd_json_correlate_t *) realloc(correlate_array, sizeof(gmd_json_correlate_t) * (correlate_size + CORRELATES_SIZE_GROW));
                    if (correlate_array == NULL)
                    {
                        return ENOMEM;
                    }
                    correlate_size += CORRELATES_SIZE_GROW;
                }

                correlate_array[correlate_count].type = type;
                strncpy(correlate_array[correlate_count].id1, primary_id, GMD_JSON_SIZE_CORRELATE_ID);
                ret = gmd_json_reader_token_string_copy_size(reader_inner, &reader_inner->tokens[i], correlate_array[correlate_count].id2, GMD_JSON_SIZE_CORRELATE_ID - 1);
                if (ret != 0)
                    break;
                i++;
                ret = gmd_json_reader_token_string_copy_size(reader_inner, &reader_inner->tokens[i], correlate_array[correlate_count].score, GMD_JSON_SIZE_CORRELATE_SCORE - 1);
                if (ret != 0)
                    break;
                correlate_count++;

            }
            i++;
        }
    }

    *correlate_array_cnt_out = correlate_count;
    *correlate_array_size_out= correlate_size;
    *correlate_array_out = correlate_array;

    /* uninitialize reader_inner */
    ret = gmd_json_reader_uninit(reader_inner);

    return ret;
}

int gmd_json_reader_get_song_preferred_recording(gmd_json_reader_t *reader, char *songID, char *recordingID)
{
    int ret = 0;
    int song_id_set = 0;
    int recording_id_set = 0;
    int i = 0;

    ret = gmd_json_reader_next_json_str(reader);

    if (ret != 0) {
        return ret;
    }

    ret = gmd_json_reader_parse_json(reader);
    if (ret != 0)
    {
        return ret;
    }

    while (i < reader->tokens_cnt)
    {
        if (reader->tokens[i].type == JSMN_STRING &&
            (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_SONG_ID)))
        {
            i++;
            ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], songID);
            if (ret != 0)
                break;
            song_id_set = 1;
        }
        else if (reader->tokens[i].type == JSMN_STRING &&
            (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_PREFERRED_RECORDING_ID)))
        {
            i++;
            ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], recordingID);
            if (ret != 0)
                break;
            recording_id_set = 1;
        }
        i++;
    }

    if (song_id_set && recording_id_set) {
        return 0;
    }

    return 1;
}

int gmd_json_reader_get_recording_info(gmd_json_reader_t *reader, char *recording_id, char *recording_name, char *artist_id, char *artist_name, char *release_year, char *song_id, int max_descriptors, gmd_json_descriptor_t *descriptor_array, int *descriptor_array_cnt_out)
{
    int ret = 0;
    int recording_id_set = 0;
    int recording_name_set = 0;
    int artist_id_set = 0;
    int artist_name_set = 0;
    int song_id_set = 0;
    int descriptors_set = 0;
    int i = 0;

    *descriptor_array_cnt_out = 0;
    ret = gmd_json_reader_next_json_str(reader);

    if (ret != 0) {
        return ret;
    }

    ret = gmd_json_reader_parse_json(reader);
    if (ret != 0)
    {
        return ret;
    }

    while (i < reader->tokens_cnt)
    {
        if (reader->tokens[i].type == JSMN_STRING &&
            (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_RECORDING_NAME)))
        {
            i++;
            ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], recording_name);
            if (ret != 0)
                break;
            recording_name_set = 1;
        }
        else if (reader->tokens[i].type == JSMN_STRING &&
            (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_RECORDING_ID)))
        {
            i++;
            ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], recording_id);
            if (ret != 0)
                break;
            recording_id_set = 1;
        }
        else if (reader->tokens[i].type == JSMN_STRING &&
            (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_ARTIST_ID)))
        {
            i++;
            ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], artist_id);
            if (ret != 0)
                break;
            artist_id_set = 1;
        }
        else if (reader->tokens[i].type == JSMN_STRING &&
            (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_ARTIST_NAME)))
        {
            i++;
            ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], artist_name);
            if (ret != 0)
                break;
            artist_name_set = 1;
        }
        else if (reader->tokens[i].type == JSMN_STRING &&
            (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_RELEASE_YEAR)))
        {
            i++;
            ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], release_year);
            if (ret != 0)
                break;
        }
        else if (reader->tokens[i].type == JSMN_STRING &&
            (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_SONG_ID)))
        {
            i++;
            ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], song_id);
            if (ret != 0)
                break;
            song_id_set = 1;
        }
        else if (reader->tokens[i].type == JSMN_STRING &&
            0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_DESCRIPTORS))
        {
            ++i;
            ret = gmd_json_reader_descriptors_token_to_array(reader, &i, max_descriptors, descriptor_array, descriptor_array_cnt_out);
            descriptors_set = 1;
            break;
        }
        i++;
    }

    if (recording_name_set && recording_id_set && artist_id_set && artist_name_set && song_id_set && descriptors_set) {
        return 0;
    }

    return 1;
}

int gmd_json_reader_get_master_data(gmd_json_reader_t *reader, char *master_id, char *master_name, char *release_year)
{
    int ret = 0;
    int master_name_set = 0;
    int master_id_set = 0;
    int i = 0;

    ret = gmd_json_reader_next_json_str(reader);

    if (ret != 0) {
        return ret;
    }

    ret = gmd_json_reader_parse_json(reader);
    if (ret != 0)
    {
        return ret;
    }

    while (i < reader->tokens_cnt)
    {
        if (reader->tokens[i].type == JSMN_STRING &&
            (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_ALBUM_MASTER_NAME)))
        {
            i++;
            ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], master_name);
            if (ret != 0)
                break;
            master_name_set = 1;
        }
        else if (reader->tokens[i].type == JSMN_STRING &&
            (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_ALBUM_MASTER_ID)))
        {
            i++;
            ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], master_id);
            if (ret != 0)
                break;
            master_id_set = 1;
        }
        else if (reader->tokens[i].type == JSMN_STRING &&
            (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_RELEASE_YEAR)))
        {
            i++;
            ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], release_year);
            if (ret != 0)
                break;
        }

        i++;
    }

    if (master_name_set && master_id_set) {
        return 0;
    }

    return 1;
}

int gmd_json_reader_get_master_preferred_edition(gmd_json_reader_t *reader, char *preferred_edition_id, char *master_name, char *master_id, char *release_year)
{
    int ret = 0;
    int preferred_edition_id_set = 0;
    int master_name_set = 0;
    int master_id_set = 0;
    int i = 0;

    ret = gmd_json_reader_next_json_str(reader);

    if (ret != 0) {
        return ret;
    }

    ret = gmd_json_reader_parse_json(reader);
    if (ret != 0)
    {
        return ret;
    }

    while (i < reader->tokens_cnt)
    {
        if (reader->tokens[i].type == JSMN_STRING &&
            (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_PREFERRED_EDITION_ID)))
        {
            i++;
            ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], preferred_edition_id);
            if (ret != 0)
                break;
            preferred_edition_id_set = 1;
        }
        else if (reader->tokens[i].type == JSMN_STRING &&
            (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_ALBUM_MASTER_NAME)))
        {
            i++;
            ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], master_name);
            if (ret != 0)
                break;
            master_name_set = 1;
        }
        else if (reader->tokens[i].type == JSMN_STRING &&
            (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_ALBUM_MASTER_ID)))
        {
            i++;
            ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], master_id);
            if (ret != 0)
                break;
            master_id_set = 1;
        }
        else if (reader->tokens[i].type == JSMN_STRING &&
            (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_RELEASE_YEAR)))
        {
            i++;
            ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], release_year);
            if (ret != 0)
                break;
        }

        i++;
    }

    if (preferred_edition_id_set && master_name_set && master_id_set) {
        return 0;
    }

    return 1;

}

int gmd_json_reader_get_edition_recordings(gmd_json_reader_t *reader, char *edition_id, int max_recordings, char **recording_ids, int *cnt_recording_ids_out, int *release_type_out)
{
    int ret = 0;
    int i = 0;
    int edition_id_set = 0;
    int recording_count = 0;
    int release_type = -1;
    char release_type_str[GMD_MAX_LINE] = {0};

    *cnt_recording_ids_out = 0;

    ret = gmd_json_reader_next_json_str(reader);

    if (ret != 0) {
        return ret;
    }

    ret = gmd_json_reader_parse_json(reader);
    if (ret != 0)
    {
        return ret;
    }

    i = 0;
    while (i < reader->tokens_cnt)
    {
        if (reader->tokens[i].type == JSMN_STRING &&
            (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_ALBUM_EDITION_ID)))
        {
            i++;
            ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], edition_id);
            if (ret != 0)
                break;
            edition_id_set = 1;
        }
        else if (reader->tokens[i].type == JSMN_STRING &&
            (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_RECORDING_ID)) &&
            recording_count < max_recordings)
        {
            i++;
            ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], recording_ids[recording_count]);
            if (ret != 0)
                break;
            recording_count++;
        }
        else if (reader->tokens[i].type == JSMN_STRING &&
                 (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_RELEASE_TYPE)))
        {
            int id_found = 0;
            while (!id_found)
            {
                i++;
                if (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_ID))
                {
                    id_found = 1;
                    i++;
                    ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], release_type_str);
                    if (ret != 0)
                        break;
                    release_type = atoi(release_type_str);
                }
            }
        }
        i++;
    }
    if (edition_id_set) {
        *cnt_recording_ids_out = recording_count;
        *release_type_out = release_type;
        return 0;
    }

    return 1;
}

int gmd_json_reader_get_edition_data(gmd_json_reader_t *reader, char *edition_id, char *master_id, char *edition_name)
{
    int ret = 0;
    int i = 0;
    int edition_id_set = 0;
    int master_id_set = 0;

    ret = gmd_json_reader_next_json_str(reader);

    if (ret != 0) {
        return ret;
    }

    ret = gmd_json_reader_parse_json(reader);
    if (ret != 0)
    {
        return ret;
    }

    i = 0;
    while (i < reader->tokens_cnt)
    {
        if (reader->tokens[i].type == JSMN_STRING &&
            (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_ALBUM_EDITION_ID)))
        {
            i++;
            ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], edition_id);
            if (ret != 0)
                break;
            edition_id_set = 1;
        }
        else if (reader->tokens[i].type == JSMN_STRING &&
                 (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_ALBUM_MASTER_ID)))
        {
            i++;
            ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], master_id);
            if (ret != 0)
                break;
            master_id_set = 1;
        }
        else if (reader->tokens[i].type == JSMN_STRING &&
                 (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_ALBUM_EDITION_NAME)))
        {
            i++;
            ret = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], edition_name);
            if (ret != 0)
                break;
        }

        i++;
    }
    if (edition_id_set & master_id_set) {
        return 0;
    }

    return 1;
}

static int gmd_json_reader_next_json_str_OBJECT(gmd_json_reader_t *reader)
{
    int             error = 0;
    char            line_buffer[GMD_MAX_LINE] = {0};
    char            *chunk = NULL;
    int             b_done = 0;
    int             size_data = 0;
    int             length = 0;

    reader->buffer_cnt = 0;

    /* short circuit, if already matched key, don't need to parse rest of data */
    if (reader->b_key_match_once && reader->key_filter != NULL)
        return GMD_JSON_NO_MORE_DATA;

    if (strlen(reader->leftover_line) > 0)
    {
        error = process_chunk_OBJECT(reader, reader->leftover_line, &b_done, strlen(reader->leftover_line));
        if (error)
            goto cleanup;
    }

    while(!b_done)
    {
        if (reader->use_input_buffer)
        {
            chunk = get_next_chunk(&reader->input_buffer, "}", &length);
            if (chunk == NULL)
                break;
        }
        else
        {
            if (NULL == fgets(line_buffer, GMD_MAX_LINE, reader->f))
                break;
            chunk = line_buffer;
            length = strlen(chunk);
        }

        error = process_chunk_OBJECT(reader, chunk, &b_done, length);
        if (error)
            goto cleanup;

        if (b_done)
        {
            break;
        }
    }

    if (b_done)
    {
        /* size is buffer + key + 2 quotation marks + colon + 2 braces + string terminator */
        size_data = reader->buffer_cnt + strlen(reader->key_match) + 6;
        if (size_data > reader->output_str_size)
        {
            reader->output_str = (char *) realloc(reader->output_str, sizeof(char) * size_data);
            if (reader->output_str == NULL)
            {
                reader->logger("Error reallocating output_str!");
                error = ENOMEM;
                goto cleanup;
            }
            reader->output_str_size = size_data;
        }

        sprintf(reader->output_str, "{\"%s\":%s}", reader->key_match, reader->buffer);
    }
    else
    {
        return GMD_JSON_NO_MORE_DATA;
    }

cleanup:

    return error;
}

static int process_chunk_OBJECT(gmd_json_reader_t *reader, char *chunk, int *b_done_out, int length)
{
    int             error = 0;
    int             i = 0;
    int             b_done = 0;

    for (i = 0; i < length; ++i)
    {
        /* skip special chars inside quotes */
        if (chunk[i] == '"' && !reader->b_ignore_quote)
        {
            reader->b_ignore_special_chars = reader->b_ignore_special_chars == 0 ? 1 : 0;
        }

        /* handle strings with escaped double quotes */
        if (chunk[i] == '\\' && !reader->b_ignore_quote)
        {
            reader->b_ignore_quote = 1;
        }
        else
        {
            reader->b_ignore_quote = 0;
        }

        if (!reader->b_ignore_special_chars)
        {
            if (chunk[i] == '{')
            {
                reader->level_current += 1;

                if (reader->b_key_match && reader->level_current == reader->level_key + 1)
                {
                    reader->b_copy_on = 1;
                }
            }
            else if (chunk[i] == '}')
            {
                reader->level_current -= 1;

                if (reader->level_current == reader->level_key && reader->b_copy_on)
                {
                    b_done = 1;
                }
            }
            else if (chunk[i] == ':' && reader->level_current == reader->level_key)
            {
                /* check for key match ... also handles case if key is NULL (match all keys) */
                check_for_key_match(reader, chunk, i);
            }
        }

        if (reader->b_copy_on)
        {
            error = add_char_to_buffer(reader, chunk[i]);
            if (error)
                break;
        }

        if (b_done)
        {
            error = add_char_to_buffer(reader, '\0');
            if (error)
                break;

            *b_done_out = 1;
            if (!reader->use_input_buffer)
                strcpy(reader->leftover_line, chunk+i+1);
            reader->b_copy_on = 0;
            reader->b_key_match = 0;
            break;
        }
    }

    return error;
}

static int gmd_json_reader_next_json_str_ARRAY(gmd_json_reader_t *reader)
{
    int             error = 0;
    char            line_buffer[GMD_MAX_LINE] = {0};
    char            *chunk = NULL;
    int             b_done = 0;
    int             size_data = 0;
    int             length = 0;

    reader->buffer_cnt = 0;

    /* short circuit, if already matched key, don't need to parse rest of data */
    if (reader->b_key_match_once && reader->key_filter != NULL && reader->datatype == GMD_TYPE_OBJECT)
        return GMD_JSON_NO_MORE_DATA;

    if (strlen(reader->leftover_line) > 0)
    {
        error = process_chunk_ARRAY(reader, reader->leftover_line, &b_done, strlen(reader->leftover_line));
        if (error)
            goto cleanup;
    }

    while(!b_done)
    {
        if (reader->use_input_buffer)
        {
            chunk = get_next_chunk(&reader->input_buffer, "}", &length);
            if (chunk == NULL)
                break;
        }
        else
        {
            if (NULL == fgets(line_buffer, GMD_MAX_LINE, reader->f))
                break;
            chunk = line_buffer;
            length = strlen(chunk);
        }

        error = process_chunk_ARRAY(reader, chunk, &b_done, length);
        if (error)
            goto cleanup;

        if (b_done)
        {
            break;
        }
    }

    if (b_done)
    {
        /* size is buffer + 1 string terminator */
        size_data = reader->buffer_cnt + 1;
        if (size_data > reader->output_str_size)
        {
            reader->output_str = (char *) realloc(reader->output_str, sizeof(char) * size_data);
            if (reader->output_str == NULL)
            {
                reader->logger("Error reallocating output_str!");
                error = ENOMEM;
                goto cleanup;
            }
            reader->output_str_size = size_data;
        }

        strcpy(reader->output_str, reader->buffer);
    }
    else
    {
        return GMD_JSON_NO_MORE_DATA;
    }

cleanup:

    return error;
}

static int process_chunk_ARRAY(gmd_json_reader_t *reader, char *chunk, int *b_done_out, int length)
{
    int             error = 0;
    int             i = 0;
    int             b_done = 0;

    for (i = 0; i < length; ++i)
    {
        /* skip special chars inside quotes */
        if (chunk[i] == '"' && !reader->b_ignore_quote)
        {
            reader->b_ignore_special_chars = reader->b_ignore_special_chars == 0 ? 1 : 0;
        }

        /* handle strings with escaped double quotes */
        if (chunk[i] == '\\' && !reader->b_ignore_quote)
        {
            reader->b_ignore_quote = 1;
        }
        else
        {
            reader->b_ignore_quote = 0;
        }

        if (!reader->b_ignore_special_chars)
        {
            if (chunk[i] == '{')
            {
                reader->level_current += 1;

                if (reader->b_key_match && reader->level_current == reader->level_key + 1)
                {
                    reader->b_copy_on = 1;
                }
            }
            else if (chunk[i] == '}')
            {
                reader->level_current -= 1;

                if (reader->level_current == reader->level_key && reader->b_copy_on)
                {
                    b_done = 1;
                }
            }
            else if (chunk[i] == ']' && reader->level_current == reader->level_key && reader->b_key_match)
            {
                reader->b_key_match = 0;
            }
            else if (chunk[i] == ':' && reader->level_current == reader->level_key)
            {
                /* check for key match ... also handles case if key is NULL (match all keys) */
                check_for_key_match(reader, chunk, i);
            }
        }

        if (reader->b_copy_on)
        {
            error = add_char_to_buffer(reader, chunk[i]);
            if (error)
                break;
        }

        if (b_done)
        {
            error = add_char_to_buffer(reader, '\0');
            if (error)
                break;

            *b_done_out = 1;
            if (!reader->use_input_buffer)
                strcpy(reader->leftover_line, chunk+i+1);
            reader->b_copy_on = 0;
            break;
        }
    }

    return error;
}

static int add_char_to_buffer(gmd_json_reader_t *reader, char c)
{
    if (reader->buffer_cnt == reader->buffer_size)
    {
        reader->buffer = (char *) realloc(reader->buffer, reader->buffer_size + sizeof(char) * GMD_JSON_BUFFER_SIZE_GROW);
        if (reader->buffer == NULL)
        {
            reader->logger("Error reallocating buffer!");
            return ENOMEM;
        }
        reader->buffer_size += GMD_JSON_BUFFER_SIZE_GROW;
    }

    reader->buffer[reader->buffer_cnt] = c;
    reader->buffer_cnt += 1;

    return 0;
}

/* if key match, then switch copy flag on */
/* also set key if match */
static void check_for_key_match(gmd_json_reader_t *reader, char *chunk, int index)
{
    int index_quote_1 = -1;
    int index_quote_2 = -1;
    int key_length = 0;
    char test_key[GMD_JSON_MAX_KEY] = {0};

    /* walk back buffer until 2 double quotes */
    while (index >= 0)
    {
        if (chunk[index] == '"')
        {
            if (index_quote_2 == -1)
            {
                index_quote_2 = index;
            }
            else
            {
                index_quote_1 = index;
                break;
            }
        }
        index--;
    }

    if (index_quote_1 == -1 || index_quote_2 == -1)
        return;

    /* string length is index2 - index1 - 1*/
    key_length = index_quote_2 - index_quote_1 - 1;
    if (key_length < 1 || key_length >= GMD_JSON_MAX_KEY)
        return;

    strncpy(test_key, chunk+index_quote_1+1, key_length);

    /* check for match */
    /* always match if key_filter is NULL */
    if (reader->key_filter == NULL || strcmp(reader->key_filter, test_key) == 0)
    {
        reader->b_key_match = 1;
        reader->b_key_match_once = 1;
        strcpy(reader->key_match, test_key);
    }
}

static int gmd_json_reader_parse_json(gmd_json_reader_t *reader)
{
    int error = 0;
    int tokens_cnt = 0;

    jsmn_init(&(reader->p));

    /* passing NULL instead of tokens will return the number of tokens */
    tokens_cnt = jsmn_parse(&(reader->p), reader->output_str, strlen(reader->output_str), NULL, 0);

    if (tokens_cnt > reader->tokens_size)
    {
        reader->tokens = (jsmntok_t *) realloc(reader->tokens, sizeof(jsmntok_t) * tokens_cnt);
        if (reader->tokens == NULL)
        {
            return ENOMEM;
        }
        reader->tokens_size = tokens_cnt;
    }

    jsmn_init(&(reader->p));
    tokens_cnt = jsmn_parse(&(reader->p), reader->output_str, strlen(reader->output_str), reader->tokens, tokens_cnt);

    if (tokens_cnt < 0)
    {
        reader->logger("Error returned from jsmn: %d", tokens_cnt);
        error = tokens_cnt;
    }
    else
    {
        reader->tokens_cnt = tokens_cnt;
    }

    return error;
}

static int gmd_json_reader_token_string_eq(gmd_json_reader_t *reader, jsmntok_t *tok, const char *str)
{
    int str_len = strlen(str);
    int tok_len = tok->end - tok->start;

    if (str_len != tok_len)
        return 1;

    return strncmp(str, reader->output_str + tok->start, str_len);
}

static int gmd_json_reader_token_string_copy(gmd_json_reader_t *reader, jsmntok_t *tok, char *str)
{
    return gmd_json_reader_token_string_copy_size(reader, tok, str, reader->max_string_size - 1);
}
static int gmd_json_reader_token_string_copy_size(gmd_json_reader_t *reader, jsmntok_t *tok, char *str, int max_length)
{
    int tok_len = tok->end - tok->start;
    int str_len = tok_len > max_length ? max_length : tok_len;

    if (tok_len < 0)
        return 1;
    else if (tok_len == 0)
        str[0] = '\0';
    else
    {
        /*snprintf is way faster than strncpy on windows cygwin compiler*/
        snprintf(str, str_len+1, "%s", reader->output_str + tok->start);
        /*strncpy(str, reader->output_str + tok->start, str_len);*/
        str[str_len] = '\0';
    }

    return 0;
}

static int gmd_json_reader_descriptors_token_to_array(gmd_json_reader_t *reader, int *current_token_index, int max_descriptors, gmd_json_descriptor_t *descriptors, int *cnt_descriptors_out)
{
    int error = 0;
    int i = *current_token_index;
    int current_descriptor_type = GMD_DESCRIPTOR_TYPE_GENRE;
    int cnt_descriptors = 0;
    /* size is number of elements in descriptors object */
    int cnt_descriptor_types_left = reader->tokens[i].size;
    /* 2 for id and weight */
    int num_fields_in_descriptor_object = 2;
    int cnt_fields_left = num_fields_in_descriptor_object;
    int cnt_descriptors_left = 0;

    *cnt_descriptors_out = 0;

    for (i = i + 1; i < reader->tokens_cnt; ++i)
    {
        if (cnt_descriptor_types_left == 0)
            break;

        if (reader->tokens[i].type == JSMN_STRING)
        {
            /* set current descriptor type */
            if (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_GENRES))
            {
                current_descriptor_type = GMD_DESCRIPTOR_TYPE_GENRE;
                descriptors[cnt_descriptors].type = current_descriptor_type;

                i++;
                cnt_descriptors_left = reader->tokens[i].size;
            }
            else if (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_ORIGINS))
            {
                current_descriptor_type = GMD_DESCRIPTOR_TYPE_ORIGIN;
                descriptors[cnt_descriptors].type = current_descriptor_type;

                i++;
                cnt_descriptors_left = reader->tokens[i].size;
            }
            else if (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_ERAS))
            {
                current_descriptor_type = GMD_DESCRIPTOR_TYPE_ERA;
                descriptors[cnt_descriptors].type = current_descriptor_type;

                i++;
                cnt_descriptors_left = reader->tokens[i].size;
            }
            else if (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_ATYPES))
            {
                current_descriptor_type = GMD_DESCRIPTOR_TYPE_ATYPE;
                descriptors[cnt_descriptors].type = current_descriptor_type;

                i++;
                cnt_descriptors_left = reader->tokens[i].size;
            }
            else if (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_LANGUAGES))
            {
                current_descriptor_type = GMD_DESCRIPTOR_TYPE_LANG;
                descriptors[cnt_descriptors].type = current_descriptor_type;

                i++;
                cnt_descriptors_left = reader->tokens[i].size;
            }
            else if (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_MOODS))
            {
                current_descriptor_type = GMD_DESCRIPTOR_TYPE_MOOD;
                descriptors[cnt_descriptors].type = current_descriptor_type;

                i++;
                cnt_descriptors_left = reader->tokens[i].size;
            }
            else if (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_TEMPOS))
            {
                current_descriptor_type = GMD_DESCRIPTOR_TYPE_TEMPO;
                descriptors[cnt_descriptors].type = current_descriptor_type;

                i++;
                cnt_descriptors_left = reader->tokens[i].size;
            }
            else if (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_ID))
            {
                i++;
                error = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], descriptors[cnt_descriptors].id);
                if (error != 0)
                    break;
                cnt_fields_left--;
                if (cnt_fields_left == 0)
                {
                    cnt_descriptors++;
                    if (cnt_descriptors == max_descriptors)
                        break;

                    cnt_descriptors_left--;
                    if (cnt_descriptors_left == 0)
                        cnt_descriptor_types_left--;

                    cnt_fields_left = num_fields_in_descriptor_object;
                    descriptors[cnt_descriptors].type = current_descriptor_type;
                }
            }
            else if (0 == gmd_json_reader_token_string_eq(reader, &reader->tokens[i], GMD_KEY_WEIGHT))
            {
                i++;
                error = gmd_json_reader_token_string_copy(reader, &reader->tokens[i], descriptors[cnt_descriptors].weight);
                if (error != 0)
                    break;
                cnt_fields_left--;
                if (cnt_fields_left == 0)
                {
                    cnt_descriptors++;
                    if (cnt_descriptors == max_descriptors)
                        break;

                    cnt_descriptors_left--;
                    if (cnt_descriptors_left == 0)
                        cnt_descriptor_types_left--;

                    cnt_fields_left = num_fields_in_descriptor_object;
                    descriptors[cnt_descriptors].type = current_descriptor_type;
                }
            }
        }
    }

    if (error == 0)
    {
        *current_token_index = i;
        *cnt_descriptors_out = cnt_descriptors;
    }

    return error;
}

/* use this to chunk up string buffer */
static char *get_next_chunk(char **text, const char *delim,  int *chunk_length)
{
    char    *start = *text;
    char    *p;

    p = (start != NULL) ? strpbrk(start, delim) : NULL;

    if (p == NULL) {
        *text = NULL;
    } else {
        *chunk_length = p - start + 1;
        *text = p + 1;
    }

    return start;
}

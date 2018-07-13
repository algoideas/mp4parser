#include <stdio.h>

#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#define STREAM_BUFFER_SIZE  (1 * 1024 * 1024)

/**
 * stream_t definition
 */

typedef struct stream_s
{
    /* Real file path (it can only be changed during stream_t opening) */
    char *psz_path;

    /* Stream source for stream filter */
    FILE *p_source;

    /* Stream buffer */
    uint8_t *p_buffer;
}stream_t;


int32_t stream_Open( stream_t *s);

int32_t stream_Close( stream_t *s);


/**
 * Try to read "i_read" bytes into a buffer pointed by "p_read".  If
 * "p_read" is NULL then data are skipped instead of read.
 * \return The real number of bytes read/skip. If this value is less
 * than i_read that means that it's the end of the stream.
 * \note stream_Read increments the stream position, and when p_read is NULL,
 * this is its only task.
 */
int64_t stream_Read( stream_t *s, void *p_read, int i_read );

/**
 * Store in pp_peek a pointer to the next "i_peek" bytes in the stream
 * \return The real number of valid bytes. If it's less
 * or equal to 0, *pp_peek is invalid.
 * \note pp_peek is a pointer to internal buffer and it will be invalid as
 * soons as other stream_* functions are called.
 * \note Contrary to stream_Read, stream_Peek doesn't modify the stream
 * position, and doesn't necessarily involve copying of data. It's mainly
 * used by the modules to quickly probe the (head of the) stream.
 * \note Due to input limitation, the return value could be less than i_peek
 * without meaning the end of the stream (but only when you have i_peek >=
 * p_input->i_bufsize)
 */
int64_t stream_Peek( stream_t *s, const uint8_t **pp_peek, int i_peek );

int64_t stream_Tell( stream_t *s );

int32_t stream_Seek( stream_t *s, uint64_t i_pos );

/**
 * Get the size of the stream.
 */
int64_t stream_Size( stream_t *s );

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

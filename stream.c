#include "stream.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


int32_t stream_Open( stream_t *s)
{
    int ret = -1;
       
    if ((NULL == s) || (NULL == s->psz_path))
    {
        printf("s or s->psz_path is NULL!\n");
        return -1;
    }
    
    s->p_source = fopen(s->psz_path, "rb");
    if (NULL == s->p_source)
    {
        printf("fopen %s fail!\n", s->psz_path);
        return -1;
    }

    s->p_buffer = (uint8_t *)malloc(sizeof(uint8_t *) * STREAM_BUFFER_SIZE);  
    if (!s->p_buffer) 
    {
        fclose(s->p_source);
        printf("call calloc fail!\n");
        return -1; 
    }

    return 0;
}

int32_t stream_Close( stream_t *s)
{
    if ((NULL == s) || (NULL == s->p_source))
    {
        printf("p_stream or p_stream->p_source is NULL!\n");
        return -1;
    }
    
    if (s->p_buffer)
    {
        free(s->p_buffer);
        s->p_buffer = NULL;
    }
    
    fclose(s->p_source);

    return 0;
}


/**
 * Try to read "i_read" bytes into a buffer pointed by "p_read".  If
 * "p_read" is NULL then data are skipped instead of read.
 * \return The real number of bytes read/skip. If this value is less
 * than i_read that means that it's the end of the stream.
 * \note stream_Read increments the stream position, and when p_read is NULL,
 * this is its only task.
 */
int64_t stream_Read( stream_t *s, void *p_read, int i_read )
{
    int64_t i_pos = 0;
    
    if ((NULL == s) || (NULL == s->p_source))
    {
        printf("p_stream or p_stream->p_source is NULL!\n");
        return -1;
    }

    i_pos = fread(p_read, 1, i_read, s->p_source);
    
    return i_pos;
}

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
int64_t stream_Peek( stream_t *s, const uint8_t **pp_peek, int i_peek )
{
    int64_t i_pos = 0;
    int32_t i_len = 0;
    
    if ((NULL == s) || (NULL == s->p_source))
    {
        printf("s or s->p_source is NULL!\n");
        return -1;
    }

    memset(s->p_buffer, 0x0, STREAM_BUFFER_SIZE);
    i_len = fread(s->p_buffer, 1, i_peek, s->p_source);
    i_pos = ftell(s->p_source);
    fseek(s->p_source, i_pos - i_len, 0);

    *pp_peek = s->p_buffer;

    return i_len;
}

int64_t stream_Tell( stream_t *s )
{
    if ((NULL == s) || (NULL == s->p_source))
    {
        printf("p_stream or p_stream->p_source is NULL!\n");
        return -1;
    }
    
    int64_t i_pos = ftell(s->p_source);    
    return i_pos;
}

int32_t stream_Seek( stream_t *s, uint64_t i_pos )
{
    if ((NULL == s) || (NULL == s->p_source))
    {
        printf("p_stream or p_stream->p_source is NULL!\n");
        return -1;
    }

    fseek(s->p_source, i_pos, 0);

    return 0;
}

/**
 * Get the size of the stream.
 */
int64_t stream_Size( stream_t *s )
{
    if ((NULL == s) || (NULL == s->p_source))
    {
        printf("p_stream or p_stream->p_source is NULL!\n");
        return -1;
    }

    uint64_t i_pos = 0;
    fseek(s->p_source, 0, SEEK_END);
    i_pos = ftell(s->p_source);
    fseek(s->p_source, 0, SEEK_SET);
    
    return i_pos;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

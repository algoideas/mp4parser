// MP4Parser.c
#include "libmp4.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

typedef int64_t mtime_t;

/* Contain all information about a chunk */
typedef struct
{
    uint64_t     i_offset; /* absolute position of this chunk in the file */
    uint32_t     i_sample_description_index; /* index for SampleEntry to use */
    uint32_t     i_sample_count; /* how many samples in this chunk */
    uint32_t     i_sample_first; /* index of the first sample in this chunk */
    uint32_t     i_sample; /* index of the next sample to read in this chunk */

    /* now provide way to calculate pts, dts, and offset without too
        much memory and with fast access */

    /* with this we can calculate dts/pts without waste memory */
    uint64_t     i_first_dts;   /* DTS of the first sample */
    uint64_t     i_last_dts;    /* DTS of the last sample */

    uint32_t     i_entries_dts;
    uint32_t     *p_sample_count_dts;
    uint32_t     *p_sample_delta_dts;   /* dts delta */

    uint32_t     i_entries_pts;
    uint32_t     *p_sample_count_pts;
    int32_t      *p_sample_offset_pts;  /* pts-dts */

    uint8_t      **p_sample_data;     /* set when b_fragmented is true */
    uint32_t     *p_sample_size;
    /* TODO if needed add pts
        but quickly *add* support for edts and seeking */

} mp4_chunk_t;

 /* Contain all needed information for read all track with vlc */
typedef struct
{
    unsigned int i_track_ID;/* this should be unique */

    int b_ok;               /* The track is usable */
    int b_enable;           /* is the trak enable by default */
    bool b_selected;  /* is the trak being played */
    bool b_chapter;   /* True when used for chapter only */

    bool b_mac_encoding;

    //es_format_t fmt;
    //es_out_id_t *p_es;

    /* display size only ! */
    int i_width;
    int i_height;
    float f_rotation;

    /* more internal data */
    uint32_t        i_timescale;    /* time scale for this track only */
    uint16_t        current_qid;    /* Smooth Streaming quality level ID */

    /* elst */
    int             i_elst;         /* current elst */
    int64_t         i_elst_time;    /* current elst start time (in movie time scale)*/
    MP4_Box_t       *p_elst;        /* elst (could be NULL) */

    /* give the next sample to read, i_chunk is to find quickly where
      the sample is located */
    uint32_t         i_sample;       /* next sample to read */
    uint32_t         i_chunk;        /* chunk where next sample is stored */
    /* total count of chunk and sample */
    uint32_t         i_chunk_count;
    uint32_t         i_sample_count;

    mp4_chunk_t    *chunk; /* always defined  for each chunk */
    mp4_chunk_t    *cchunk; /* current chunk if b_fragmented is true */

    /* sample size, p_sample_size defined only if i_sample_size == 0
        else i_sample_size is size for all sample */
    uint32_t         i_sample_size;
    uint32_t         *p_sample_size; /* XXX perhaps add file offset if take
                                    too much time to do sumations each time*/

    uint32_t     i_sample_first; /* i_sample_first value
                                                   of the next chunk */
    uint64_t     i_first_dts;    /* i_first_dts value
                                                   of the next chunk */

    MP4_Box_t *p_stbl;  /* will contain all timing information */
    MP4_Box_t *p_stsd;  /* will contain all data to initialize decoder */
    MP4_Box_t *p_sample;/* point on actual sdsd */

    bool b_drms;
    bool b_has_non_empty_cchunk;
    bool b_codec_need_restart;
    void      *p_drms;
    MP4_Box_t *p_skcr;

    mtime_t i_time;

    struct
    {
        /* for moof parsing */
        MP4_Box_t *p_traf;
        MP4_Box_t *p_tfhd;
        MP4_Box_t *p_trun;
        uint64_t   i_traf_base_offset;
    } context;

} mp4_track_t;


typedef struct mp4_fragment_t mp4_fragment_t;
struct mp4_fragment_t
{
    uint64_t i_chunk_range_min_offset;
    uint64_t i_chunk_range_max_offset;
    uint64_t i_duration;
    MP4_Box_t *p_moox;
    mp4_fragment_t *p_next;
};


typedef struct demux_sys_s
{
    MP4_Box_t    *p_root;      /* container for the whole file */

    mtime_t      i_pcr;

    uint64_t     i_overall_duration; /* Full duration, including all fragments */
    uint64_t     i_time;         /* time position of the presentation
                                  * in movie timescale */
    uint32_t     i_timescale;    /* movie time scale */
    uint64_t     i_duration;     /* movie duration */
    unsigned int i_tracks;       /* number of tracks */
    mp4_track_t  *track;         /* array of track */
    float        f_fps;          /* number of frame per seconds */

    bool         b_fragmented;   /* fMP4 */
    bool         b_seekable;
    bool         b_fastseekable;
    bool         b_seekmode;
    bool         b_smooth;       /* Is it Smooth Streaming? */

    bool            b_index_probed;
    bool            b_fragments_probed;
    mp4_fragment_t  moovfragment; /* moov */
    mp4_fragment_t *p_fragments;  /* known fragments (moof following moov) */

    struct
    {
        mp4_fragment_t *p_fragment;
        uint32_t        i_current_box_type;
        uint32_t        i_mdatbytesleft;
    } context;

    /* */
    MP4_Box_t    *p_tref_chap;

    
}demux_sys_t;

int main(int argc, char *argv[])
{
    const uint8_t   *p_peek;
    char file_name[512];
    memset(file_name, 0x0, sizeof(file_name));
    stream_t stream;
    stream.psz_path = file_name;

    demux_sys_t *p_sys;
    MP4_Box_t *p_ftyp;
    MP4_Box_t *p_moov;
    MP4_Box_t *p_mvhd;


    if (argc >= 2)
    {
        snprintf(file_name, 512, "%s", argv[1]);
    }
    else
    {
        printf("Please input file!\n");
        return -1;
    }

    /* create our structure that will contains all data */
    p_sys = calloc( 1, sizeof( demux_sys_t ) );
    if ( !p_sys )
        return -1;

    if (0 != MP4_BoxOpen(&stream))
    {
        printf("MP4_BoxOpen fail!\n");
        return -1;
    }

    /* Load all boxes ( except raw data ) */
    if( ( p_sys->p_root = MP4_BoxGetRoot( &stream ) ) == NULL )
    {
        printf("MP4_BoxGetRoot fail!\n");
    }

    //MP4_BoxDumpStructure(&stream, p_sys->p_root);

    if( ( p_ftyp = MP4_BoxGet( p_sys->p_root, "/ftyp" ) ) )
    {
        switch( BOXDATA(p_ftyp)->i_major_brand )
        {
            case( ATOM_isom ):
                printf(
                         "ISO Media file (isom) version %d.\n",
                         BOXDATA(p_ftyp)->i_minor_version );
                break;
            case( ATOM_3gp4 ):
            case( VLC_FOURCC( '3', 'g', 'p', '5' ) ):
            case( VLC_FOURCC( '3', 'g', 'p', '6' ) ):
            case( VLC_FOURCC( '3', 'g', 'p', '7' ) ):
                printf("3GPP Media file Release: %c\n",
#ifdef WORDS_BIGENDIAN
                        BOXDATA(p_ftyp)->i_major_brand
#else
                        BOXDATA(p_ftyp)->i_major_brand >> 24
#endif
                        );
                break;
            case( VLC_FOURCC( 'q', 't', ' ', ' ') ):
                printf( "Apple QuickTime file\n" );
                break;
            case( VLC_FOURCC( 'i', 's', 'm', 'l') ):
                printf( "PIFF (= isml = fMP4) file\n" );
                break;
            default:
                printf(
                         "unrecognized major file specification (%4.4s).\n",
                          (char*)&BOXDATA(p_ftyp)->i_major_brand );
                break;
        }
    }
    else
    {
        printf( "file type box missing (assuming ISO Media file)\n" );
    }

    if( !( p_sys->i_tracks = MP4_BoxCount( p_sys->p_root, "/moov/trak" ) ) )
    {
        printf("cannot find any /moov/trak" );
    }

    printf("found %d track%c\n",
                        p_sys->i_tracks,
                        p_sys->i_tracks ? 's':' ' );

    
    free(p_sys);

    MP4_BoxClose(&stream);
    
    return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

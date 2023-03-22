/*
 * media_player.c - libvlc smoke test
 *
 * $Id: eef9ece0516b6525107f9ba9dcf6314cf350e961 $
 */

/**********************************************************************
 *  Copyright (C) 2010 Pierre d'Herbemont.                            *
 *  This program is free software; you can redistribute and/or modify *
 *  it under the terms of the GNU General Public License as published *
 *  by the Free Software Foundation; version 2 of the license, or (at *
 *  your option) any later version.                                   *
 *                                                                    *
 *  This program is distributed in the hope that it will be useful,   *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              *
 *  See the GNU General Public License for more details.              *
 *                                                                    *
 *  You should have received a copy of the GNU General Public License *
 *  along with this program; if not, you can get it from:             *
 *  http://www.gnu.org/copyleft/gpl.html                              *
 **********************************************************************/

#include "test.h"
#include "../lib/libvlc_internal.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <vlc_threads.h>
#include <vlc_fs.h>
#include <vlc_input_item.h>
#include <vlc_events.h>

static void media_parse_ended(const libvlc_event_t *event, void *user_data)
{
    (void)event;
    vlc_sem_t *sem = user_data;
    vlc_sem_post (sem);
}

static void print_media(libvlc_media_t *media)
{
    libvlc_media_track_t **pp_tracks;
    unsigned i_count = libvlc_media_tracks_get(media, &pp_tracks);
    if (i_count > 0)
    {
        for (unsigned i = 0; i < i_count; ++i)
        {
            libvlc_media_track_t *p_track = pp_tracks[i];
            log("\ttrack(%d/%d): codec: %4.4s/%4.4s, ", i, p_track->i_id,
                (const char *)&p_track->i_codec,
                (const char *)&p_track->i_original_fourcc);
            switch (p_track->i_type)
            {
            case libvlc_track_audio:
                printf("audio: channels: %u, rate: %u\n",
                       p_track->audio->i_channels, p_track->audio->i_rate);
                break;
            case libvlc_track_video:
                printf("video: %ux%u, sar: %u/%u, fps: %u/%u\n",
                       p_track->video->i_width, p_track->video->i_height,
                       p_track->video->i_sar_num, p_track->video->i_sar_den,
                       p_track->video->i_frame_rate_num, p_track->video->i_frame_rate_den);
                break;
            case libvlc_track_text:
                printf("text: %s\n", p_track->subtitle->psz_encoding);
                break;
            case libvlc_track_unknown:
                printf("unknown\n");
                break;
            default:
                vlc_assert_unreachable();
            }
        }
        libvlc_media_tracks_release(pp_tracks, i_count);
    }
    else
        log("\tmedia doesn't have any tracks\n");

    for (enum libvlc_meta_t i = libvlc_meta_Title;
         i <= libvlc_meta_DiscTotal; ++i)
    {
        char *psz_meta = libvlc_media_get_meta(media, i);
        if (psz_meta != NULL)
            log("\tmeta(%d): '%s'\n", i, psz_meta);
        free(psz_meta);
    }
}

static void test_media_preparsed(libvlc_instance_t *vlc, const char *path,
                                 const char *location,
                                 libvlc_media_parse_flag_t parse_flags,
                                 libvlc_media_parsed_status_t i_expected_status)
{
    log ("test_media_preparsed: %s, expected: %d\n", path ? path : location,
         i_expected_status);

    libvlc_media_t *media;
    if (path != NULL)
        media = libvlc_media_new_path (vlc, path);
    else
        media = libvlc_media_new_location (vlc, location);
    assert (media != NULL);

    vlc_sem_t sem;
    vlc_sem_init (&sem, 0);

    // Check to see if we are properly receiving the event.
    libvlc_event_manager_t *em = libvlc_media_event_manager (media);
    libvlc_event_attach (em, libvlc_MediaParsedChanged, media_parse_ended, &sem);

    // Parse the media. This is synchronous.
    int i_ret = libvlc_media_parse_with_options(media, parse_flags, -1);
    assert(i_ret == 0);

    // Wait for preparsed event
    vlc_sem_wait (&sem);
    vlc_sem_destroy (&sem);

    // We are good, now check Elementary Stream info.
    assert (libvlc_media_get_parsed_status(media) == i_expected_status);
    if (i_expected_status == libvlc_media_parsed_status_done)
        print_media(media);

    libvlc_media_release (media);
}

static void input_item_preparse_timeout( const vlc_event_t *p_event,
                                         void *user_data )
{
    vlc_sem_t *p_sem = user_data;

    assert( p_event->u.input_item_preparse_ended.new_status == ITEM_PREPARSE_TIMEOUT );
    vlc_sem_post(p_sem);
}

static void test_input_metadata_timeout(libvlc_instance_t *vlc, int timeout,
                                        int wait_and_cancel)
{
    log ("test_input_metadata_timeout: timeout: %d, wait_and_cancel: %d\n",
         timeout, wait_and_cancel);

    int i_ret, p_pipe[2];
    i_ret = vlc_pipe(p_pipe);
    assert(i_ret == 0 && p_pipe[1] >= 0);

    char psz_fd_uri[strlen("fd://") + 11];
    sprintf(psz_fd_uri, "fd://%u", (unsigned) p_pipe[1]);
    input_item_t *p_item = input_item_NewFile(psz_fd_uri, "test timeout", 0,
                                              ITEM_LOCAL);
    assert(p_item != NULL);

    vlc_sem_t sem;
    vlc_sem_init (&sem, 0);
    i_ret = vlc_event_attach(&p_item->event_manager, vlc_InputItemPreparseEnded,
                             input_item_preparse_timeout, &sem);
    assert(i_ret == 0);
    i_ret = libvlc_MetadataRequest(vlc->p_libvlc_int, p_item,
                                   META_REQUEST_OPTION_SCOPE_LOCAL, timeout, vlc);
    assert(i_ret == 0);

    if (wait_and_cancel > 0)
    {
        msleep(wait_and_cancel * 1000);
        libvlc_MetadataCancel(vlc->p_libvlc_int, vlc);

    }
    vlc_sem_wait(&sem);

    input_item_Release(p_item);
    vlc_sem_destroy(&sem);
    vlc_close(p_pipe[0]);
    vlc_close(p_pipe[1]);
}

#define TEST_SUBITEMS_COUNT 6
static struct
{
    const char *file;
    libvlc_media_type_t type;
} test_media_subitems_list[TEST_SUBITEMS_COUNT] =
{
    { "directory", libvlc_media_type_directory, },
    { "file.jpg", libvlc_media_type_file },
    { "file.mkv", libvlc_media_type_file },
    { "file.mp3", libvlc_media_type_file },
    { "file.png", libvlc_media_type_file },
    { "file.ts", libvlc_media_type_file },
};

static void subitem_parse_ended(const libvlc_event_t *event, void *user_data)
{
    (void)event;
    vlc_sem_t *sem = user_data;
    vlc_sem_post (sem);
}

static void subitem_added(const libvlc_event_t *event, void *user_data)
{
#ifdef _WIN32
#define FILE_SEPARATOR   '\\'
#else
#define FILE_SEPARATOR   '/'
#endif
    bool *subitems_found = user_data;
    libvlc_media_t *m = event->u.media_subitem_added.new_child;
    assert (m);

    char *mrl = libvlc_media_get_mrl (m);
    assert (mrl);

    const char *file = strrchr (mrl, FILE_SEPARATOR);
    assert (file);
    file++;
    log ("subitem_added, file: %s\n", file);

    for (unsigned i = 0; i < TEST_SUBITEMS_COUNT; ++i)
    {
        if (strcmp (test_media_subitems_list[i].file, file) == 0)
        {
            assert (!subitems_found[i]);
            assert (libvlc_media_get_type(m) == test_media_subitems_list[i].type);
            subitems_found[i] = true;
        }
    }
    free (mrl);
#undef FILE_SEPARATOR
}

static void test_media_subitems_media(libvlc_media_t *media, bool play,
                                      bool b_items_expected)
{
    libvlc_media_add_option(media, ":ignore-filetypes= ");
    libvlc_media_add_option(media, ":no-sub-autodetect-file");

    bool subitems_found[TEST_SUBITEMS_COUNT] = { 0 };
    vlc_sem_t sem;
    vlc_sem_init (&sem, 0);

    libvlc_event_manager_t *em = libvlc_media_event_manager (media);
    libvlc_event_attach (em, libvlc_MediaSubItemAdded, subitem_added, subitems_found);

    if (play)
    {
        /* XXX: libvlc_media_parse_with_options won't work with fd, since it
         * won't be preparsed because fd:// is an unknown type, so play the
         * file to force parsing. */
        libvlc_event_attach (em, libvlc_MediaSubItemTreeAdded, subitem_parse_ended, &sem);

        libvlc_media_player_t *mp = libvlc_media_player_new_from_media (media);
        assert (mp);
        assert (libvlc_media_player_play (mp) != -1);
        vlc_sem_wait (&sem);
        libvlc_media_player_release (mp);
    }
    else
    {
        libvlc_event_attach (em, libvlc_MediaParsedChanged, subitem_parse_ended, &sem);

        int i_ret = libvlc_media_parse_with_options(media, libvlc_media_parse_local, -1);
        assert(i_ret == 0);
        vlc_sem_wait (&sem);
    }

    vlc_sem_destroy (&sem);

    if (!b_items_expected)
        return;

    for (unsigned i = 0; i < TEST_SUBITEMS_COUNT; ++i)
    {
        log ("test if %s was added\n", test_media_subitems_list[i].file);
        assert (subitems_found[i]);
    }
}

static void test_media_subitems(libvlc_instance_t *vlc)
{
    const char *subitems_path = SRCDIR"/samples/subitems";

    libvlc_media_t *media;

    log ("Testing media_subitems: path: '%s'\n", subitems_path);
    media = libvlc_media_new_path (vlc, subitems_path);
    assert (media != NULL);
    test_media_subitems_media (media, false, true);
    libvlc_media_release (media);

    #define NB_LOCATIONS 2
    char *subitems_realpath = realpath (subitems_path, NULL);
    assert (subitems_realpath != NULL);
    const char *schemes[NB_LOCATIONS] = { "file://", "dir://" };
    for (unsigned i = 0; i < NB_LOCATIONS; ++i)
    {
        char *location;
        assert (asprintf (&location, "%s%s", schemes[i], subitems_realpath) != -1);
        log ("Testing media_subitems: location: '%s'\n", location);
        media = libvlc_media_new_location (vlc, location);
        assert (media != NULL);
        test_media_subitems_media (media, false, true);
        free (location);
        libvlc_media_release (media);
    }
    free (subitems_realpath);

#ifdef HAVE_OPENAT
    /* listing directory via a fd works only if HAVE_OPENAT is defined */
    int fd = open (subitems_path, O_RDONLY);
    log ("Testing media_subitems: fd: '%d'\n", fd);
    assert (fd >= 0);
    media = libvlc_media_new_fd (vlc, fd);
    assert (media != NULL);
    test_media_subitems_media (media, true, true);
    libvlc_media_release (media);
    vlc_close (fd);
#else
#warning not testing subitems list via a fd location
#endif

    log ("Testing media_subitems failure\n");
    media = libvlc_media_new_location (vlc, "wrongfile://test");
    assert (media != NULL);
    test_media_subitems_media (media, false, false);
    libvlc_media_release (media);
}

int main(int i_argc, char *ppsz_argv[])
{
    test_init();

    libvlc_instance_t *vlc = libvlc_new (test_defaults_nargs,
                                         test_defaults_args);
    assert (vlc != NULL);

    char *psz_test_arg = i_argc > 1 ? ppsz_argv[1] : NULL;
    if (psz_test_arg != NULL)
    {
        alarm(0);
        const char *psz_test_url;
        const char *psz_test_path;
        if (strstr(psz_test_arg, "://") != NULL)
        {
            psz_test_url = psz_test_arg;
            psz_test_path = NULL;
        }
        else
        {
            psz_test_url = NULL;
            psz_test_path = psz_test_arg;
        }
        test_media_preparsed (vlc, psz_test_path, psz_test_url,
                              libvlc_media_parse_network,
                              libvlc_media_parsed_status_done);
        return 0;
    }

    test_media_preparsed (vlc, SRCDIR"/samples/image.jpg", NULL,
                          libvlc_media_parse_local,
                          libvlc_media_parsed_status_done);
    test_media_preparsed (vlc, NULL, "http://parsing_should_be_skipped.org/video.mp4",
                          libvlc_media_parse_local,
                          libvlc_media_parsed_status_skipped);
    test_media_preparsed (vlc, NULL, "unknown://parsing_should_be_skipped.org/video.mp4",
                          libvlc_media_parse_local,
                          libvlc_media_parsed_status_skipped);
    test_media_subitems (vlc);

    /* Testing libvlc_MetadataRequest timeout and libvlc_MetadataCancel. For
     * that, we need to create a local input_item_t based on a pipe. There is
     * no way to do that with a libvlc_media_t, that's why we don't use
     * libvlc_media_parse*() */

    test_input_metadata_timeout (vlc, 100, 0);
    test_input_metadata_timeout (vlc, 0, 100);

    libvlc_release (vlc);

    return 0;
}

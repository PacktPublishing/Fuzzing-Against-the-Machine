#define NB_PROFILE \
    (sizeof(video_profile_value_list)/sizeof(video_profile_value_list[0]))

static const char video_profile_name_list[][37] = {
    "Video - H.264 + MP3 (MP4)",
    "Video - VP80 + Vorbis (Webm)",
    "Video - H.264 + MP3 (TS)",
    "Video - H.265 + MP3 (MP4)",
    "Video - Theora + Vorbis (OGG)",
    "Video - MPEG-2 + MPGA (TS)",
    "Video - Dirac + MP3 (TS)",
    "Video - WMV + WMA (ASF)",
    "Video - DIV3 + MP3 (ASF)",
    "Audio - Vorbis (OGG)",
    "Audio - MP3",
    "Audio - FLAC",
    "Audio - CD",
    "Video for MPEG4 720p TV/device",
    "Video for MPEG4 1080p TV/device",
    "Video for DivX compatible player",
    "Video for iPod SD",
    "Video for iPod HD/iPhone/PSP",
    "Video for Android SD Low",
    "Video for Android SD High",
    "Video for Android HD",
    "Video for Youtube SD",
    "Video for Youtube HD",
};

/* !!! Do not add profiles here without widget or muxer alias (see standard.c) support in profile editor !!! */
static const char * video_profile_value_list[] = {
    "audio_enable=yes;audio_codec=mpga;acodec_bitrate=128;acodec_channels=2;acodec_samplerate=44100;video_enable=yes;video_codec=h264;vcodec_bitrate=0;vcodec_qp=0;vcodec_framerate=0;vcodec_width=0;vcodec_height=0;muxer_mux=mp4",
    "video_enable=yes;video_codec=VP80;vcodec_bitrate=2000;vcodec_framerate=0;vcodec_width=0;vcodec_height=0;audio_enable=yes;audio_codec=vorb;acodec_bitrate=128;acodec_channels=2;acodec_samplerate=44100;muxer_mux=webm",
    "audio_enable=yes;audio_codec=mpga;acodec_bitrate=128;acodec_channels=2;acodec_samplerate=44100;video_enable=yes;video_codec=h264;vcodec_bitrate=800;vcodec_qp=0;vcodec_framerate=0;vcodec_width=0;vcodec_height=0;muxer_mux=ts",
    "audio_enable=yes;audio_codec=mpga;acodec_bitrate=128;acodec_channels=2;acodec_samplerate=44100;video_enable=yes;video_codec=hevc;vcodec_bitrate=0;vcodec_framerate=0;vcodec_width=0;vcodec_height=0;muxer_mux=mp4",
    "audio_enable=yes;audio_codec=vorb;acodec_bitrate=128;acodec_channels=2;acodec_samplerate=44100;video_enable=yes;video_codec=theo;vcodec_bitrate=800;vcodec_framerate=0;vcodec_width=0;vcodec_height=0;muxer_mux=ogg",
    "audio_enable=yes;audio_codec=mpga;acodec_bitrate=128;acodec_channels=2;acodec_samplerate=44100;video_enable=yes;video_codec=mp2v;vcodec_bitrate=800;vcodec_framerate=0;vcodec_width=0;vcodec_height=0;muxer_mux=ts",
    "video_enable=yes;video_codec=drac;vcodec_bitrate=800;vcodec_framerate=0;vcodec_width=0;vcodec_height=0;audio_enable=yes;audio_codec=mpga;acodec_bitrate=128;acodec_channels=2;acodec_samplerate=44100;muxer_mux=ts",
    "audio_enable=yes;audio_codec=wma2;acodec_bitrate=128;acodec_channels=2;acodec_samplerate=44100;video_enable=yes;video_codec=WMV2;vcodec_bitrate=800;vcodec_framerate=0;vcodec_width=0;vcodec_height=0;muxer_mux=asf",
    "audio_enable=yes;audio_codec=mp3;acodec_bitrate=128;acodec_channels=2;acodec_samplerate=44100;video_enable=yes;video_codec=DIV3;vcodec_bitrate=800;vcodec_framerate=0;vcodec_width=0;vcodec_height=0;muxer_mux=asf",
    "audio_enable=yes;audio_codec=vorb;acodec_bitrate=128;acodec_channels=2;acodec_samplerate=44100;muxer_mux=ogg",
    "audio_enable=yes;audio_codec=mp3;acodec_bitrate=128;acodec_channels=2;acodec_samplerate=44100;muxer_mux=mp3",
    "audio_enable=yes;audio_codec=flac;acodec_bitrate=128;acodec_channels=2;acodec_samplerate=44100;muxer_mux=flac",
    "audio_enable=yes;audio_codec=s16l;acodec_bitrate=128;acodec_channels=2;acodec_samplerate=44100;muxer_mux=wav",
    "audio_enable=yes;audio_codec=mp3;acodec_bitrate=192;acodec_channels=2;acodec_samplerate=44100;video_enable=yes;video_codec=h264;vcodec_bitrate=1500;vcodec_qp=0;vcodec_framerate=0;vcodec_width=1280;vcodec_height=720;muxer_mux=mp4",
    "audio_enable=yes;audio_codec=mp3;acodec_bitrate=192;acodec_channels=2;acodec_samplerate=44100;video_enable=yes;video_codec=h264;vcodec_bitrate=3500;vcodec_qp=0;vcodec_framerate=0;vcodec_width=1920;vcodec_height=1080;muxer_mux=mp4",
    "video_enable=yes;video_codec=DIV3;vcodec_bitrate=900;vcodec_framerate=0;vcodec_width=720;vcodec_height=576;audio_enable=yes;audio_codec=mp3;acodec_bitrate=128;acodec_channels=2;acodec_samplerate=44100;muxer_mux=avi",
    "audio_enable=yes;audio_codec=mp3;acodec_bitrate=128;acodec_channels=2;acodec_samplerate=44100;video_enable=yes;video_codec=h264;vcodec_bitrate=600;vcodec_qp=0;vcodec_framerate=0;vcodec_width=320;vcodec_height=180;muxer_mux=mp4",
    "audio_enable=yes;audio_codec=mp3;acodec_bitrate=128;acodec_channels=2;acodec_samplerate=44100;video_enable=yes;video_codec=h264;vcodec_bitrate=700;vcodec_qp=0;vcodec_framerate=0;vcodec_width=480;vcodec_height=272;muxer_mux=mp4",
    "audio_enable=yes;audio_codec=mp3;acodec_bitrate=24;acodec_channels=1;acodec_samplerate=44100;video_enable=yes;video_codec=h264;vcodec_bitrate=56;vcodec_qp=0;vcodec_framerate=12;vcodec_width=176;vcodec_height=144;vcodec_custom=profile%3Dbaseline;muxer_mux=mp4",
    "audio_enable=yes;audio_codec=mp3;acodec_bitrate=128;acodec_channels=2;acodec_samplerate=44100;video_enable=yes;video_codec=h264;vcodec_bitrate=500;vcodec_qp=0;vcodec_framerate=0;vcodec_width=480;vcodec_height=360;vcodec_custom=profile%3Dbaseline;muxer_mux=mp4",
    "audio_enable=yes;audio_codec=mp3;acodec_bitrate=192;acodec_channels=2;acodec_samplerate=44100;video_enable=yes;video_codec=h264;vcodec_bitrate=2000;vcodec_qp=0;vcodec_framerate=0;vcodec_width=1280;vcodec_height=720;vcodec_custom=profile%3Dbaseline;muxer_mux=mp4",
    "audio_enable=yes;audio_codec=mp3;acodec_bitrate=128;acodec_channels=2;acodec_samplerate=44100;video_enable=yes;video_codec=h264;vcodec_bitrate=800;vcodec_qp=0;vcodec_framerate=0;vcodec_width=640;vcodec_height=480;muxer_mux=mp4",
    "audio_enable=yes;audio_codec=mp3;acodec_bitrate=128;acodec_channels=2;acodec_samplerate=44100;video_enable=yes;video_codec=h264;vcodec_bitrate=1500;vcodec_qp=0;vcodec_framerate=0;vcodec_width=1280;vcodec_height=720;muxer_mux=mp4",
};



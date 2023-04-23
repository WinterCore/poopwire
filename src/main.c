#define _POSIX_C_SOURCE 200809L

#include <spa/param/audio/format-utils.h>
#include <pipewire/pipewire.h>
#include <stdio.h>
#include <math.h>

#define DEFAULT_RATE 44100
#define DEFAULT_CHANNELS 2
#define DEFAULT_VOLUME 1

struct data {
    struct pw_main_loop *loop;
    struct pw_stream *stream;
    int16_t *filedata;
    size_t pos;
    size_t offset_pos;
};

static void on_process(void *userdata) {
    struct data *data = userdata;
    struct pw_buffer *b = pw_stream_dequeue_buffer(data->stream);

    if (b == NULL) {
        pw_log_warn("Out of buffers: %m");
        return;
    }

    struct spa_buffer *buf = b->buffer;
    int16_t *dst = buf->datas[0].data;
    if (dst == NULL) {
        return;
    }

    size_t stride = sizeof(int16_t) * DEFAULT_CHANNELS;
    size_t n_frames = SPA_MIN(b->requested, buf->datas[0].maxsize / stride);
    // size_t n_frames = 10000;
    // int n_frames = buf->datas[0].maxsize / stride;
    // printf("Requested data: %ld\n", n_frames);
    // printf("[DEBUG]: Producing data\n");
    // fflush(stdout);

    for (size_t i = 0; i < n_frames; i += 1) {

        for (size_t c = 0; c < DEFAULT_CHANNELS; c += 1) {
            int16_t a = data->filedata[data->pos];
            int16_t b = data->filedata[data->offset_pos];
            *dst = (a + b) / 2;
            data->pos += 1;
            data->offset_pos += 1;
            dst += 1;
        }
    }

    /*
    for (int i = 0; i < n_frames; i += 1) {
        data->accumulator += M_PI_M2 * 3000 / DEFAULT_RATE;
        if (data->accumulator >= M_PI_M2) {
            data->accumulator -= M_PI_M2;
        }

        int16_t val = sin(data->accumulator) * DEFAULT_VOLUME * 16767.f;
        for (int c = 0; c < DEFAULT_CHANNELS; c += 1) {
            *dst++ = val;
        }
    }
    */

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = stride;
    buf->datas[0].chunk->size = n_frames * stride;

    pw_stream_queue_buffer(data->stream, b);
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .process = on_process,
};

int main(int argc, char **argv) {
    printf(
        "compiled with libpipewire %s\n"
        "Linked with libpipewire%s\n",
        pw_get_headers_version(),
        pw_get_library_version()
    );

    // TODO: Add support for wav files
    FILE *fd = fopen("/home/winter/Downloads/someday-that-summer.raw", "rb");

    if (fd == NULL) {
        perror("[ERROR]: Failed to open audio file!\n");
        exit(EXIT_FAILURE);
    }

    fseek(fd, 0, SEEK_END);
    size_t size = ftell(fd);
    uint8_t *filedata = malloc(size);

    fseek(fd, 0, SEEK_SET);
    fread(filedata, 1, size, fd);
    fclose(fd);

    struct data data = {0};
    pw_init(&argc, &argv);
    data.filedata = (int16_t *) filedata;
    data.loop = pw_main_loop_new(NULL);
    data.stream = pw_stream_new_simple(
        pw_main_loop_get_loop(data.loop),
        "pepepopo",
        pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Playback",
            PW_KEY_MEDIA_ROLE, "Music",
            NULL
        ),
        &stream_events,
        &data
    );
    data.pos = 0;
    data.offset_pos = size / 4;

    const struct spa_pod *params[1];
    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    params[0] = spa_format_audio_raw_build(
        &b,
        SPA_PARAM_EnumFormat,
        &SPA_AUDIO_INFO_RAW_INIT(
            .format = SPA_AUDIO_FORMAT_S16,
            .channels = DEFAULT_CHANNELS,
            .rate = DEFAULT_RATE
        )
    );

    pw_stream_connect(
        data.stream,
        PW_DIRECTION_OUTPUT,
        PW_ID_ANY,
        PW_STREAM_FLAG_AUTOCONNECT |
        PW_STREAM_FLAG_MAP_BUFFERS |
        PW_STREAM_FLAG_RT_PROCESS,
        params,
        1
    );

    pw_main_loop_run(data.loop);
    pw_stream_destroy(data.stream);
    pw_main_loop_destroy(data.loop);

    return 0;
}

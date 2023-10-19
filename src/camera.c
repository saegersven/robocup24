/*
* Controls camera at /dev/video0 via V4L2 (+libv4l)
* Initializes camera and starts capture thread with start_capture().
* Grabs frames and stores reference. Frames are retrieved and
* copied with grab_frame().
* stop_capture() closes camera and stops thread.
*/

#include "camera.h"

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>

#include <linux/videodev2.h>
#include <pthread.h>
#include "../libs/libv4l/include/libv4l2.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

/* Safe ioctl */
static void xioctl(int fh, int request, void* arg) {
    int r;
    do {
        r = v4l2_ioctl(fh, request, arg);
    } while(r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

    if(r == -1) {
        fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

pthread_mutex_t frame_lock = PTHREAD_MUTEX_INITIALIZER;
Image current_frame;
int has_frame;

pthread_mutex_t signal_lock = PTHREAD_MUTEX_INITIALIZER;
int stop_signal;

pthread_t capture_thread_id;

void start_capture(int width, int height) {
    alloc_image(&current_frame);
    stop_signal = 0;
    has_frame = 0;

    struct image_size *size = malloc(sizeof(struct image_size));
    size->width = width;
    size->height = height;
    
    pthread_create(&capture_thread_id, NULL, capture_loop, (void*)size);
}

void stop_capture() {
    pthread_mutex_lock(&signal_lock);
    stop_signal = 1;
    pthread_mutex_unlock(&signal_lock);

    pthread_join(capture_thread_id, NULL);
}

Image grab_frame() {
    Image frame;

    pthread_mutex_lock(&frame_lock);

    /* TODO: Add timeout */
    while(!has_frame) {
        pthread_mutex_unlock(&frame_lock);
        usleep(10);
        pthread_mutex_lock(&frame_lock);
    }

    frame.width = current_frame.width;
    frame.height = current_frame.height;
    frame.channels = current_frame.channels; /* Always 3 */
    alloc_image(&frame);

    memcpy(frame.data, current_frame.data, frame.width * frame.height * frame.channels);

    has_frame = 0;

    pthread_mutex_unlock(&frame_lock);

    return frame;
}

void *capture_loop(void *size) {
    pthread_detach(pthread_self());

    struct v4l2_format          fmt;
    struct v4l2_buffer          buf;
    struct v4l2_requestbuffers  req;
    enum v4l2_buf_type          type;
    fd_set                      fds;
    struct timeval              tv;
    int                         r, fd = -1;
    unsigned int                i, n_buffers;
    char                        *dev_name = "/dev/video0";
    struct image_data_buffer    *buffers;

    unsigned int width = ((struct image_size*)size)->width;
    unsigned int height = ((struct image_size*)size)->height;

    fd = v4l2_open(dev_name, O_RDWR | O_NONBLOCK, 0);
    if(fd < 0) {
        fprintf(stderr, "Could not open camera device\n");
        exit(EXIT_FAILURE);
    }

    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = width;
    fmt.fmt.pix.height      = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
    xioctl(fd, VIDIOC_S_FMT, &fmt);

    if(fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_BGR24) {
        printf("Libv4l did not accept BGR24 format\n");
        exit(EXIT_FAILURE);
    }

    if(fmt.fmt.pix.width != width || (fmt.fmt.pix.height != height)) {
        printf("Driver is sending image at %dx%d\n",
            fmt.fmt.pix.width, fmt.fmt.pix.height);
    }

    CLEAR(req);
    req.count = 2;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    xioctl(fd, VIDIOC_REQBUFS, &req);

    buffers = calloc(req.count, sizeof(*buffers));
    for(n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        CLEAR(buf);

        buf.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory  = V4L2_MEMORY_MMAP;
        buf.index   = n_buffers;

        xioctl(fd, VIDIOC_QUERYBUF, &buf);

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = v4l2_mmap(NULL, buf.length,
            PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

        if(MAP_FAILED == buffers[n_buffers].start) {
            fprintf(stderr, "mmap failed\n");
            exit(EXIT_FAILURE);
        }
    }

    for(i = 0; i < n_buffers; ++i) {
        CLEAR(buf);
        buf.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory  = V4L2_MEMORY_MMAP;
        buf.index = i;
        xioctl(fd, VIDIOC_QBUF, &buf);
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    xioctl(fd, VIDIOC_STREAMON, &type);
    while(1) {
        do {
            FD_ZERO(&fds);
            FD_SET(fd, &fds);

            /* Timeout */
            tv.tv_sec = 2;
            tv.tv_usec = 0;

            r = select(fd + 1, &fds, NULL, NULL, &tv);
        } while((r == -1) && (errno == EINTR));

        if(r == -1) {
            fprintf(stderr, "select failed\n");
            exit(EXIT_FAILURE);
        }

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        xioctl(fd, VIDIOC_DQBUF, &buf);

        /* Convert to image */
        pthread_mutex_lock(&frame_lock);
        current_frame.width = fmt.fmt.pix.width;
        current_frame.height = fmt.fmt.pix.height;
        current_frame.channels = 3;
        current_frame.data = buffers[buf.index].start;

        has_frame = 1;
        pthread_mutex_unlock(&frame_lock);

        /* Check if stop signal was activated */
        pthread_mutex_lock(&signal_lock);
        int sig = stop_signal;
        pthread_mutex_unlock(&signal_lock);
        if(sig) {
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            xioctl(fd, VIDIOC_STREAMOFF, &type);
            for(i = 0; i < n_buffers; ++i) {
                v4l2_munmap(buffers[i].start, buffers[i].length);
            }
            v4l2_close(fd);
            
            pthread_exit(NULL);
        }

        /* Queue next buffer */
        xioctl(fd, VIDIOC_QBUF, &buf);
    }
}
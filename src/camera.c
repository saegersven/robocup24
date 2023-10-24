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

#include "utils.h"

// Safe ioctl
static void xioctl(int fh, int request, void *arg) {
    int r;
    do {
        r = v4l2_ioctl(fh, request, args);
    } while(r == -1 && ((errno = EINTR) || (errno == EAGAIN)));

    if(r == -1) {
	    fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
	    exit(EXIT_FAILURE);
    }
}

static pthread_mutex_t frame_lock = PTHREAD_MUTEX_INITIALIZER;
static Image current_frame;
static int has_frame;

static pthread_mutex_t signal_lock = PTHREAD_MUTEX_INITIALIZER;
static int stop_signal;

static pthread_t capture_thread_id;

static int camera_fd;

static const int NUM_BUFFERS = 2;

void camera_start_capture(int width, int height) {
    alloc_image(&current_frame);
    stop_signal = 0;
    has_frame = 0;

    struct image_size *size = malloc(sizeof(struct image_size));
    size->width = width;
    size->height = height;
    
    pthread_create(&capture_thread_id, NULL, camera_capture_loop, (void*)size);
}

void camera_stop_capture() {
    pthread_mutex_lock(&signal_lock);
    stop_signal = 1;
    pthread_mutex_unlock(&signal_lock);

    pthread_join(capture_thread_id, NULL);
}

Image camera_grab_frame() {
    Image frame;

    pthread_mutex_lock(&frame_lock);

    // TODO: Add timeout
    while(!has_frame) {
        pthread_mutex_unlock(&frame_lock);
        usleep(10);
        pthread_mutex_lock(&frame_lock);
    }

    frame.width = current_frame.width;
    frame.height = current_frame.height;
    frame.channels = current_frame.channels; // Always 3
    alloc_image(&frame);

    memcpy(frame.data, current_frame.data, frame.width * frame.height * frame.channels);

    has_frame = 0;

    pthread_mutex_unlock(&frame_lock);

    return frame;
}

void camera_init_capture(struct image_size size, struct v4l2_format *fmt, char *dev_name) {
    camera_fd = v4l2_open(dev_name, O_RDWR | O_NONBLOCK, 0);
    if(camera_fd < 0) {
        fprintf(stderr, "Could not open camera device\n");
        exit(EXIT_FAILURE);
    }

    CLEAR(fmt);
    fmt->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt->fmt.pix.width       = size.width;
    fmt->fmt.pix.height      = size.height;
    fmt->fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
    fmt->fmt.pix.field       = V4L2_FIELD_INTERLACED;
    xioctl(camera_fd, VIDIOC_S_FMT, fmt);

    if(fmt->fmt.pix.pixelformat != V4L2_PIX_FMT_BGR24) {
        printf("Libv4l did not accept BGR24 format\n");
        exit(EXIT_FAILURE);
    }

    if(fmt->fmt.pix.width != size.width || (fmt->fmt.pix.height != size.height)) {
        printf("Driver is sending image at %dx%d\n",
            fmt->fmt.pix.width, fmt->fmt.pix.height);
    }
}

void camera_init_buffers(struct v4l2_requestbuffers *req, struct image_data_buffer *buffers, struct v4l2_buffer *buf) {
    CLEAR(*req);
    req->count = NUM_BUFFERS;
    req->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req->memory = V4L2_MEMORY_MMAP;
    xioctl(camera_fd, VIDIOC_REQBUFS, req);

    buffers = calloc(req->count, sizeof(*buffers));
    for(int i = 0; i < NUM_BUFFERS; i++) {
        CLEAR(*buf);

        buf->type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf->memory  = V4L2_MEMORY_MMAP;
        buf->index   = i;

        xioctl(camera_fd, VIDIOC_QUERYBUF, buf);

        buffers[i].length = buf->length;
        buffers[i].start = v4l2_mmap(NULL, buf->length,
            PROT_READ | PROT_WRITE, MAP_SHARED, camera_fd, buf->m.offset);

        if(MAP_FAILED == buffers[i].start) {
            fprintf(stderr, "mmap failed\n");
            exit(EXIT_FAILURE);
        }
    }
}

camera_queue_buffers(struct v4l2_buffer *buf, enum v4l2_buf_type *type) {
    for(i = 0; i < NUM_BUFFERS; ++i) {
        CLEAR(*buf);
        buf->type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf->memory  = V4L2_MEMORY_MMAP;
        buf->index = i;
        xioctl(camera_fd, VIDIOC_QBUF, buf);
    }
    *type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
}

void *camera_capture_loop(void *size) {
    pthread_detach(pthread_self());

    struct v4l2_format          fmt;
    struct v4l2_buffer          buf;
    struct v4l2_requestbuffers  req;
    enum v4l2_buf_type          type;
    fd_set                      fds;
    struct timeval              tv;
    int                         r;
    unsigned int                i, n_buffers;
    char                        *dev_name = "/dev/video0";
    struct image_data_buffer    *buffers;

    // Camera initialization (request for specified format)
    camera_init_capture(*((struct image_size*)size), &fmt, dev_name);
    
    // Init and request two buffers, do memory mapping
    camera_init_buffers(&req, buffers, &buf);

    // Queue the two buffers
    camera_queue_buffers(&buf, &type);

    // Capture
    xioctl(camera_fd, VIDIOC_STREAMON, &type);
    while(1) {
        // Set timeout
        do {
            FD_ZERO(&fds);
            FD_SET(camera_fd, &fds);

            // Timeout
            tv.tv_sec = 2;
            tv.tv_usec = 0;

            r = select(camera_fd + 1, &fds, NULL, NULL, &tv);
        } while((r == -1) && (errno == EINTR));

        if(r == -1) {
            fprintf(stderr, "select failed\n");
            exit(EXIT_FAILURE);
        }

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        xioctl(camera_fd, VIDIOC_DQBUF, &buf);

        // Put data into current_frame image container
        pthread_mutex_lock(&frame_lock);
        current_frame.width = fmt.fmt.pix.width;
        current_frame.height = fmt.fmt.pix.height;
        current_frame.channels = 3;
        current_frame.data = buffers[buf.index].start;

        has_frame = 1;
        pthread_mutex_unlock(&frame_lock);

        // Check if stop signal was activated
        pthread_mutex_lock(&signal_lock);
        int sig = stop_signal;
        pthread_mutex_unlock(&signal_lock);
        if(sig) {
            // Stop capture
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            xioctl(camera_fd, VIDIOC_STREAMOFF, &type);
            for(i = 0; i < n_buffers; ++i) {
                v4l2_munmap(buffers[i].start, buffers[i].length);
            }
            v4l2_close(camera_fd);
            
            pthread_exit(NULL);
        }

        // Re-queue buffer
        xioctl(camera_fd, VIDIOC_QBUF, &buf);
    }
}

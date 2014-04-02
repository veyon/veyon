/*
 *  Copyright (C) 2012 Intel Corporation.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#ifdef LIBVNCSERVER_CONFIG_LIBVA

#include <X11/Xlib.h>
#include <va/va_x11.h>

enum _slice_types {
	SLICE_TYPE_P = 0,  /* Predicted */
	SLICE_TYPE_B = 1,  /* Bi-predicted */
	SLICE_TYPE_I = 2,  /* Intra coded */
};

#define SURFACE_NUM     7

VADisplay       va_dpy = NULL;
VAConfigID      va_config_id;
VASurfaceID     va_surface_id[SURFACE_NUM];
VAContextID     va_context_id = 0;

VABufferID      va_pic_param_buf_id[SURFACE_NUM];
VABufferID      va_mat_param_buf_id[SURFACE_NUM];
VABufferID      va_sp_param_buf_id[SURFACE_NUM];
VABufferID      va_d_param_buf_id[SURFACE_NUM];

static int cur_height = 0;
static int cur_width = 0;
static unsigned int num_frames = 0;
static int sid = 0;
static unsigned int frame_id = 0;
static int field_order_count = 0;
static VASurfaceID curr_surface = VA_INVALID_ID;

VAStatus gva_status;
VASurfaceStatus gsurface_status;
#define CHECK_SURF(X) \
    gva_status = vaQuerySurfaceStatus(va_dpy, X, &gsurface_status); \
    if (gsurface_status != 4) printf("ss: %d\n", gsurface_status);

#ifdef _DEBUG
#define DebugLog(A) rfbClientLog A
#else
#define DebugLog(A)
#endif

#define CHECK_VASTATUS(va_status,func)                  \
    if (va_status != VA_STATUS_SUCCESS) {                   \
        /*fprintf(stderr,"%s:%s (%d) failed,exit\n", __func__, func, __LINE__);*/ \
        rfbClientErr("%s:%s:%d failed (0x%x),exit\n", __func__, func, __LINE__, va_status); \
        exit(1);                                \
    } else  { \
        /*fprintf(stderr,">> SUCCESS for: %s:%s (%d)\n", __func__, func, __LINE__);*/ \
        DebugLog(("%s:%s:%d success\n", __func__, func, __LINE__)); \
    }

/*
 * Forward declarations
 */
static void h264_decode_frame(int f_width, int f_height, char *framedata, int framesize, int slice_type);
static void SetVAPictureParameterBufferH264(VAPictureParameterBufferH264 *p, int width, int height);
static void SetVASliceParameterBufferH264(VASliceParameterBufferH264 *p);
static void SetVASliceParameterBufferH264_Intra(VASliceParameterBufferH264 *p, int first);

static void put_updated_rectangle(rfbClient *client, int x, int y, int width, int height, int f_width, int f_height, int first_for_frame);
static void nv12_to_rgba(const VAImage vaImage, rfbClient *client, int ch_x, int ch_y, int ch_w, int ch_h);


/* FIXME: get this value from the server instead of hardcoding 32bit pixels */
#define BPP (4 * 8)

static const char *string_of_FOURCC(uint32_t fourcc)
{
    static int buf;
    static char str[2][5];

    buf ^= 1;
    str[buf][0] = fourcc;
    str[buf][1] = fourcc >> 8;
    str[buf][2] = fourcc >> 16;
    str[buf][3] = fourcc >> 24;
    str[buf][4] = '\0';
    return str[buf];
}

static inline const char *string_of_VAImageFormat(VAImageFormat *imgfmt)
{
    return string_of_FOURCC(imgfmt->fourcc);
}


static rfbBool
HandleH264 (rfbClient* client, int rx, int ry, int rw, int rh)
{
    rfbH264Header hdr;
    char *framedata;

    DebugLog(("Framebuffer update with H264 (x: %d, y: %d, w: %d, h: %d)\n", rx, ry, rw, rh));

    /* First, read the frame size and allocate buffer to store the data */
    if (!ReadFromRFBServer(client, (char *)&hdr, sz_rfbH264Header))
        return FALSE;

    hdr.slice_type = rfbClientSwap32IfLE(hdr.slice_type);
    hdr.nBytes = rfbClientSwap32IfLE(hdr.nBytes);
    hdr.width = rfbClientSwap32IfLE(hdr.width);
    hdr.height = rfbClientSwap32IfLE(hdr.height);

    framedata = (char*) malloc(hdr.nBytes);

    /* Obtain frame data from the server */
    DebugLog(("Reading %d bytes of frame data (type: %d)\n", hdr.nBytes, hdr.slice_type));
    if (!ReadFromRFBServer(client, framedata, hdr.nBytes))
        return FALSE;

    /* First make sure we have a large enough raw buffer to hold the
     * decompressed data.  In practice, with a fixed BPP, fixed frame
     * buffer size and the first update containing the entire frame
     * buffer, this buffer allocation should only happen once, on the
     * first update.
     */
    if ( client->raw_buffer_size < (( rw * rh ) * ( BPP / 8 ))) {
        if ( client->raw_buffer != NULL ) {
            free( client->raw_buffer );
        }

        client->raw_buffer_size = (( rw * rh ) * ( BPP / 8 ));
        client->raw_buffer = (char*) malloc( client->raw_buffer_size );
        rfbClientLog("Allocated raw buffer of %d bytes (%dx%dx%d BPP)\n", client->raw_buffer_size, rw, rh, BPP);
    }

    /* Decode frame if frame data was sent. Server only sends frame data for the first
     * framebuffer update message for a particular frame buffer contents.
     * If more than 1 rectangle is updated, the messages after the first one (with
     * the H.264 frame) have nBytes == 0.
     */
    if (hdr.nBytes > 0) {
        DebugLog(("  decoding %d bytes of H.264 data\n", hdr.nBytes));
        h264_decode_frame(hdr.width, hdr.height, framedata, hdr.nBytes, hdr.slice_type);
    }

    DebugLog(("  updating rectangle (%d, %d)-(%d, %d)\n", rx, ry, rw, rh));
    put_updated_rectangle(client, rx, ry, rw, rh, hdr.width, hdr.height, hdr.nBytes != 0);

    free(framedata);

    return TRUE;
}

static void h264_cleanup_decoder()
{
    VAStatus va_status;

    rfbClientLog("%s()\n", __FUNCTION__);

    if (va_surface_id[0] != VA_INVALID_ID) {
        va_status = vaDestroySurfaces(va_dpy, &va_surface_id[0], SURFACE_NUM);
        CHECK_VASTATUS(va_status, "vaDestroySurfaces");
    }

    if (va_context_id) {
        va_status = vaDestroyContext(va_dpy, va_context_id);
        CHECK_VASTATUS(va_status, "vaDestroyContext");
        va_context_id = 0;
    }

    num_frames = 0;
    sid = 0;
    frame_id = 0;
    field_order_count = 0;
}

static void h264_init_decoder(int width, int height)
{
    VAStatus va_status;

    if (va_context_id) {
        rfbClientLog("%s: va_dpy already initialized\n", __FUNCTION__);
    }

    if (va_dpy != NULL) {
        rfbClientLog("%s: Re-initializing H.264 decoder\n", __FUNCTION__);
    }
    else {
        rfbClientLog("%s: initializing H.264 decoder\n", __FUNCTION__);

        /* Attach VA display to local X display */
        Display *win_display = (Display *)XOpenDisplay(":0.0");
        if (win_display == NULL) {
            rfbClientErr("Can't connect to local display\n");
            exit(-1);
        }

        int major_ver, minor_ver;
        va_dpy = vaGetDisplay(win_display);
        va_status = vaInitialize(va_dpy, &major_ver, &minor_ver);
        CHECK_VASTATUS(va_status, "vaInitialize");
        rfbClientLog("%s: libva version %d.%d found\n", __FUNCTION__, major_ver, minor_ver);
    }

    /* Check for VLD entrypoint */
    int num_entrypoints;
    VAEntrypoint    entrypoints[5];
    int vld_entrypoint_found = 0;

    /* Change VAProfileH264High if needed */
    VAProfile profile = VAProfileH264High;
    va_status = vaQueryConfigEntrypoints(va_dpy, profile, entrypoints, &num_entrypoints);
    CHECK_VASTATUS(va_status, "vaQueryConfigEntrypoints");
    int i;
    for (i = 0; i < num_entrypoints; ++i) {
        if (entrypoints[i] == VAEntrypointVLD) {
            vld_entrypoint_found = 1;
            break;
        }
    }

    if (vld_entrypoint_found == 0) {
        rfbClientErr("VLD entrypoint not found\n");
        exit(1);
    }

    /* Create configuration for the decode pipeline */
    VAConfigAttrib attrib;
    attrib.type = VAConfigAttribRTFormat;
    va_status = vaCreateConfig(va_dpy, profile, VAEntrypointVLD, &attrib, 1, &va_config_id);
    CHECK_VASTATUS(va_status, "vaCreateConfig");

    /* Create VA surfaces */
    for (i = 0; i < SURFACE_NUM; ++i) {
        va_surface_id[i]       = VA_INVALID_ID;
        va_pic_param_buf_id[i] = VA_INVALID_ID;
        va_mat_param_buf_id[i] = VA_INVALID_ID;
        va_sp_param_buf_id[i]  = VA_INVALID_ID;
        va_d_param_buf_id[i]   = VA_INVALID_ID;
    }
    va_status = vaCreateSurfaces(va_dpy, width, height, VA_RT_FORMAT_YUV420, SURFACE_NUM, &va_surface_id[0]);
    CHECK_VASTATUS(va_status, "vaCreateSurfaces");
    for (i = 0; i < SURFACE_NUM; ++i) {
        DebugLog(("%s: va_surface_id[%d] = %p\n", __FUNCTION__, i, va_surface_id[i]));
    }

    /* Create VA context */
    va_status = vaCreateContext(va_dpy, va_config_id, width, height, 0/*VA_PROGRESSIVE*/,  &va_surface_id[0], SURFACE_NUM, &va_context_id);
    CHECK_VASTATUS(va_status, "vaCreateContext");
    DebugLog(("%s: VA context created (id: %d)\n", __FUNCTION__, va_context_id));


    /* Instantiate decode pipeline */
    va_status = vaBeginPicture(va_dpy, va_context_id, va_surface_id[0]);
    CHECK_VASTATUS(va_status, "vaBeginPicture");

    rfbClientLog("%s: H.264 decoder initialized\n", __FUNCTION__);
}

static void h264_decode_frame(int f_width, int f_height, char *framedata, int framesize, int slice_type)
{
    VAStatus va_status;

    DebugLog(("%s: called for frame of %d bytes (%dx%d) slice_type=%d\n", __FUNCTION__, framesize, width, height, slice_type));

    /* Initialize decode pipeline if necessary */
    if ( (f_width > cur_width) || (f_height > cur_height) ) {
        if (va_dpy != NULL)
            h264_cleanup_decoder();
        cur_width = f_width;
        cur_height = f_height;

        h264_init_decoder(f_width, f_height);
        rfbClientLog("%s: decoder initialized\n", __FUNCTION__);
    }

    /* Decode frame */
    static VAPictureH264 va_picture_h264, va_old_picture_h264;

    /* The server should always send an I-frame when a new client connects
     * or when the resolution of the framebuffer changes, but we check
     * just in case.
     */
    if ( (slice_type != SLICE_TYPE_I) && (num_frames == 0) ) {
        rfbClientLog("First frame is not an I frame !!! Skipping!!!\n");
        return;
    }

    DebugLog(("%s: frame_id=%d va_surface_id[%d]=0x%x field_order_count=%d\n", __FUNCTION__, frame_id, sid, va_surface_id[sid], field_order_count));

    va_picture_h264.picture_id = va_surface_id[sid];
    va_picture_h264.frame_idx  = frame_id;
    va_picture_h264.flags = 0;
    va_picture_h264.BottomFieldOrderCnt = field_order_count;
    va_picture_h264.TopFieldOrderCnt = field_order_count;

    /* Set up picture parameter buffer */
    if (va_pic_param_buf_id[sid] == VA_INVALID_ID) {
        va_status = vaCreateBuffer(va_dpy, va_context_id, VAPictureParameterBufferType, sizeof(VAPictureParameterBufferH264), 1, NULL, &va_pic_param_buf_id[sid]);
        CHECK_VASTATUS(va_status, "vaCreateBuffer(PicParam)");
    }
    CHECK_SURF(va_surface_id[sid]);

    VAPictureParameterBufferH264 *pic_param_buf = NULL;
    va_status = vaMapBuffer(va_dpy, va_pic_param_buf_id[sid], (void **)&pic_param_buf);
    CHECK_VASTATUS(va_status, "vaMapBuffer(PicParam)");

    SetVAPictureParameterBufferH264(pic_param_buf, f_width, f_height);
    memcpy(&pic_param_buf->CurrPic, &va_picture_h264, sizeof(VAPictureH264));

    if (slice_type == SLICE_TYPE_P) {
        memcpy(&pic_param_buf->ReferenceFrames[0], &va_old_picture_h264, sizeof(VAPictureH264));
        pic_param_buf->ReferenceFrames[0].flags = 0;
    }
    else if (slice_type != SLICE_TYPE_I) {
        rfbClientLog("Frame type %d not supported!!!\n");
        return;
    }
    pic_param_buf->frame_num = frame_id;

    va_status = vaUnmapBuffer(va_dpy, va_pic_param_buf_id[sid]);
    CHECK_VASTATUS(va_status, "vaUnmapBuffer(PicParam)");

    /* Set up IQ matrix buffer */
    if (va_mat_param_buf_id[sid] == VA_INVALID_ID) {
        va_status = vaCreateBuffer(va_dpy, va_context_id, VAIQMatrixBufferType, sizeof(VAIQMatrixBufferH264), 1, NULL, &va_mat_param_buf_id[sid]);
        CHECK_VASTATUS(va_status, "vaCreateBuffer(IQMatrix)");
    }
    CHECK_SURF(va_surface_id[sid]);

    VAIQMatrixBufferH264 *iq_matrix_buf = NULL;
    va_status = vaMapBuffer(va_dpy, va_mat_param_buf_id[sid], (void **)&iq_matrix_buf);
    CHECK_VASTATUS(va_status, "vaMapBuffer(IQMatrix)");

    static const unsigned char m_MatrixBufferH264[]= {
        /* ScalingList4x4[6][16] */
        0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
        0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
        0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
        0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
        0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
        0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
        /* ScalingList8x8[2][64] */
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    };

    memcpy(iq_matrix_buf, m_MatrixBufferH264, 224);
    va_status = vaUnmapBuffer(va_dpy, va_mat_param_buf_id[sid]);
    CHECK_VASTATUS(va_status, "vaUnmapBuffer(IQMatrix)");

    VABufferID buffer_ids[2];
    buffer_ids[0] = va_pic_param_buf_id[sid];
    buffer_ids[1] = va_mat_param_buf_id[sid];

    CHECK_SURF(va_surface_id[sid]);
    va_status = vaRenderPicture(va_dpy, va_context_id, buffer_ids, 2);
    CHECK_VASTATUS(va_status, "vaRenderPicture");

    /* Set up slice parameter buffer */
    if (va_sp_param_buf_id[sid] == VA_INVALID_ID) {
        va_status = vaCreateBuffer(va_dpy, va_context_id, VASliceParameterBufferType, sizeof(VASliceParameterBufferH264), 1, NULL, &va_sp_param_buf_id[sid]);
        CHECK_VASTATUS(va_status, "vaCreateBuffer(SliceParam)");
    }
    CHECK_SURF(va_surface_id[sid]);

    VASliceParameterBufferH264 *slice_param_buf = NULL;
    va_status = vaMapBuffer(va_dpy, va_sp_param_buf_id[sid], (void **)&slice_param_buf);
    CHECK_VASTATUS(va_status, "vaMapBuffer(SliceParam)");

    static int t2_first = 1;
    if (slice_type == SLICE_TYPE_I) {
        SetVASliceParameterBufferH264_Intra(slice_param_buf, t2_first);
        t2_first = 0;
    } else {
        SetVASliceParameterBufferH264(slice_param_buf);
        memcpy(&slice_param_buf->RefPicList0[0], &va_old_picture_h264, sizeof(VAPictureH264));
        slice_param_buf->RefPicList0[0].flags = 0;
    }
    slice_param_buf->slice_data_bit_offset = 0;
    slice_param_buf->slice_data_size = framesize;

    va_status = vaUnmapBuffer(va_dpy, va_sp_param_buf_id[sid]);
    CHECK_VASTATUS(va_status, "vaUnmapBuffer(SliceParam)");
    CHECK_SURF(va_surface_id[sid]);

    /* Set up slice data buffer and copy H.264 encoded data */
    if (va_d_param_buf_id[sid] == VA_INVALID_ID) {
        /* TODO use estimation matching framebuffer dimensions instead of this large value */
        va_status = vaCreateBuffer(va_dpy, va_context_id, VASliceDataBufferType, 4177920, 1, NULL, &va_d_param_buf_id[sid]); /* 1080p size */
        CHECK_VASTATUS(va_status, "vaCreateBuffer(SliceData)");
    }

    char *slice_data_buf;
    va_status = vaMapBuffer(va_dpy, va_d_param_buf_id[sid], (void **)&slice_data_buf);
    CHECK_VASTATUS(va_status, "vaMapBuffer(SliceData)");
    memcpy(slice_data_buf, framedata, framesize);

    CHECK_SURF(va_surface_id[sid]);
    va_status = vaUnmapBuffer(va_dpy, va_d_param_buf_id[sid]);
    CHECK_VASTATUS(va_status, "vaUnmapBuffer(SliceData)");

    buffer_ids[0] = va_sp_param_buf_id[sid];
    buffer_ids[1] = va_d_param_buf_id[sid];

    CHECK_SURF(va_surface_id[sid]);
    va_status = vaRenderPicture(va_dpy, va_context_id, buffer_ids, 2);
    CHECK_VASTATUS(va_status, "vaRenderPicture");

    va_status = vaEndPicture(va_dpy, va_context_id);
    CHECK_VASTATUS(va_status, "vaEndPicture");

    /* Prepare next one... */
    int sid_new = (sid + 1) % SURFACE_NUM;
    DebugLog(("%s: new Surface ID = %d\n", __FUNCTION__, sid_new));
    va_status = vaBeginPicture(va_dpy, va_context_id, va_surface_id[sid_new]);
    CHECK_VASTATUS(va_status, "vaBeginPicture");

    /* Get decoded data */
    va_status = vaSyncSurface(va_dpy, va_surface_id[sid]);
    CHECK_VASTATUS(va_status, "vaSyncSurface");
    CHECK_SURF(va_surface_id[sid]);

    curr_surface = va_surface_id[sid];

    sid = sid_new;

    field_order_count += 2;
    ++frame_id;
    if (frame_id > 15) {
        frame_id = 0;
    }

    ++num_frames;

    memcpy(&va_old_picture_h264, &va_picture_h264, sizeof(VAPictureH264));
}

static void put_updated_rectangle(rfbClient *client, int x, int y, int width, int height, int f_width, int f_height, int first_for_frame)
{
    if (curr_surface == VA_INVALID_ID) {
        rfbClientErr("%s: called, but current surface is invalid\n", __FUNCTION__);
        return;
    }

    VAStatus va_status;

    if (client->outputWindow) {
        /* use efficient vaPutSurface() method of putting the framebuffer on the screen */
        if (first_for_frame) {
            /* vaPutSurface() clears window contents outside the given destination rectangle => always update full screen. */
            va_status = vaPutSurface(va_dpy, curr_surface, client->outputWindow, 0, 0, f_width, f_height, 0, 0, f_width, f_height, NULL, 0, VA_FRAME_PICTURE);
            CHECK_VASTATUS(va_status, "vaPutSurface");
        }
    }
    else if (client->frameBuffer) {
        /* ... or copy the changed framebuffer region manually as a fallback */
        VAImage decoded_image;
        decoded_image.image_id = VA_INVALID_ID;
        decoded_image.buf      = VA_INVALID_ID;
        va_status = vaDeriveImage(va_dpy, curr_surface, &decoded_image);
        CHECK_VASTATUS(va_status, "vaDeriveImage");

        if ((decoded_image.image_id == VA_INVALID_ID) || (decoded_image.buf == VA_INVALID_ID)) {
            rfbClientErr("%s: vaDeriveImage() returned success but VA image is invalid (id: %d, buf: %d)\n", __FUNCTION__, decoded_image.image_id, decoded_image.buf);
        }

        nv12_to_rgba(decoded_image, client, x, y, width, height);

        va_status = vaDestroyImage(va_dpy, decoded_image.image_id);
        CHECK_VASTATUS(va_status, "vaDestroyImage");
    }
}

static void SetVAPictureParameterBufferH264(VAPictureParameterBufferH264 *p, int width, int height)
{
    int i;
    unsigned int width_in_mbs = (width + 15) / 16;
    unsigned int height_in_mbs = (height + 15) / 16;

    memset(p, 0, sizeof(VAPictureParameterBufferH264));
    p->picture_width_in_mbs_minus1 = width_in_mbs - 1;
    p->picture_height_in_mbs_minus1 = height_in_mbs - 1;
    p->num_ref_frames = 1;
    p->seq_fields.value = 145;
    p->pic_fields.value = 0x501;
    for (i = 0; i < 16; i++) {
        p->ReferenceFrames[i].flags = VA_PICTURE_H264_INVALID;
        p->ReferenceFrames[i].picture_id = 0xffffffff;
    }
}

static void SetVASliceParameterBufferH264(VASliceParameterBufferH264 *p)
{
    int i;
    memset(p, 0, sizeof(VASliceParameterBufferH264));
    p->slice_data_size = 0;
    p->slice_data_bit_offset = 64;
    p->slice_alpha_c0_offset_div2 = 2;
    p->slice_beta_offset_div2 = 2;
    p->chroma_weight_l0_flag = 1;
    p->chroma_weight_l0[0][0]=1;
    p->chroma_offset_l0[0][0]=0;
    p->chroma_weight_l0[0][1]=1;
    p->chroma_offset_l0[0][1]=0;
    p->luma_weight_l1_flag = 1;
    p->chroma_weight_l1_flag = 1;
    p->luma_weight_l0[0]=0x01;
    for (i = 0; i < 32; i++) {
        p->RefPicList0[i].flags = VA_PICTURE_H264_INVALID;
        p->RefPicList1[i].flags = VA_PICTURE_H264_INVALID;
    }
    p->RefPicList1[0].picture_id = 0xffffffff;
}

static void SetVASliceParameterBufferH264_Intra(VASliceParameterBufferH264 *p, int first)
{
    int i;
    memset(p, 0, sizeof(VASliceParameterBufferH264));
    p->slice_data_size = 0;
    p->slice_data_bit_offset = 64;
    p->slice_alpha_c0_offset_div2 = 2;
    p->slice_beta_offset_div2 = 2;
    p->slice_type = 2;
    if (first) {
        p->luma_weight_l0_flag = 1;
        p->chroma_weight_l0_flag = 1;
        p->luma_weight_l1_flag = 1;
        p->chroma_weight_l1_flag = 1;
    } else {
        p->chroma_weight_l0_flag = 1;
        p->chroma_weight_l0[0][0]=1;
        p->chroma_offset_l0[0][0]=0;
        p->chroma_weight_l0[0][1]=1;
        p->chroma_offset_l0[0][1]=0;
        p->luma_weight_l1_flag = 1;
        p->chroma_weight_l1_flag = 1;
        p->luma_weight_l0[0]=0x01;
    }
    for (i = 0; i < 32; i++) {
        p->RefPicList0[i].flags = VA_PICTURE_H264_INVALID;
        p->RefPicList1[i].flags = VA_PICTURE_H264_INVALID;
    }
    p->RefPicList1[0].picture_id = 0xffffffff;
    p->RefPicList0[0].picture_id = 0xffffffff;
}

static void nv12_to_rgba(const VAImage vaImage, rfbClient *client, int ch_x, int ch_y, int ch_w, int ch_h)
{
    DebugLog(("%s: converting region (%d, %d)-(%d, %d) from NV12->RGBA\n", __FUNCTION__, ch_x, ch_y, ch_w, ch_h));

    VAStatus va_status;
    uint8_t *nv12_buf;
    va_status = vaMapBuffer(va_dpy, vaImage.buf, (void **)&nv12_buf);
    CHECK_VASTATUS(va_status, "vaMapBuffer(DecodedData)");

    /* adjust x, y, width, height of the affected area so
     * x, y, width and height are always even.
     */
    if (ch_x % 2) { --ch_x; ++ch_w; }
    if (ch_y % 2) { --ch_y; ++ch_h; }
    if ((ch_x + ch_w) % 2) { ++ch_w; }
    if ((ch_y + ch_h) % 2) { ++ch_h; }

    /* point nv12_buf and dst to upper left corner of changed area */
    uint8_t *nv12_y  = &nv12_buf[vaImage.offsets[0] + vaImage.pitches[0] * ch_y + ch_x];
    uint8_t *nv12_uv = &nv12_buf[vaImage.offsets[1] + vaImage.pitches[1] * (ch_y / 2) + ch_x];
    uint32_t *dst    = &((uint32_t*)client->frameBuffer)[client->width * ch_y + ch_x];

    /* TODO: optimize R, G, B calculation. Possible ways to do this:
     *       - use lookup tables
     *       - convert from floating point to integer arithmetic
     *       - use MMX/SSE to vectorize calculations
     *       - use GPU (VA VPP, shader...)
     */
    int src_x, src_y;
    for (src_y = 0; src_y < ch_h; src_y += 2) {
        for (src_x = 0; src_x < ch_w; src_x += 2) {
            uint8_t nv_u = nv12_uv[src_x];
            uint8_t nv_v = nv12_uv[src_x + 1];
            uint8_t nv_y[4] = { nv12_y[                     src_x], nv12_y[                     src_x + 1],
                                nv12_y[vaImage.pitches[0] + src_x], nv12_y[vaImage.pitches[0] + src_x + 1] };

        int i;
            for (i = 0; i < 4; ++i) {
                double R = 1.164 * (nv_y[i] - 16)                        + 1.596 * (nv_v - 128);
                double G = 1.164 * (nv_y[i] - 16) - 0.391 * (nv_u - 128) - 0.813 * (nv_v - 128);
                double B = 1.164 * (nv_y[i] - 16) + 2.018 * (nv_u - 128);

                /* clamp R, G, B values. For some Y, U, V combinations,
                 * the results of the above calculations fall outside of
                 * the range 0-255.
                 */
                if (R < 0.0) R = 0.0;
                if (G < 0.0) G = 0.0;
                if (B < 0.0) B = 0.0;
                if (R > 255.0) R = 255.0;
                if (G > 255.0) G = 255.0;
                if (B > 255.0) B = 255.0;

                dst[client->width * (i / 2) + src_x + (i % 2)] = 0
                               | ((unsigned int)(R + 0.5) << client->format.redShift)
                               | ((unsigned int)(G + 0.5) << client->format.greenShift)
                               | ((unsigned int)(B + 0.5) << client->format.blueShift);
            }
        }

        nv12_y  += 2 * vaImage.pitches[0];
        nv12_uv += vaImage.pitches[1];
        dst     += 2 * client->width;
    }

    CHECK_SURF(va_surface_id[sid]);
    va_status = vaUnmapBuffer(va_dpy, vaImage.buf);
    CHECK_VASTATUS(va_status, "vaUnmapBuffer(DecodedData)");
}

#endif /* LIBVNCSERVER_CONFIG_LIBVA */

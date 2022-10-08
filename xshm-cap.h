//
// Created by galister on 9/25/22.
//

#ifndef SCREENCAPTEST_XSHM_CAP_H
#define SCREENCAPTEST_XSHM_CAP_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <xcb/randr.h>
#include <xcb/shm.h>
#include <xcb/xfixes.h>
#include <xcb/xinerama.h>

#include "xhelpers.h"

#define TEX_INTERNAL_FORMAT GL_BGRA
#define TEX_EXTERNAL_FORMAT GL_BGR

#ifdef __cplusplus
extern "C" {
#endif

struct xshm_data {
    xcb_connection_t *xcb;
    xcb_screen_t *xcb_screen;
    xcb_shm_t *xshm;

    char *server;
    uint_fast32_t screen_id;
    int_fast32_t x_org;
    int_fast32_t y_org;
    int_fast32_t width;
    int_fast32_t height;

    int_fast32_t cut_top;
    int_fast32_t cut_left;
    int_fast32_t cut_right;
    int_fast32_t cut_bot;

    int_fast32_t adj_x_org;
    int_fast32_t adj_y_org;
    int_fast32_t adj_width;
    int_fast32_t adj_height;

    bool show_cursor;
    bool use_xinerama;
    bool use_randr;
};

struct vec2i_t{
    int32_t x;
    int32_t y;
};

struct xshm_data * xshm_cap_start(int32_t screen);
void xshm_grab_image(struct xshm_data * data);
void xshm_cap_end(struct xshm_data * data);
__int32_t xshm_num_screens();

void xshm_screen_size(int32_t screen, struct vec2i_t *vec);
void xshm_mouse_position(struct xshm_data * data, struct vec2i_t *vec);

uint32_t xshm_grab_bgra32(struct xshm_data * data);
uint8_t * xshm_pixel_buffer(struct xshm_data * data);

void xshm_mouse_move(struct xshm_data * data, int16_t x, int16_t y);
void xshm_mouse_event(struct xshm_data * data, int16_t x, int16_t y, uint8_t button, int32_t pressed);
void xshm_keybd_event(struct xshm_data *data, uint8_t keycode, int32_t pressed);

#ifdef __cplusplus
}
#endif

#endif //SCREENCAPTEST_XSHM_CAP_H

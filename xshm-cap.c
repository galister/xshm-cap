//
// Created by galister on 9/25/22.
//

#include "xshm-cap.h"

bool xshm_check_extensions(xcb_connection_t *xcb)
{
    bool ok = true;

    if (!xcb_get_extension_data(xcb, &xcb_shm_id)->present) {
        printf("Missing SHM extension !");
        ok = false;
    }

    if (!xcb_get_extension_data(xcb, &xcb_xinerama_id)->present)
        printf("Missing Xinerama extension !");

    if (!xcb_get_extension_data(xcb, &xcb_randr_id)->present)
        printf("Missing Randr extension !");

    return ok;
}


void xshm_cap_end(struct xshm_data * data){
    if (!data)
        return;

    if (data->xshm) {
        xshm_xcb_detach(data->xshm);
        data->xshm = NULL;
    }

    if (data->xcb) {
        xcb_disconnect(data->xcb);
        data->xcb = NULL;
    }

    if (data->server) {
        free(data->server);
        data->server = NULL;
    }

    free(data);
}

/**
 * Update the capture
 *
 * @return < 0 on error, 0 when size is unchanged, > 1 on size change
 */
int_fast32_t xshm_update_geometry(struct xshm_data *data)
{
    int_fast32_t prev_width = data->adj_width;
    int_fast32_t prev_height = data->adj_height;

    if (data->use_randr) {
        if (randr_screen_geo(data->xcb, data->screen_id, &data->x_org,
                             &data->y_org, &data->width, &data->height,
                             &data->xcb_screen, NULL) < 0) {
            return -1;
        }
    } else if (data->use_xinerama) {
        if (xinerama_screen_geo(data->xcb, data->screen_id,
                                &data->x_org, &data->y_org,
                                &data->width, &data->height) < 0) {
            return -1;
        }
        data->xcb_screen = xcb_get_screen(data->xcb, 0);
    } else {
        data->x_org = 0;
        data->y_org = 0;
        if (x11_screen_geo(data->xcb, data->screen_id, &data->width,
                           &data->height) < 0) {
            return -1;
        }
        data->xcb_screen = xcb_get_screen(data->xcb, data->screen_id);
    }

    if (!data->width || !data->height) {
        printf("Failed to get geometry");
        return -1;
    }

    data->adj_y_org = data->y_org;
    data->adj_x_org = data->x_org;
    data->adj_height = data->height;
    data->adj_width = data->width;

    if (data->cut_top != 0) {
        if (data->y_org > 0)
            data->adj_y_org = data->y_org + data->cut_top;
        else
            data->adj_y_org = data->cut_top;
        data->adj_height = data->adj_height - data->cut_top;
    }
    if (data->cut_left != 0) {
        if (data->x_org > 0)
            data->adj_x_org = data->x_org + data->cut_left;
        else
            data->adj_x_org = data->cut_left;
        data->adj_width = data->adj_width - data->cut_left;
    }
    if (data->cut_right != 0)
        data->adj_width = data->adj_width - data->cut_right;
    if (data->cut_bot != 0)
        data->adj_height = data->adj_height - data->cut_bot;

    printf(
         "Geometry %" PRIdFAST32 "x%" PRIdFAST32 " @ %" PRIdFAST32
         ",%" PRIdFAST32,
         data->width, data->height, data->x_org, data->y_org);

    if (prev_width == data->adj_width && prev_height == data->adj_height)
        return 0;

    return 1;
}


__int32_t xshm_num_screens()
{
    xcb_connection_t * xcb = xcb_connect(NULL, NULL);
    if (!xcb || xcb_connection_has_error(xcb)) {
        printf("Unable to open X display !");
        return 0;
    }

    int retval = 1;
    if (randr_is_active(xcb))
        retval = randr_screen_count(xcb);
    else if (xinerama_is_active(xcb))
        retval =  xinerama_screen_count(xcb);

    xcb_disconnect(xcb);
    return retval;
}


struct xshm_data * xsh_screen_init(int32_t screen)
{
    struct xshm_data * data = calloc(1, sizeof(struct xshm_data));
    data->screen_id = screen;

    data->xcb = xcb_connect(NULL, NULL);
    if (!data->xcb || xcb_connection_has_error(data->xcb)) {
        printf("Unable to open X display !");
        goto fail;
    }

    if (!xshm_check_extensions(data->xcb))
        goto fail;

    data->use_randr = randr_is_active(data->xcb) ? true : false;
    data->use_xinerama = xinerama_is_active(data->xcb) ? true : false;

    if (xshm_update_geometry(data) < 0) {
        printf("failed to update geometry !");
    }
    return data;

    fail:
    xshm_cap_end(data);
}


void xshm_screen_size(int32_t screen, struct vec2i_t *vec)
{
    struct xshm_data * data = xsh_screen_init(screen);
    vec->x = (int) data->adj_width;
    vec->y = (int) data->adj_height;
}


void xshm_mouse_position(struct xshm_data * data, struct vec2i_t *vec)
{
    xcb_query_pointer_cookie_t xp_c =
            xcb_query_pointer_unchecked(data->xcb, data->xcb_screen->root);
    xcb_query_pointer_reply_t *xp =
            xcb_query_pointer_reply(data->xcb, xp_c, NULL);

    if (!xp)
        return;

    vec->x = (int)(xp->root_x - data->adj_x_org);
    vec->y = (int)(xp->root_y - data->adj_y_org);

    free(xp);
}


struct xshm_data * xshm_cap_start(int32_t screen){
    struct xshm_data * data = xsh_screen_init(screen);

    data->xshm = xshm_xcb_attach(data->xcb, data->adj_width, data->adj_height);
    if (!data->xshm) {
        printf("failed to attach shm !");
        goto fail;
    }

    return data;

    fail:
    xshm_cap_end(data);
}

uint8_t * xshm_pixel_buffer(struct xshm_data * data)
{
    return data->xshm->data;
}

uint32_t xshm_grab_bgra32(struct xshm_data * data)
{
    xcb_shm_get_image_cookie_t img_c;
    xcb_shm_get_image_reply_t *img_r;

    img_c = xcb_shm_get_image_unchecked(data->xcb, data->xcb_screen->root,
                                        data->adj_x_org, data->adj_y_org,
                                        data->adj_width, data->adj_height,
                                        ~0, XCB_IMAGE_FORMAT_Z_PIXMAP,
                                        data->xshm->seg, 0);

    img_r = xcb_shm_get_image_reply(data->xcb, img_c, NULL);

    if (img_r) {
        uint32_t length = img_r->size;
        free(img_r);
        return length;
    }
    return 0;
}

void xshm_mouse_move(struct xshm_data * data, int16_t x, int16_t y)
{
    xcb_warp_pointer(data->xcb, XCB_NONE, data->xcb_screen->root, 0, 0, 0, 0, data->adj_x_org + x, data->adj_y_org + y);
    xcb_flush(data->xcb);
}

void xshm_mouse_event(struct xshm_data * data, int16_t x, int16_t y, uint8_t button, int32_t pressed)
{
    Display *display = XOpenDisplay(NULL);
    XTestFakeButtonEvent(display, button, pressed, CurrentTime);
    XFlush(display);
    XCloseDisplay(display);
}

void xshm_keybd_event(struct xshm_data *data, uint8_t keycode, int32_t pressed)
{
    Display *display = XOpenDisplay(NULL);
    XTestFakeKeyEvent(display, keycode, pressed, CurrentTime);
    XFlush(display);
    XCloseDisplay(display);
}
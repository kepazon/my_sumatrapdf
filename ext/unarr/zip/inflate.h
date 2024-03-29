/* Copyright 2014 the unarr project authors (see AUTHORS file).
   License: LGPLv3 */

#ifndef zip_inflate_h
#define zip_inflate_h

#include <stddef.h>
#include <stdbool.h>

typedef struct inflate_state_s inflate_state;

inflate_state *inflate_create(bool inflate64);
/* updates avail_in and avail_out and returns -1 on EOF or any other non-zero value on error */
int inflate_process(inflate_state *state, const void *data_in, size_t *avail_in, void *data_out, size_t *avail_out);
void inflate_free(inflate_state *state);

#endif

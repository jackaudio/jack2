/*
    Copyright (C) 1999-2000 Paul Davis 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef __jack_memops_h__
#define __jack_memops_h__

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef sun
#define __inline__
#endif

typedef	enum  {
	None,
	Rectangular,
	Triangular,
	Shaped
} DitherAlgorithm;

#define DITHER_BUF_SIZE 8
#define DITHER_BUF_MASK 7

typedef struct {
    unsigned int depth;
    float rm1;
    unsigned int idx;
    float e[DITHER_BUF_SIZE];
} dither_state_t;

/* float functions */
void sample_move_floatLE_sSs (jack_default_audio_sample_t *dst, char *src, unsigned long nsamples, unsigned long dst_skip);
void sample_move_dS_floatLE (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);

/* integer functions */
void sample_move_d32u24_sSs          (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_d32u24_sS           (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_d24_sSs             (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_d24_sS              (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_d16_sSs             (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_d16_sS              (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);

void sample_move_dither_rect_d32u24_sSs   (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_dither_rect_d32u24_sS    (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_dither_tri_d32u24_sSs    (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_dither_tri_d32u24_sS     (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_dither_shaped_d32u24_sSs (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_dither_shaped_d32u24_sS  (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_dither_rect_d24_sSs      (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_dither_rect_d24_sS       (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_dither_tri_d24_sSs       (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_dither_tri_d24_sS        (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_dither_shaped_d24_sSs    (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_dither_shaped_d24_sS     (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_dither_rect_d16_sSs      (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_dither_rect_d16_sS       (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_dither_tri_d16_sSs       (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_dither_tri_d16_sS        (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_dither_shaped_d16_sSs    (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_move_dither_shaped_d16_sS     (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);

void sample_move_dS_s32u24s          (jack_default_audio_sample_t *dst, char *src, unsigned long nsamples, unsigned long src_skip);
void sample_move_dS_s32u24           (jack_default_audio_sample_t *dst, char *src, unsigned long nsamples, unsigned long src_skip);
void sample_move_dS_s24s             (jack_default_audio_sample_t *dst, char *src, unsigned long nsamples, unsigned long src_skip);
void sample_move_dS_s24              (jack_default_audio_sample_t *dst, char *src, unsigned long nsamples, unsigned long src_skip);
void sample_move_dS_s16s             (jack_default_audio_sample_t *dst, char *src, unsigned long nsamples, unsigned long src_skip);
void sample_move_dS_s16              (jack_default_audio_sample_t *dst, char *src, unsigned long nsamples, unsigned long src_skip);

void sample_merge_d16_sS             (char *dst,  jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
void sample_merge_d32u24_sS          (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);

static __inline__ void
sample_merge (jack_default_audio_sample_t *dst, jack_default_audio_sample_t *src, unsigned long cnt)
{
	while (cnt--) {
		*dst += *src;
		dst++;
		src++;
	}
}

static __inline__ void
sample_memcpy (jack_default_audio_sample_t *dst, jack_default_audio_sample_t *src, unsigned long cnt)
{
	memcpy (dst, src, cnt * sizeof (jack_default_audio_sample_t));
}

void memset_interleave               (char *dst, char val, unsigned long bytes, unsigned long unit_bytes, unsigned long skip_bytes);
void memcpy_fake                     (char *dst, char *src, unsigned long src_bytes, unsigned long foo, unsigned long bar);

void memcpy_interleave_d16_s16       (char *dst, char *src, unsigned long src_bytes, unsigned long dst_skip_bytes, unsigned long src_skip_bytes);
void memcpy_interleave_d24_s24       (char *dst, char *src, unsigned long src_bytes, unsigned long dst_skip_bytes, unsigned long src_skip_bytes);
void memcpy_interleave_d32_s32       (char *dst, char *src, unsigned long src_bytes, unsigned long dst_skip_bytes, unsigned long src_skip_bytes);

void merge_memcpy_interleave_d16_s16 (char *dst, char *src, unsigned long src_bytes, unsigned long dst_skip_bytes, unsigned long src_skip_bytes);
void merge_memcpy_interleave_d24_s24 (char *dst, char *src, unsigned long src_bytes, unsigned long dst_skip_bytes, unsigned long src_skip_bytes);
void merge_memcpy_interleave_d32_s32 (char *dst, char *src, unsigned long src_bytes, unsigned long dst_skip_bytes, unsigned long src_skip_bytes);

void merge_memcpy_d16_s16            (char *dst, char *src, unsigned long src_bytes, unsigned long foo, unsigned long bar);
void merge_memcpy_d32_s32            (char *dst, char *src, unsigned long src_bytes, unsigned long foo, unsigned long bar);

#ifdef __cplusplus
}
#endif

#endif /* __jack_memops_h__ */

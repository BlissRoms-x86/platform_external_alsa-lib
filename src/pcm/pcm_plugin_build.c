/*
 *  PCM Plug-In shared (kernel/library) code
 *  Copyright (c) 1999 by Jaroslav Kysela <perex@suse.cz>
 *
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
  
#ifdef __KERNEL__
#define PLUGIN_DEBUG
#include "../include/driver.h"
#include "../include/pcm.h"
typedef snd_pcm_runtime_t PLUGIN_BASE;
#define snd_pcm_plugin_first(pb, channel) ((pb)->oss.plugin_first)
#define snd_pcm_plugin_last(pb, channel) ((pb)->oss.plugin_last)
#define snd_pcm_plugin_append(pb, channel, plugin) snd_pcm_oss_plugin_append(pb, plugin)
#else
#include <errno.h>
#include "pcm_local.h"
typedef snd_pcm_t PLUGIN_BASE;
#endif


ssize_t snd_pcm_plugin_transfer_size(PLUGIN_BASE *pb, int channel, size_t drv_size)
{
	snd_pcm_plugin_t *plugin, *plugin_prev, *plugin_next;
	
	if (!pb || (channel != SND_PCM_CHANNEL_PLAYBACK &&
		    channel != SND_PCM_CHANNEL_CAPTURE))
		return -EINVAL;
	if (drv_size == 0)
		return 0;
	if (drv_size < 0)
		return -EINVAL;
	if (channel == SND_PCM_CHANNEL_PLAYBACK) {
		plugin = snd_pcm_plugin_last(pb, channel);
		while (plugin) {
			plugin_prev = plugin->prev;
			if (plugin->src_size)
				drv_size = plugin->src_size(plugin, drv_size);
			plugin = plugin_prev;
		}
	} else if (channel == SND_PCM_CHANNEL_CAPTURE) {
		plugin = snd_pcm_plugin_first(pb, channel);
		while (plugin) {
			plugin_next = plugin->next;
			if (plugin->dst_size)
				drv_size = plugin->dst_size(plugin, drv_size);
			plugin = plugin_next;
		}
	}
	return drv_size;
}

ssize_t snd_pcm_plugin_hardware_size(PLUGIN_BASE *pb, int channel, size_t trf_size)
{
	snd_pcm_plugin_t *plugin, *plugin_prev, *plugin_next;
	
	if (!pb || (channel != SND_PCM_CHANNEL_PLAYBACK &&
		     channel != SND_PCM_CHANNEL_CAPTURE))
		return -EINVAL;
	if (trf_size == 0)
		return 0;
	if (trf_size < 0)
		return -EINVAL;
	if (channel == SND_PCM_CHANNEL_PLAYBACK) {
		plugin = snd_pcm_plugin_first(pb, channel);
		while (plugin) {
			plugin_next = plugin->next;
			if (plugin->dst_size)
				trf_size = plugin->dst_size(plugin, trf_size);
			plugin = plugin_next;
		}
	} else if (channel == SND_PCM_CHANNEL_CAPTURE) {
		plugin = snd_pcm_plugin_last(pb, channel);
		while (plugin) {
			plugin_prev = plugin->prev;
			if (plugin->src_size)
				trf_size = plugin->src_size(plugin, trf_size);
			plugin = plugin_prev;
		}
	} 
	return trf_size;
}


unsigned int snd_pcm_plugin_formats(unsigned int formats)
{
	formats |= SND_PCM_FMT_MU_LAW;
#ifndef __KERNEL__
	formats |= SND_PCM_FMT_A_LAW | SND_PCM_FMT_IMA_ADPCM;
#endif
	if (formats & (SND_PCM_FMT_U8|SND_PCM_FMT_S8|
		       SND_PCM_FMT_U16_LE|SND_PCM_FMT_S16_LE))
		formats |= SND_PCM_FMT_U8|SND_PCM_FMT_S8|
			   SND_PCM_FMT_U16_LE|SND_PCM_FMT_S16_LE;
	return formats;
}

int snd_pcm_plugin_hwparams(snd_pcm_channel_params_t *params,
			    snd_pcm_channel_info_t *hwinfo,
			    snd_pcm_channel_params_t *hwparams)
{
	memcpy(hwparams, params, sizeof(*hwparams));
	if ((hwinfo->formats & (1 << params->format.format)) == 0) {
		if ((snd_pcm_plugin_formats(hwinfo->formats) & (1 << params->format.format)) == 0)
			return -EINVAL;
		switch (params->format.format) {
		case SND_PCM_SFMT_U8:
			if (hwinfo->formats & SND_PCM_FMT_S8) {
				hwparams->format.format = SND_PCM_SFMT_S8;
			} else if (hwinfo->formats & SND_PCM_FMT_U16_LE) {
				hwparams->format.format = SND_PCM_SFMT_U16_LE;
			} else if (hwinfo->formats & SND_PCM_FMT_S16_LE) {
				hwparams->format.format = SND_PCM_SFMT_S16_LE;
			} else {
				return -EINVAL;
			}
			break;
		case SND_PCM_SFMT_S8:
			if (hwinfo->formats & SND_PCM_FMT_U8) {
				hwparams->format.format = SND_PCM_SFMT_U8;
			} else if (hwinfo->formats & SND_PCM_FMT_S16_LE) {
				hwparams->format.format = SND_PCM_SFMT_S16_LE;
			} else if (hwinfo->formats & SND_PCM_FMT_U16_LE) {
				hwparams->format.format = SND_PCM_SFMT_U16_LE;
			} else {
				return -EINVAL;
			}
			break;
		case SND_PCM_SFMT_S16_LE:
			if (hwinfo->formats & SND_PCM_FMT_U16_LE) {
				hwparams->format.format = SND_PCM_SFMT_U16_LE;
			} else if (hwinfo->formats & SND_PCM_FMT_S8) {
				hwparams->format.format = SND_PCM_SFMT_S8;
			} else if (hwinfo->formats & SND_PCM_FMT_U8) {
				hwparams->format.format = SND_PCM_SFMT_U8;
			} else {
				return -EINVAL;
			}
			break;
		case SND_PCM_SFMT_U16_LE:
			if (hwinfo->formats & SND_PCM_FMT_S16_LE) {
				hwparams->format.format = SND_PCM_SFMT_S16_LE;
			} else if (hwinfo->formats & SND_PCM_FMT_U8) {
				hwparams->format.format = SND_PCM_SFMT_U8;
			} else if (hwinfo->formats & SND_PCM_FMT_S8) {
				hwparams->format.format = SND_PCM_SFMT_S8;
			} else {
				return -EINVAL;
			}
			break;
		case SND_PCM_SFMT_MU_LAW:
			if (hwinfo->formats & SND_PCM_FMT_S16_LE) {
				hwparams->format.format = SND_PCM_SFMT_S16_LE;
			} else if (hwinfo->formats & SND_PCM_FMT_U16_LE) {
				hwparams->format.format = SND_PCM_SFMT_U16_LE;
			} else if (hwinfo->formats & SND_PCM_FMT_S8) {
				hwparams->format.format = SND_PCM_SFMT_S8;
			} else if (hwinfo->formats & SND_PCM_FMT_U8) {
				hwparams->format.format = SND_PCM_SFMT_U8;
			} else {
				return -EINVAL;
			}
			break;
#ifndef __KERNEL__
		case SND_PCM_SFMT_A_LAW:
			if (hwinfo->formats & SND_PCM_FMT_S16_LE) {
				hwparams->format.format = SND_PCM_SFMT_S16_LE;
			} else if (hwinfo->formats & SND_PCM_FMT_U16_LE) {
				hwparams->format.format = SND_PCM_SFMT_U16_LE;
			} else if (hwinfo->formats & SND_PCM_FMT_S8) {
				hwparams->format.format = SND_PCM_SFMT_S8;
			} else if (hwinfo->formats & SND_PCM_FMT_U8) {
				hwparams->format.format = SND_PCM_SFMT_U8;
			} else {
				return -EINVAL;
			}
		case SND_PCM_SFMT_IMA_ADPCM:
			if (hwinfo->formats & SND_PCM_FMT_S16_LE) {
				hwparams->format.format = SND_PCM_SFMT_S16_LE;
			} else if (hwinfo->formats & SND_PCM_FMT_U16_LE) {
				hwparams->format.format = SND_PCM_SFMT_U16_LE;
			} else if (hwinfo->formats & SND_PCM_FMT_S8) {
				hwparams->format.format = SND_PCM_SFMT_S8;
			} else if (hwinfo->formats & SND_PCM_FMT_U8) {
				hwparams->format.format = SND_PCM_SFMT_U8;
			} else {
				return -EINVAL;
			}
			break;
#endif
		default:
			return -EINVAL;
		}
	}

	/* voices */
      	if (params->format.voices < hwinfo->min_voices ||
      	    params->format.voices > hwinfo->max_voices) {
		int dst_voices = params->format.voices < hwinfo->min_voices ?
				 hwinfo->min_voices : hwinfo->max_voices;
		if ((params->format.rate < hwinfo->min_rate ||
		     params->format.rate > hwinfo->max_rate) &&
		    dst_voices > 2)
			dst_voices = 2;
		hwparams->format.voices = dst_voices;
	}

	/* rate */
        if (params->format.rate < hwinfo->min_rate ||
            params->format.rate > hwinfo->max_rate) {
        	int dst_rate = params->format.rate < hwinfo->min_rate ?
        		       hwinfo->min_rate : hwinfo->max_rate;
		hwparams->format.rate = dst_rate;
	}

	/* interleave */
	if (!(hwinfo->flags & SND_PCM_CHNINFO_INTERLEAVE))
		hwparams->format.interleave = 0;
	if (!(hwinfo->flags & SND_PCM_CHNINFO_NONINTERLEAVE))
		hwparams->format.interleave = 1;
	return 0;
}


int snd_pcm_plugin_format(PLUGIN_BASE *pb, 
			  snd_pcm_channel_params_t *params, 
			  snd_pcm_channel_params_t *hwparams,
			  snd_pcm_channel_info_t *hwinfo,
			  snd_pcm_channel_params_t *newparams)
{
	snd_pcm_channel_params_t tmpparams;
	snd_pcm_channel_params_t *dstparams;
	snd_pcm_plugin_t *plugin;
	int err;

	if (params->channel == SND_PCM_CHANNEL_PLAYBACK) {
		memcpy(newparams, params, sizeof(*params));
		memcpy(&tmpparams, params, sizeof(*params));
		dstparams = hwparams;
	} else {
		memcpy(newparams, hwparams, sizeof(*hwparams));
		memcpy(&tmpparams, hwparams, sizeof(*hwparams));
		dstparams = params;
	}

	pdprintf("newparams: interleave=%i, format=%i, rate=%i, voices=%i\n", 
		 newparams->format.interleave,
		 newparams->format.format,
		 newparams->format.rate,
		 newparams->format.voices);
	pdprintf("dstparams: interleave=%i, format=%i, rate=%i, voices=%i\n", 
		 dstparams->format.interleave,
		 dstparams->format.format,
		 dstparams->format.rate,
		 dstparams->format.voices);
	/* Convert to interleaved format if needed */
	if (!newparams->format.interleave &&
	    (newparams->format.voices != dstparams->format.voices ||
	     (newparams->format.rate != dstparams->format.rate &&
	      newparams->format.voices > 1))) {
		tmpparams.format.interleave = 1;
		err = snd_pcm_plugin_build_interleave(&newparams->format,
						      &tmpparams.format,
						      &plugin);
		pdprintf("params interleave change: src=%i, dst=%i returns %i\n", newparams->format.interleave, tmpparams.format.interleave, err);
		if (err < 0) {
			snd_pcm_plugin_free(plugin);
			return err;
		}      					    
		err = snd_pcm_plugin_append(pb, params->channel, plugin);
		if (err < 0) {
			snd_pcm_plugin_free(plugin);
			return err;
		}
		newparams->format.interleave = 1;
		/* Avoid useless interleave revert */
		if (params->channel == SND_PCM_CHANNEL_PLAYBACK &&
		    (hwinfo->flags & SND_PCM_CHNINFO_INTERLEAVE))
			dstparams->format.interleave = 1;
      	}

	/* voices reduction */
	if (newparams->format.voices > dstparams->format.voices) {
		tmpparams.format.voices = dstparams->format.voices;
		err = snd_pcm_plugin_build_voices(&newparams->format,
						  &tmpparams.format,
						  &plugin);
		pdprintf("params voices reduction: src=%i, dst=%i returns %i\n", newparams->format.voices, tmpparams.format.voices, err);
		if (err < 0) {
			snd_pcm_plugin_free(plugin);
			return err;
		}
		err = snd_pcm_plugin_append(pb, params->channel, plugin);
		if (err < 0) {
			snd_pcm_plugin_free(plugin);
			return err;
		}
		newparams->format.voices = dstparams->format.voices;
        }

	/* rate down resampling */
        if (newparams->format.rate > dstparams->format.rate &&
	    snd_pcm_format_linear(newparams->format.format) &&
	    snd_pcm_format_width(newparams->format.format) <= 16 &&
	    snd_pcm_format_width(newparams->format.format) >= snd_pcm_format_width(newparams->format.format)) {
		tmpparams.format.rate = dstparams->format.rate;
        	err = snd_pcm_plugin_build_rate(&newparams->format,
						&tmpparams.format,
						&plugin);
		pdprintf("params rate down resampling: src=%i, dst=%i returns %i\n", newparams->format.rate, tmpparams.format.rate, err);
		if (err < 0) {
			snd_pcm_plugin_free(plugin);
			return err;
		}      					    
		err = snd_pcm_plugin_append(pb, params->channel, plugin);
		if (err < 0) {
			snd_pcm_plugin_free(plugin);
			return err;
		}
		newparams->format.rate = dstparams->format.rate;
        }

	/* format change */
	if (newparams->format.format != dstparams->format.format) {
		tmpparams.format.format = dstparams->format.format;
		switch (newparams->format.format) {
		case SND_PCM_SFMT_MU_LAW:
			err = snd_pcm_plugin_build_mulaw(&newparams->format,
							 &tmpparams.format,
							 &plugin);
			break;
#ifndef __KERNEL__
		case SND_PCM_SFMT_A_LAW:
			err = snd_pcm_plugin_build_alaw(&newparams->format,
							&tmpparams.format,
							&plugin);
			break;
		case SND_PCM_SFMT_IMA_ADPCM:
			err = snd_pcm_plugin_build_adpcm(&newparams->format,
							 &tmpparams.format,
							 &plugin);
			break;
#endif
		default:
			err = snd_pcm_plugin_build_linear(&newparams->format,
							  &tmpparams.format,
							  &plugin);
		}
		pdprintf("params format change: src=%i, dst=%i returns %i\n", newparams->format.format, tmpparams.format.format, err);
		if (err < 0)
			return err;
		err = snd_pcm_plugin_append(pb, params->channel, plugin);
		if (err < 0) {
			snd_pcm_plugin_free(plugin);
			return err;
		}
		newparams->format.format = dstparams->format.format;
	}

	/* rate resampling */
        if (newparams->format.rate != dstparams->format.rate) {
		tmpparams.format.rate = dstparams->format.rate;
        	err = snd_pcm_plugin_build_rate(&newparams->format,
						&tmpparams.format,
						&plugin);
		pdprintf("params rate resampling: src=%i, dst=%i return %i\n", newparams->format.rate, tmpparams.format.rate, err);
		if (err < 0) {
			snd_pcm_plugin_free(plugin);
			return err;
		}      					    
		err = snd_pcm_plugin_append(pb, params->channel, plugin);
		if (err < 0) {
			snd_pcm_plugin_free(plugin);
			return err;
		}
		newparams->format.rate = dstparams->format.rate;
        }
      
	/* voices extension  */
	if (newparams->format.voices != dstparams->format.voices) {
		tmpparams.format.voices = dstparams->format.voices;
		err = snd_pcm_plugin_build_voices(&newparams->format,
						  &tmpparams.format,
						  &plugin);
		pdprintf("params voices extension: src=%i, dst=%i returns %i\n", newparams->format.voices, tmpparams.format.voices, err);
		if (err < 0) {
			snd_pcm_plugin_free(plugin);
			return err;
		}      					    
		err = snd_pcm_plugin_append(pb, params->channel, plugin);
		if (err < 0) {
			snd_pcm_plugin_free(plugin);
			return err;
		}
		newparams->format.voices = dstparams->format.voices;
	}

	/* interleave change */
	if (dstparams->format.voices > 1 && 
	    hwinfo->mode == SND_PCM_MODE_BLOCK &&
	    newparams->format.interleave != dstparams->format.interleave) {
		tmpparams.format.interleave = dstparams->format.interleave;
		err = snd_pcm_plugin_build_interleave(&newparams->format,
						      &tmpparams.format,
						      &plugin);
		pdprintf("params interleave change: src=%i, dst=%i return %i\n", newparams->format.interleave, tmpparams.format.interleave, err);
		if (err < 0) {
			snd_pcm_plugin_free(plugin);
			return err;
		}      					    
		err = snd_pcm_plugin_append(pb, params->channel, plugin);
		if (err < 0) {
			snd_pcm_plugin_free(plugin);
			return err;
		}
		newparams->format.interleave = dstparams->format.interleave;
	}
	return 0;
}



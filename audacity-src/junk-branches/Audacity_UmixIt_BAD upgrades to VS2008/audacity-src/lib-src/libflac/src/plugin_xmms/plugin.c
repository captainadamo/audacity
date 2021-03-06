/* libxmms-flac - XMMS FLAC input plugin
 * Copyright (C) 2000,2001,2002  Josh Coalson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>

#include <xmms/plugin.h>
#include <xmms/util.h>
#include <xmms/configfile.h>
#include <xmms/titlestring.h>

#ifdef HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif

#include "FLAC/all.h"
#include "plugin_common/all.h"
#include "configure.h"
#include "wrap_id3.h"
#include "charset.h"

#ifdef min
#undef min
#endif
#define min(x,y) ((x)<(y)?(x):(y))

#define FLAC__DO_DITHER

typedef struct {
	FLAC__bool abort_flag;
	FLAC__bool is_playing;
	FLAC__bool eof;
	FLAC__bool play_thread_open; /* if true, is_playing must also be true */
	unsigned total_samples;
	unsigned bits_per_sample;
	unsigned channels;
	unsigned sample_rate;
	unsigned length_in_msec;
	AFormat sample_format;
	int seek_to_in_sec;
} file_info_struct;

static void FLAC_XMMS__init();
static int  FLAC_XMMS__is_our_file(char *filename);
static void FLAC_XMMS__play_file(char *filename);
static void FLAC_XMMS__stop();
static void FLAC_XMMS__pause(short p);
static void FLAC_XMMS__seek(int time);
static int  FLAC_XMMS__get_time();
static void FLAC_XMMS__cleanup();
static void FLAC_XMMS__get_song_info(char *filename, char **title, int *length);

static void *play_loop_(void *arg);
static FLAC__bool safe_decoder_init_(const char *filename, FLAC__FileDecoder *decoder);
static void safe_decoder_finish_(FLAC__FileDecoder *decoder);
static void safe_decoder_delete_(FLAC__FileDecoder *decoder);
static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void metadata_callback_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void error_callback_(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

InputPlugin flac_ip =
{
	NULL,
	NULL,
	"Reference FLAC Player v" VERSION,
	FLAC_XMMS__init,
	FLAC_XMMS__aboutbox,
	FLAC_XMMS__configure,
	FLAC_XMMS__is_our_file,
	NULL,
	FLAC_XMMS__play_file,
	FLAC_XMMS__stop,
	FLAC_XMMS__pause,
	FLAC_XMMS__seek,
	NULL,
	FLAC_XMMS__get_time,
	NULL,
	NULL,
	FLAC_XMMS__cleanup,
	NULL,
	NULL,
	NULL,
	NULL,
	FLAC_XMMS__get_song_info,
	NULL,		/* file_info_box */
	NULL
};

#define SAMPLES_PER_WRITE 512
static FLAC__int32 reservoir_[FLAC__MAX_BLOCK_SIZE * 2/*for overflow*/ * FLAC_PLUGIN__MAX_SUPPORTED_CHANNELS];
static FLAC__byte sample_buffer_[SAMPLES_PER_WRITE * FLAC_PLUGIN__MAX_SUPPORTED_CHANNELS * (24/8)]; /* (24/8) for max bytes per sample */
static unsigned wide_samples_in_reservoir_ = 0;
static FLAC__FileDecoder *decoder_ = 0;
static file_info_struct file_info_;
static pthread_t decode_thread_;
static FLAC__bool audio_error_ = false;

InputPlugin *get_iplugin_info()
{
	flac_ip.description = g_strdup_printf("Reference FLAC Player v%s", FLAC__VERSION_STRING);
	return &flac_ip;
}

void FLAC_XMMS__init()
{
	ConfigFile *cfg;

	flac_cfg.tag_override = FALSE;
	g_free(flac_cfg.tag_format);
	flac_cfg.convert_char_set = FALSE;

	cfg = xmms_cfg_open_default_file();

	xmms_cfg_read_boolean(cfg, "flac", "tag_override", &flac_cfg.tag_override);

	if(!xmms_cfg_read_string(cfg, "flac", "tag_format", &flac_cfg.tag_format))
		flac_cfg.tag_format = g_strdup("%p - %t");

	xmms_cfg_read_boolean(cfg, "flac", "convert_char_set", &flac_cfg.convert_char_set);

	if(!xmms_cfg_read_string(cfg, "flac", "file_char_set", &flac_cfg.file_char_set))
		flac_cfg.file_char_set = FLAC_plugin__charset_get_current();

	if(!xmms_cfg_read_string(cfg, "flac", "user_char_set", &flac_cfg.user_char_set))
		flac_cfg.user_char_set = FLAC_plugin__charset_get_current();

	decoder_ = FLAC__file_decoder_new();
}

int FLAC_XMMS__is_our_file(char *filename)
{
	char *ext;

	ext = strrchr(filename, '.');
	if(ext)
		if(!strcasecmp(ext, ".flac") || !strcasecmp(ext, ".fla"))
			return 1;
	return 0;
}

void FLAC_XMMS__play_file(char *filename)
{
	FILE *f;
	gchar *ret;

	wide_samples_in_reservoir_ = 0;
	audio_error_ = false;
	file_info_.abort_flag = false;
	file_info_.is_playing = false;
	file_info_.eof = false;
	file_info_.play_thread_open = false;

	if(0 == (f = fopen(filename, "r")))
		return;
	fclose(f);

	if(decoder_ == 0)
		return;

	if(!safe_decoder_init_(filename, decoder_))
		return;

	file_info_.is_playing = true;

	if(flac_ip.output->open_audio(file_info_.sample_format, file_info_.sample_rate, file_info_.channels) == 0) {
		audio_error_ = true;
		safe_decoder_finish_(decoder_);
		return;
	}

	ret = flac_format_song_title(filename);
	flac_ip.set_info(ret, file_info_.length_in_msec, file_info_.sample_rate * file_info_.channels * file_info_.bits_per_sample, file_info_.sample_rate, file_info_.channels);

	g_free(ret);

	file_info_.seek_to_in_sec = -1;
	file_info_.play_thread_open = true;
	pthread_create(&decode_thread_, NULL, play_loop_, NULL);
}

void FLAC_XMMS__stop()
{
	if(file_info_.is_playing) {
		file_info_.is_playing = false;
		if(file_info_.play_thread_open) {
			file_info_.play_thread_open = false;
			pthread_join(decode_thread_, NULL);
		}
		flac_ip.output->close_audio();
		safe_decoder_finish_(decoder_);
	}
}

void FLAC_XMMS__pause(short p)
{
	flac_ip.output->pause(p);
}

void FLAC_XMMS__seek(int time)
{
	file_info_.seek_to_in_sec = time;
	file_info_.eof = false;

	while(file_info_.seek_to_in_sec != -1)
		xmms_usleep(10000);
}

int FLAC_XMMS__get_time()
{
	if(audio_error_)
		return -2;
	if(!file_info_.is_playing || (file_info_.eof && !flac_ip.output->buffer_playing()))
		return -1;
	else
		return flac_ip.output->output_time();
}

void FLAC_XMMS__cleanup()
{
	safe_decoder_delete_(decoder_);
	decoder_ = 0;
}

void FLAC_XMMS__get_song_info(char *filename, char **title, int *length_in_msec)
{
	FLAC__StreamMetadata streaminfo;

	if(0 == filename)
		filename = "";

	if(!FLAC__metadata_get_streaminfo(filename, &streaminfo)) {
		/* @@@ how to report the error? */
		if(title) {
			static const char *errtitle = "Invalid FLAC File: ";
			*title = g_malloc(strlen(errtitle) + 1 + strlen(filename) + 1 + 1);
			sprintf(*title, "%s\"%s\"", errtitle, filename);
		}
		if(length_in_msec)
			*length_in_msec = -1;
		return;
	}

	if(title) {
		*title = flac_format_song_title(filename);
	}
	if(length_in_msec)
		*length_in_msec = streaminfo.data.stream_info.total_samples * 10 / (streaminfo.data.stream_info.sample_rate / 100);
}

/***********************************************************************
 * local routines
 **********************************************************************/

void *play_loop_(void *arg)
{
	(void)arg;

	while(file_info_.is_playing) {
		if(!file_info_.eof) {
			while(wide_samples_in_reservoir_ < SAMPLES_PER_WRITE) {
				if(FLAC__file_decoder_get_state(decoder_) == FLAC__FILE_DECODER_END_OF_FILE) {
					file_info_.eof = true;
					break;
				}
				else if(!FLAC__file_decoder_process_single(decoder_)) {
					/*@@@ this should probably be a dialog */
					fprintf(stderr, "libxmms-flac: READ ERROR processing frame\n");
					file_info_.eof = true;
					break;
				}
			}
			if(wide_samples_in_reservoir_ > 0) {
				const unsigned channels = file_info_.channels;
				const unsigned bits_per_sample = file_info_.bits_per_sample;
#ifdef FLAC__DO_DITHER
				const unsigned target_bps = min(bits_per_sample, 16);
#else
				const unsigned target_bps = bits_per_sample;
#endif
				const unsigned n = min(wide_samples_in_reservoir_, SAMPLES_PER_WRITE);
				const unsigned delta = n * channels;
				int bytes = (int)FLAC__plugin_common__pack_pcm_signed_little_endian(sample_buffer_, reservoir_, n, channels, bits_per_sample, target_bps);
				unsigned i;

				for(i = delta; i < wide_samples_in_reservoir_ * channels; i++)
					reservoir_[i-delta] = reservoir_[i];
				wide_samples_in_reservoir_ -= n;

				flac_ip.add_vis_pcm(flac_ip.output->written_time(), file_info_.sample_format, channels, bytes, sample_buffer_);
				while(flac_ip.output->buffer_free() < (int)bytes && file_info_.is_playing && file_info_.seek_to_in_sec == -1)
					xmms_usleep(10000);
				if(file_info_.is_playing && file_info_.seek_to_in_sec == -1)
					flac_ip.output->write_audio(sample_buffer_, bytes);
			}
			else {
				file_info_.eof = true;
				xmms_usleep(10000);
			}
		}
		else
			xmms_usleep(10000);
		if(file_info_.seek_to_in_sec != -1) {
			const double distance = (double)file_info_.seek_to_in_sec * 1000.0 / (double)file_info_.length_in_msec;
			unsigned target_sample = (unsigned)(distance * (double)file_info_.total_samples);
			if(FLAC__file_decoder_seek_absolute(decoder_, (FLAC__uint64)target_sample)) {
				flac_ip.output->flush(file_info_.seek_to_in_sec * 1000);
				file_info_.seek_to_in_sec = -1;
				file_info_.eof = false;
				wide_samples_in_reservoir_ = 0;
			}
		}
	}

	safe_decoder_finish_(decoder_);

	/* are these two calls necessary? */
	flac_ip.output->buffer_free();
	flac_ip.output->buffer_free();

	pthread_exit(NULL);
	return 0; /* to silence the compiler warning about not returning a value */
}

FLAC__bool safe_decoder_init_(const char *filename, FLAC__FileDecoder *decoder)
{
	if(decoder == 0)
		return false;

	safe_decoder_finish_(decoder);

	FLAC__file_decoder_set_md5_checking(decoder, false);
	FLAC__file_decoder_set_filename(decoder, filename);
	FLAC__file_decoder_set_write_callback(decoder, write_callback_);
	FLAC__file_decoder_set_metadata_callback(decoder, metadata_callback_);
	FLAC__file_decoder_set_error_callback(decoder, error_callback_);
	FLAC__file_decoder_set_client_data(decoder, &file_info_);
	if(FLAC__file_decoder_init(decoder) != FLAC__FILE_DECODER_OK)
		return false;

	if(!FLAC__file_decoder_process_until_end_of_metadata(decoder))
		return false;

	return true;
}

void safe_decoder_finish_(FLAC__FileDecoder *decoder)
{
	if(decoder && FLAC__file_decoder_get_state(decoder) != FLAC__FILE_DECODER_UNINITIALIZED)
		FLAC__file_decoder_finish(decoder);
}

void safe_decoder_delete_(FLAC__FileDecoder *decoder)
{
	if(decoder) {
		safe_decoder_finish_(decoder);
		FLAC__file_decoder_delete(decoder);
	}
}

FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	file_info_struct *file_info = (file_info_struct *)client_data;
	const unsigned channels = file_info->channels, wide_samples = frame->header.blocksize;
	unsigned wide_sample, offset_sample, channel;

	(void)decoder;

	if(file_info->abort_flag)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	for(offset_sample = wide_samples_in_reservoir_ * channels, wide_sample = 0; wide_sample < wide_samples; wide_sample++)
		for(channel = 0; channel < channels; channel++, offset_sample++)
			reservoir_[offset_sample] = buffer[channel][wide_sample];

	wide_samples_in_reservoir_ += wide_samples;

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void metadata_callback_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	file_info_struct *file_info = (file_info_struct *)client_data;
	(void)decoder;
	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		FLAC__ASSERT(metadata->data.stream_info.total_samples < 0x100000000); /* this plugin can only handle < 4 gigasamples */
		file_info->total_samples = (unsigned)(metadata->data.stream_info.total_samples&0xffffffff);
		file_info->bits_per_sample = metadata->data.stream_info.bits_per_sample;
		file_info->channels = metadata->data.stream_info.channels;
		file_info->sample_rate = metadata->data.stream_info.sample_rate;

#ifdef FLAC__DO_DITHER
		if(file_info->bits_per_sample == 8) {
			file_info->sample_format = FMT_S8;
		}
		else if(file_info->bits_per_sample == 16 || file_info->bits_per_sample == 24) {
			file_info->sample_format = FMT_S16_LE;
		}
		else {
			/*@@@ need some error here like wa2: MessageBox(mod_.hMainWindow, "ERROR: plugin can only handle 8/16/24-bit samples\n", "ERROR: plugin can only handle 8/16/24-bit samples", 0); */
			file_info->abort_flag = true;
			return;
		}
#else
		if(file_info->bits_per_sample == 8) {
			file_info->sample_format = FMT_S8;
		}
		else if(file_info->bits_per_sample == 16) {
			file_info->sample_format = FMT_S16_LE;
		}
		else {
			/*@@@ need some error here like wa2: MessageBox(mod_.hMainWindow, "ERROR: plugin can only handle 8/16-bit samples\n", "ERROR: plugin can only handle 8/16-bit samples", 0); */
			file_info->abort_flag = true;
			return;
		}
#endif
		file_info->length_in_msec = file_info->total_samples * 10 / (file_info->sample_rate / 100);
	}
}

void error_callback_(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	file_info_struct *file_info = (file_info_struct *)client_data;
	(void)decoder;
	if(status != FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC)
		file_info->abort_flag = true;
}

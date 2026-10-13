/*
**	Command & Conquer Generals(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// FILE: BinkVideoPlayer.cpp
// Bink playback via FFmpeg (libavformat bink demuxer + libavcodec bink decoder).

#include "Lib/BaseType.h"
#include "VideoDevice/Bink/BinkVideoPlayer.h"
#include "Common/AudioAffect.h"
#include "Common/GameAudio.h"
#include "Common/GameMemory.h"
#include "Common/GlobalData.h"
#include "Common/Registry.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>

#if HAS_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
}
#endif

#define VIDEO_LANG_PATH_FORMAT "Data/%s/Movies/%s.%s"
#define VIDEO_PATH	"Data\\Movies"
#define VIDEO_EXT		"bik"

// Opaque FFmpeg playback context (one per open stream)
struct BinkFFmpegCtx
{
#if HAS_FFMPEG
	AVFormatContext *fmt;
	AVCodecContext *dec;
	AVFrame *frame;
	AVPacket *pkt;
	SwsContext *sws;
	int videoIdx;
	int64_t frameNumber;      // next frame to present (0-based)
	int64_t startTickMs;      // wall clock at open/first ready
	int swsSrcW, swsSrcH;
	enum AVPixelFormat swsSrcFmt;
	uint8_t *rgb;             // BGRA/XRGB8888 staging buffer
	int rgbPitch;
#endif
	BinkFFmpegCtx() :
#if HAS_FFMPEG
		fmt(NULL), dec(NULL), frame(NULL), pkt(NULL), sws(NULL),
		videoIdx(-1), frameNumber(0), startTickMs(0),
		swsSrcW(0), swsSrcH(0), swsSrcFmt(AV_PIX_FMT_NONE),
		rgb(NULL), rgbPitch(0)
#endif
	{
	}
};

#if HAS_FFMPEG
static double ctxFps( BinkFFmpegCtx *ff )
{
	AVRational fr = ff->fmt->streams[ff->videoIdx]->avg_frame_rate;
	if (fr.num <= 0 || fr.den <= 0)
		fr = ff->fmt->streams[ff->videoIdx]->r_frame_rate;
	if (fr.num <= 0 || fr.den <= 0)
		return 30.0;
	return av_q2d(fr);
}

static int64_t ctxFrameCount( BinkFFmpegCtx *ff )
{
	AVStream *st = ff->fmt->streams[ff->videoIdx];
	if (st->nb_frames > 0)
		return st->nb_frames;
	// Estimate from duration
	if (st->duration > 0 && st->time_base.num > 0 && st->time_base.den > 0) {
		double fps = ctxFps(ff);
		double sec = (double)st->duration * av_q2d(st->time_base);
		int64_t n = (int64_t)(sec * fps + 0.5);
		if (n > 0) return n;
	}
	return 0;
}

static void ctxSeekFrame( BinkFFmpegCtx *ff, int64_t frame )
{
	AVStream *st = ff->fmt->streams[ff->videoIdx];
	double fps = ctxFps(ff);
	int64_t ts = (int64_t)(frame / fps / av_q2d(st->time_base));
	av_seek_frame(ff->fmt, ff->videoIdx, ts, AVSEEK_FLAG_BACKWARD);
	avcodec_flush_buffers(ff->dec);
	av_frame_unref(ff->frame);
}

// Decode forward until the decoded frame corresponds to ff->frameNumber.
// Bink: one video packet ≈ one frame.
static void ctxDecodeToFrame( BinkFFmpegCtx *ff )
{
	// Fast path: already holding the wanted frame
	// We track by re-decoding from last keyframe only when frameNumber jumps backward.
	for (;;)
	{
		int got = 0;
		while (av_read_frame(ff->fmt, ff->pkt) >= 0)
		{
			if (ff->pkt->stream_index != ff->videoIdx)
			{
				av_packet_unref(ff->pkt);
				continue;
			}
			int ret = avcodec_send_packet(ff->dec, ff->pkt);
			av_packet_unref(ff->pkt);
			if (ret < 0)
				continue;
			ret = avcodec_receive_frame(ff->dec, ff->frame);
			if (ret == 0)
			{
				got = 1;
				break;
			}
			if (ret == AVERROR(EAGAIN))
				continue;
			if (ret == AVERROR_EOF)
				return;
		}
		if (!got)
			return; // EOF or read error — leave last frame in place
	}
}

static bool ctxEnsureRgb( BinkFFmpegCtx *ff )
{
	if (!ff->frame->data[0])
		return false;
	if (!ff->rgb || ff->swsSrcW != ff->frame->width || ff->swsSrcH != ff->frame->height ||
		ff->swsSrcFmt != (enum AVPixelFormat)ff->frame->format)
	{
		if (ff->sws)
		{
			sws_freeContext(ff->sws);
			ff->sws = NULL;
		}
		if (ff->rgb)
		{
			av_free(ff->rgb);
			ff->rgb = NULL;
		}
		ff->swsSrcW = ff->frame->width;
		ff->swsSrcH = ff->frame->height;
		ff->swsSrcFmt = (enum AVPixelFormat)ff->frame->format;
		ff->rgbPitch = ff->swsSrcW * 4;
		ff->rgb = (uint8_t*)av_malloc((size_t)ff->rgbPitch * ff->swsSrcH);
		if (!ff->rgb)
			return false;
		ff->sws = sws_getContext(
			ff->swsSrcW, ff->swsSrcH, ff->swsSrcFmt,
			ff->swsSrcW, ff->swsSrcH, AV_PIX_FMT_BGR0,
			SWS_BILINEAR, NULL, NULL, NULL);
		if (!ff->sws)
			return false;
	}
	uint8_t *dst[4] = { ff->rgb, NULL, NULL, NULL };
	int dstStride[4] = { ff->rgbPitch, 0, 0, 0 };
	sws_scale(ff->sws, ff->frame->data, ff->frame->linesize, 0, ff->swsSrcH, dst, dstStride);
	return true;
}

static BinkFFmpegCtx *ctxOpen( const char *path )
{
	BinkFFmpegCtx *ff = new BinkFFmpegCtx;
	avformat_open_input(&ff->fmt, path, NULL, NULL);
	if (!ff->fmt)
	{
		delete ff;
		return NULL;
	}
	if (avformat_find_stream_info(ff->fmt, NULL) < 0)
	{
		avformat_close_input(&ff->fmt);
		delete ff;
		return NULL;
	}
	const AVCodec *codec = NULL;
	ff->videoIdx = av_find_best_stream(ff->fmt, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
	if (ff->videoIdx < 0 || !codec)
	{
		avformat_close_input(&ff->fmt);
		delete ff;
		return NULL;
	}
	ff->dec = avcodec_alloc_context3(codec);
	if (!ff->dec || avcodec_open2(ff->dec, codec, NULL) < 0)
	{
		if (ff->dec) avcodec_free_context(&ff->dec);
		avformat_close_input(&ff->fmt);
		delete ff;
		return NULL;
	}
	ff->frame = av_frame_alloc();
	ff->pkt = av_packet_alloc();
	if (!ff->frame || !ff->pkt)
	{
		if (ff->frame) av_frame_free(&ff->frame);
		if (ff->pkt) av_packet_free(&ff->pkt);
		avcodec_free_context(&ff->dec);
		avformat_close_input(&ff->fmt);
		delete ff;
		return NULL;
	}
	ff->frameNumber = 0;
	ff->startTickMs = (int64_t)GetTickCount();
	return ff;
}

static void ctxClose( BinkFFmpegCtx *ff )
{
	if (!ff) return;
	if (ff->sws) { sws_freeContext(ff->sws); ff->sws = NULL; }
	if (ff->rgb) { av_free(ff->rgb); ff->rgb = NULL; }
	if (ff->pkt) av_packet_free(&ff->pkt);
	if (ff->frame) av_frame_free(&ff->frame);
	if (ff->dec) avcodec_free_context(&ff->dec);
	if (ff->fmt) avformat_close_input(&ff->fmt);
	delete ff;
}
#endif // HAS_FFMPEG

//============================================================================
// BinkVideoStream
//============================================================================

BinkVideoStream::BinkVideoStream()
: m_ff(NULL), m_memFile(NULL)
{
}

BinkVideoStream::~BinkVideoStream()
{
	freeFF();
}

void BinkVideoStream::freeFF( void )
{
#if HAS_FFMPEG
	if (m_ff)
	{
		ctxClose(m_ff);
		m_ff = NULL;
	}
#endif
}

void BinkVideoStream::update( void )
{
}

Bool BinkVideoStream::isFrameReady( void )
{
#if HAS_FFMPEG
	if (!m_ff) return FALSE;
	int64_t count = ctxFrameCount(m_ff);
	// Always ready for frame 0 / unknown length; then rate-limit by wall clock.
	if (m_ff->frameNumber <= 0 || count <= 0)
		return TRUE;
	double fps = ctxFps(m_ff);
	if (fps <= 0.0) return TRUE;
	int64_t elapsed = (int64_t)GetTickCount() - m_ff->startTickMs;
	int64_t dueMs = (int64_t)(m_ff->frameNumber * 1000.0 / fps);
	return elapsed >= dueMs;
#else
	return TRUE;
#endif
}

void BinkVideoStream::frameDecompress( void )
{
#if HAS_FFMPEG
	if (!m_ff) return;
	// If the caller jumped (frameGoto), seek then decode one frame.
	// Sequential playback: decode one packet per call.
	ctxDecodeToFrame(m_ff);
#endif
}

void BinkVideoStream::frameRender( VideoBuffer *buffer )
{
#if HAS_FFMPEG
	if (!m_ff || !buffer || !m_ff->frame || !m_ff->frame->data[0])
		return;
	if (!ctxEnsureRgb(m_ff))
		return;

	void *mem = buffer->lock();
	if (!mem)
		return;

	UnsignedInt bw = buffer->width();
	UnsignedInt bh = buffer->height();
	UnsignedInt bpt = buffer->pitch();
	int copyW = (int)((bw < (UnsignedInt)m_ff->swsSrcW) ? bw : (UnsignedInt)m_ff->swsSrcW);
	int copyH = (int)((bh < (UnsignedInt)m_ff->swsSrcH) ? bh : (UnsignedInt)m_ff->swsSrcH);

	// VideoBuffer formats (see VideoBuffer::Type)
	switch (buffer->format())
	{
		case VideoBuffer::TYPE_X8R8G8B8:
		case VideoBuffer::TYPE_R8G8B8:
		{
			// BGRA (BINK_BGR0) → X8R8G8B8 / R8G8B8
			for (int y = 0; y < copyH; ++y)
			{
				const uint8_t *src = m_ff->rgb + (size_t)y * m_ff->rgbPitch;
				uint8_t *dst = (uint8_t *)mem + (size_t)y * bpt;
				if (buffer->format() == VideoBuffer::TYPE_X8R8G8B8)
				{
					memcpy(dst, src, (size_t)copyW * 4);
				}
				else // R8G8B8: drop X
				{
					for (int x = 0; x < copyW; ++x)
					{
						dst[x * 3 + 0] = src[x * 4 + 2]; // R
						dst[x * 3 + 1] = src[x * 4 + 1]; // G
						dst[x * 3 + 2] = src[x * 4 + 0]; // B
					}
				}
			}
			break;
		}
		case VideoBuffer::TYPE_R5G6B5:
		case VideoBuffer::TYPE_X1R5G5B5:
		{
			for (int y = 0; y < copyH; ++y)
			{
				const uint8_t *src = m_ff->rgb + (size_t)y * m_ff->rgbPitch;
				uint16_t *dst = (uint16_t *)((uint8_t *)mem + (size_t)y * bpt);
				for (int x = 0; x < copyW; ++x)
				{
					uint8_t b = src[x * 4 + 0];
					uint8_t g = src[x * 4 + 1];
					uint8_t r = src[x * 4 + 2];
					dst[x] = (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
				}
			}
			break;
		}
		default:
			break;
	}
	buffer->unlock();
#else
	(void)buffer;
#endif
}

void BinkVideoStream::frameNext( void )
{
#if HAS_FFMPEG
	if (!m_ff) return;
	int64_t count = ctxFrameCount(m_ff);
	m_ff->frameNumber++;
	if (count > 0 && m_ff->frameNumber >= count)
	{
		// Loop back (matches Bink SDK / WindowVideoManager frameIndex()==0 check)
		m_ff->frameNumber = 0;
		ctxSeekFrame(m_ff, 0);
		m_ff->startTickMs = (int64_t)GetTickCount();
	}
#endif
}

Int BinkVideoStream::frameIndex( void )
{
#if HAS_FFMPEG
	if (!m_ff) return 0;
	return (Int)m_ff->frameNumber;
#else
	return 0;
#endif
}

Int BinkVideoStream::frameCount( void )
{
#if HAS_FFMPEG
	if (!m_ff) return 0;
	return (Int)ctxFrameCount(m_ff);
#else
	return 0;
#endif
}

void BinkVideoStream::frameGoto( Int index )
{
#if HAS_FFMPEG
	if (!m_ff) return;
	int64_t count = ctxFrameCount(m_ff);
	if (index < 0) index = 0;
	if (count > 0 && index >= count) index = (Int)(count - 1); // LoadScreen calls frameGoto(frameCount())
	m_ff->frameNumber = index;
	ctxSeekFrame(m_ff, index);
	m_ff->startTickMs = (int64_t)GetTickCount() - (int64_t)(index * 1000.0 / ctxFps(m_ff));
#endif
}

Int BinkVideoStream::height( void )
{
#if HAS_FFMPEG
	if (!m_ff || !m_ff->dec) return 0;
	return m_ff->dec->height;
#else
	return 0;
#endif
}

Int BinkVideoStream::width( void )
{
#if HAS_FFMPEG
	if (!m_ff || !m_ff->dec) return 0;
	return m_ff->dec->width;
#else
	return 0;
#endif
}

//============================================================================
// BinkVideoPlayer
//============================================================================

BinkVideoPlayer::BinkVideoPlayer()
{
}

BinkVideoPlayer::~BinkVideoPlayer()
{
	deinit();
}

void BinkVideoPlayer::init( void )
{
	VideoPlayer::init();
}

void BinkVideoPlayer::deinit( void )
{
	closeAllStreams();
	VideoPlayer::deinit();
}

void BinkVideoPlayer::reset( void )
{
	VideoPlayer::reset();
}

void BinkVideoPlayer::update( void )
{
	VideoPlayer::update();
}

void BinkVideoPlayer::loseFocus( void )
{
	VideoPlayer::loseFocus();
}

void BinkVideoPlayer::regainFocus( void )
{
	VideoPlayer::regainFocus();
}

VideoStreamInterface* BinkVideoPlayer::createStream( BinkFFmpegCtx *ctx )
{
	if (!ctx)
		return NULL;

	BinkVideoStream *stream = NEW BinkVideoStream;
	if (!stream)
	{
#if HAS_FFMPEG
		ctxClose(ctx);
#endif
		return NULL;
	}

	stream->m_ff = ctx;
	stream->m_next = m_firstStream;
	stream->m_player = this;
	m_firstStream = stream;
	return stream;
}

VideoStreamInterface* BinkVideoPlayer::open( AsciiString movieTitle )
{
#if !HAS_FFMPEG
	(void)movieTitle;
	return NULL;
#else
	VideoStreamInterface* stream = NULL;

	const Video* pVideo = getVideo(movieTitle);
	if (!pVideo)
		return NULL;

	char filePath[MAX_PATH];
	filePath[0] = 0;

	if (TheGlobalData && TheGlobalData->m_modDir.isNotEmpty())
	{
		sprintf(filePath, "%s%s\\%s.%s", TheGlobalData->m_modDir.str(), VIDEO_PATH, pVideo->m_filename.str(), VIDEO_EXT);
	}

	BinkFFmpegCtx *ctx = NULL;
	if (filePath[0])
		ctx = ctxOpen(filePath);

	if (!ctx)
	{
		sprintf(filePath, VIDEO_LANG_PATH_FORMAT, GetRegistryLanguage().str(), pVideo->m_filename.str(), VIDEO_EXT);
		ctx = ctxOpen(filePath);
	}

	if (!ctx)
	{
		sprintf(filePath, "%s\\%s.%s", VIDEO_PATH, pVideo->m_filename.str(), VIDEO_EXT);
		ctx = ctxOpen(filePath);
	}

	if (!ctx)
	{
		DEBUG_LOG(("BinkVideoPlayer::open - failed to open Bink '%s'\n", pVideo->m_filename.str()));
		return NULL;
	}

	stream = createStream(ctx);
	return stream;
#endif
}

VideoStreamInterface* BinkVideoPlayer::load( AsciiString movieTitle )
{
	// FFmpeg path streams from disk; load() is equivalent to open()
	return open(movieTitle);
}

void BinkVideoPlayer::notifyVideoPlayerOfNewProvider( Bool nowHasValid )
{
	// Bink audio was previously routed through Miles/DirectSound; video-only FFmpeg path ignores this.
	(void)nowHasValid;
}

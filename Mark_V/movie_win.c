/*
Copyright (C) 2001 Quake done Quick
Copyright (C) 2009-2014 Baker and others

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sys_win_movie.c -- server code for moving users

#include "quakedef.h"

#ifdef SUPPORTS_AVI_CAPTURE // Baker change
#include <windows.h>
#include <vfw.h>

static void (CALLBACK *qAVIFileInit)(void);
static HRESULT (CALLBACK *qAVIFileOpen)(PAVIFILE *, LPCTSTR, UINT, LPCLSID);
static HRESULT (CALLBACK *qAVIFileCreateStream)(PAVIFILE, PAVISTREAM *, AVISTREAMINFO *);
static HRESULT (CALLBACK *qAVIMakeCompressedStream)(PAVISTREAM *, PAVISTREAM, AVICOMPRESSOPTIONS *, CLSID *);
static HRESULT (CALLBACK *qAVIStreamSetFormat)(PAVISTREAM, LONG, LPVOID, LONG);
static HRESULT (CALLBACK *qAVIStreamWrite)(PAVISTREAM, LONG, LONG, LPVOID, LONG, DWORD, LONG *, LONG *);
static ULONG (CALLBACK *qAVIStreamRelease)(PAVISTREAM);
static ULONG (CALLBACK *qAVIFileRelease)(PAVIFILE);
static void (CALLBACK *qAVIFileExit)(void);

static MMRESULT (ACMAPI *qacmDriverOpen)(LPHACMDRIVER, HACMDRIVERID, DWORD);
static MMRESULT (ACMAPI *qacmDriverDetails)(HACMDRIVERID, LPACMDRIVERDETAILS, DWORD);
static MMRESULT (ACMAPI *qacmDriverEnum)(ACMDRIVERENUMCB, DWORD, DWORD);
static MMRESULT (ACMAPI *qacmFormatTagDetails)(HACMDRIVER, LPACMFORMATTAGDETAILS, DWORD);
static MMRESULT (ACMAPI *qacmStreamOpen)(LPHACMSTREAM, HACMDRIVER, LPWAVEFORMATEX, LPWAVEFORMATEX, LPWAVEFILTER, DWORD, DWORD, DWORD);
static MMRESULT (ACMAPI *qacmStreamSize)(HACMSTREAM, DWORD, LPDWORD, DWORD);
static MMRESULT (ACMAPI *qacmStreamPrepareHeader)(HACMSTREAM, LPACMSTREAMHEADER, DWORD);
static MMRESULT (ACMAPI *qacmStreamUnprepareHeader)(HACMSTREAM, LPACMSTREAMHEADER, DWORD);
static MMRESULT (ACMAPI *qacmStreamConvert)(HACMSTREAM, LPACMSTREAMHEADER, DWORD);
static MMRESULT (ACMAPI *qacmStreamClose)(HACMSTREAM, DWORD);
static MMRESULT (ACMAPI *qacmDriverClose)(HACMDRIVER, DWORD);

static HINSTANCE avi_handle = NULL, acm_handle = NULL;

PAVIFILE	m_file;
PAVISTREAM	m_uncompressed_video_stream;
PAVISTREAM  m_compressed_video_stream;
PAVISTREAM  m_audio_stream;

unsigned long m_codec_fourcc;
int			m_video_frame_counter;
int			m_video_frame_size;

cbool	m_audio_is_mp3;
int			m_audio_frame_counter;
WAVEFORMATEX m_wave_format;
MPEGLAYER3WAVEFORMAT m_mp3_format;
cbool	mp3_encoder_exists;
HACMDRIVER	m_mp3_driver;
HACMSTREAM	m_mp3_stream;
ACMSTREAMHEADER	m_mp3_stream_header;

extern cbool avi_loaded, acm_loaded;


#define AVI_GETFUNC(f) (qAVI##f = (void *)GetProcAddress(avi_handle, "AVI" #f))
#define ACM_GETFUNC(f) (qacm##f = (void *)GetProcAddress(acm_handle, "acm" #f))

void AVI_LoadLibrary (void)
{
	avi_loaded = false;

	if (!(avi_handle = LoadLibrary("avifil32.dll")))
	{
		Con_WarningLinef ("Avi capturing module not found");
		goto fail;
	}

	AVI_GETFUNC(FileInit);
	AVI_GETFUNC(FileOpen);
	AVI_GETFUNC(FileCreateStream);
	AVI_GETFUNC(MakeCompressedStream);
	AVI_GETFUNC(StreamSetFormat);
	AVI_GETFUNC(StreamWrite);
	AVI_GETFUNC(StreamRelease);
	AVI_GETFUNC(FileRelease);
	AVI_GETFUNC(FileExit);

	avi_loaded = qAVIFileInit && qAVIFileOpen && qAVIFileCreateStream &&
			qAVIMakeCompressedStream && qAVIStreamSetFormat && qAVIStreamWrite &&
			qAVIStreamRelease && qAVIFileRelease && qAVIFileExit;

	if (!avi_loaded)
	{
		Con_SafePrintLinef ("Avi capturing module not initialized");
		goto fail;
	}

	Con_SafePrintLinef ("Avi capturing module initialized");
	return;

fail:
	if (avi_handle)
	{
		FreeLibrary (avi_handle);
		avi_handle = NULL;
	}
}

void ACM_LoadLibrary (void)
{
	acm_loaded = false;

	if (!(acm_handle = LoadLibrary("msacm32.dll")))
	{
		Con_WarningLinef ("ACM module not found");
		goto fail;
	}

	ACM_GETFUNC(DriverOpen);
	ACM_GETFUNC(DriverEnum);
	ACM_GETFUNC(StreamOpen);
	ACM_GETFUNC(StreamSize);
	ACM_GETFUNC(StreamPrepareHeader);
	ACM_GETFUNC(StreamUnprepareHeader);
	ACM_GETFUNC(StreamConvert);
	ACM_GETFUNC(StreamClose);
	ACM_GETFUNC(DriverClose);
	qacmDriverDetails = (void *)GetProcAddress (acm_handle, "acmDriverDetailsA");
	qacmFormatTagDetails = (void *)GetProcAddress (acm_handle, "acmFormatTagDetailsA");

	acm_loaded = qacmDriverOpen && qacmDriverDetails && qacmDriverEnum &&
			qacmFormatTagDetails && qacmStreamOpen && qacmStreamSize &&
			qacmStreamPrepareHeader && qacmStreamUnprepareHeader &&
			qacmStreamConvert && qacmStreamClose && qacmDriverClose;

	if (!acm_loaded)
	{
		Con_SafePrintLinef ("ACM module not initialized");
		goto fail;
	}

	Con_SafePrintLinef ("ACM module initialized");
	return;

fail:
	if (acm_handle)
	{
		FreeLibrary (acm_handle);
		acm_handle = NULL;
	}
}

PAVISTREAM Capture_VideoStream (void)
{
	return m_codec_fourcc ? m_compressed_video_stream : m_uncompressed_video_stream;
}

BOOL CALLBACK acmDriverEnumCallback (HACMDRIVERID mp3_driver_id, DWORD dwInstance, DWORD fdwSupport)
{
	if (fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CODEC)
	{
		unsigned int	i;
		ACMDRIVERDETAILS drvDetails;

		memset (&drvDetails, 0, sizeof(drvDetails));
		drvDetails.cbStruct = sizeof(drvDetails);
		qacmDriverDetails (mp3_driver_id, &drvDetails, 0);
		qacmDriverOpen (&m_mp3_driver, mp3_driver_id, 0);

		for (i = 0 ; i < drvDetails.cFormatTags ; i++)
		{
			ACMFORMATTAGDETAILS	fmtDetails;

			memset (&fmtDetails, 0, sizeof(fmtDetails));
			fmtDetails.cbStruct = sizeof(fmtDetails);
			fmtDetails.dwFormatTagIndex = i;
			qacmFormatTagDetails (m_mp3_driver, &fmtDetails, ACM_FORMATTAGDETAILSF_INDEX);
			if (fmtDetails.dwFormatTag == WAVE_FORMAT_MPEGLAYER3)
			{
				MMRESULT	mmr;

				Con_DPrintLinef ("MP3-capable ACM codec found: %s", drvDetails.szLongName);

				m_mp3_stream = NULL;
				if ((mmr = qacmStreamOpen(&m_mp3_stream, m_mp3_driver, &m_wave_format, &m_mp3_format.wfx, NULL, 0, 0, 0)))
				{
					switch (mmr)
					{
					case MMSYSERR_INVALPARAM:
						Con_DPrintLinef ("ERROR: Invalid parameters passed to acmStreamOpen");
						break;

					case ACMERR_NOTPOSSIBLE:
						Con_DPrintLinef ("ERROR: No ACM filter found capable of encoding MP3");
						break;

					default:
						Con_DPrintLinef ("ERROR: Couldn't open ACM encoding stream");
						break;
					}
					continue;
				}
				mp3_encoder_exists = true;

				return false;
			}
		}

		qacmDriverClose (m_mp3_driver, 0);
	}

	return true;
}

int Capture_Open (const char *filename, const char *usercodec, cbool silentish)
{
	HRESULT			hr;
	BITMAPINFOHEADER bitmap_info_header;
	AVISTREAMINFO	stream_header;
	const char		*fourcc;

	m_video_frame_counter = m_audio_frame_counter = 0;
	m_file = NULL;
	m_codec_fourcc = 0;
	m_uncompressed_video_stream = m_compressed_video_stream = m_audio_stream = NULL;
	m_audio_is_mp3 = (cbool)capture_mp3.value;

	fourcc = usercodec;
	if (strcasecmp(fourcc, "none") != 0)	// codec fourcc supplied
		m_codec_fourcc = mmioFOURCC (fourcc[0], fourcc[1], fourcc[2], fourcc[3]);

	qAVIFileInit ();
	hr = qAVIFileOpen (&m_file, filename, OF_WRITE | OF_CREATE, NULL);
	if (FAILED(hr))
	{
		if (!silentish)
			Con_PrintLinef ("ERROR: Couldn't open AVI file for writing");
		return -1;

	}

	// initialize video data
#pragma message ("Video resize should probably be disabled during vid capture")
	m_video_frame_size = clwidth * clheight * RGB_3;
#pragma message ("Baker: These should be clwidth, especially with GL -resizable or after blitting is in WinQuake")
	memset (&bitmap_info_header, 0, sizeof(bitmap_info_header));
	bitmap_info_header.biSize = sizeof(BITMAPINFOHEADER);

	bitmap_info_header.biWidth = clwidth;
	bitmap_info_header.biHeight = clheight;

	bitmap_info_header.biPlanes = 1;
	bitmap_info_header.biBitCount = 24;
	bitmap_info_header.biCompression = BI_RGB;
	bitmap_info_header.biSizeImage = m_video_frame_size;

	memset (&stream_header, 0, sizeof(stream_header));
	stream_header.fccType = streamtypeVIDEO;
	stream_header.fccHandler = m_codec_fourcc;
	stream_header.dwScale = 1;
	stream_header.dwRate = (unsigned long)(0.5 + capture_fps.value);
	stream_header.dwSuggestedBufferSize = bitmap_info_header.biSizeImage;
	SetRect (&stream_header.rcFrame, 0, 0, bitmap_info_header.biWidth, bitmap_info_header.biHeight);

	hr = qAVIFileCreateStream (m_file, &m_uncompressed_video_stream, &stream_header);
	if (FAILED(hr))
	{
		if (!silentish)
			Con_PrintLinef ("ERROR: Couldn't create video stream");
		return -2;
	}

	if (m_codec_fourcc)
	{
		AVICOMPRESSOPTIONS	opts;

		memset (&opts, 0, sizeof(opts));
		opts.fccType = stream_header.fccType;
		opts.fccHandler = m_codec_fourcc;

		// Make the stream according to compression
		hr = qAVIMakeCompressedStream (&m_compressed_video_stream, m_uncompressed_video_stream, &opts, NULL);
		if (FAILED(hr))
		{
			if (!silentish)
				Con_PrintLinef ("ERROR: Couldn't make compressed video stream");
			return -3;
		}
	}

	hr = qAVIStreamSetFormat (Capture_VideoStream(), 0, &bitmap_info_header, bitmap_info_header.biSize);
	if (FAILED(hr))
	{
		Con_PrintLinef ("ERROR: Couldn't set video stream format");
		return false;
	}

	// initialize audio data
	memset (&m_wave_format, 0, sizeof(m_wave_format));
	m_wave_format.wFormatTag = WAVE_FORMAT_PCM;
	m_wave_format.nChannels = 2;		// always stereo in Quake sound engine
	m_wave_format.nSamplesPerSec = shm->speed;
	m_wave_format.wBitsPerSample = 16;	// always 16bit in Quake sound engine
	m_wave_format.nBlockAlign = m_wave_format.wBitsPerSample/8 * m_wave_format.nChannels;
	m_wave_format.nAvgBytesPerSec = m_wave_format.nSamplesPerSec * m_wave_format.nBlockAlign;

	memset (&stream_header, 0, sizeof(stream_header));
	stream_header.fccType = streamtypeAUDIO;
	stream_header.dwScale = m_wave_format.nBlockAlign;
	stream_header.dwRate = stream_header.dwScale * (unsigned long)m_wave_format.nSamplesPerSec;
	stream_header.dwSampleSize = m_wave_format.nBlockAlign;

	hr = qAVIFileCreateStream (m_file, &m_audio_stream, &stream_header);
	if (FAILED(hr))
	{
		Con_PrintLinef ("ERROR: Couldn't create audio stream");
		return false;
	}

	if (m_audio_is_mp3)
	{
		memset (&m_mp3_format, 0, sizeof(m_mp3_format));
		m_mp3_format.wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
		m_mp3_format.wfx.nChannels = 2;
		m_mp3_format.wfx.nSamplesPerSec = shm->speed;
		m_mp3_format.wfx.wBitsPerSample = 0;
		m_mp3_format.wfx.nBlockAlign = 1;
		m_mp3_format.wfx.nAvgBytesPerSec = capture_mp3_kbps.value * 125;
		m_mp3_format.wfx.cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;
		m_mp3_format.wID = MPEGLAYER3_ID_MPEG;
		m_mp3_format.fdwFlags = MPEGLAYER3_FLAG_PADDING_OFF;
		m_mp3_format.nBlockSize = m_mp3_format.wfx.nAvgBytesPerSec / capture_fps.value;
		m_mp3_format.nFramesPerBlock = 1;
		m_mp3_format.nCodecDelay = 1393;

		// try to find an MP3 codec
		m_mp3_driver = NULL;
		mp3_encoder_exists = false;
		qacmDriverEnum (acmDriverEnumCallback, 0, 0);
		if (!mp3_encoder_exists)
		{
			Con_PrintLinef ("ERROR: Couldn't find any MP3 encoder");
			return false;
		}

		hr = qAVIStreamSetFormat (m_audio_stream, 0, &m_mp3_format, sizeof(MPEGLAYER3WAVEFORMAT));
		if (FAILED(hr))
		{
			Con_PrintLinef ("ERROR: Couldn't set audio stream format");
			return false;
		}
	}
	else
	{
		hr = qAVIStreamSetFormat (m_audio_stream, 0, &m_wave_format, sizeof(WAVEFORMATEX));
		if (FAILED(hr))
		{
			Con_PrintLinef ("ERROR: Couldn't set audio stream format");
			return false;
		}
	}

	return true;
}

void Capture_Close (void)
{
	if (m_uncompressed_video_stream)
		qAVIStreamRelease (m_uncompressed_video_stream);
	if (m_compressed_video_stream)
		qAVIStreamRelease (m_compressed_video_stream);
	if (m_audio_stream)
		qAVIStreamRelease (m_audio_stream);
	if (m_audio_is_mp3)
	{
		qacmStreamClose (m_mp3_stream, 0);
		qacmDriverClose (m_mp3_driver, 0);
	}
	if (m_file)
		qAVIFileRelease (m_file);

	qAVIFileExit ();
}

void Capture_WriteVideo (byte *pixel_buffer)
{
	HRESULT	hr;
	int	size = clwidth * clheight * RGB_3;

	// check frame size (TODO: other things too?) hasn't changed
	if (m_video_frame_size != size)
	{
		Con_PrintLinef ("ERROR: Frame size changed: new %d old %d", size, m_video_frame_size);
		return;
	}

	if (!Capture_VideoStream())
	{
		Con_PrintLinef ("ERROR: Video stream is NULL");
		return;
	}

	hr = qAVIStreamWrite (Capture_VideoStream(), m_video_frame_counter++, 1, pixel_buffer, m_video_frame_size, AVIIF_KEYFRAME, NULL, NULL);
	if (FAILED(hr))
	{
		Con_PrintLinef ("ERROR: Couldn't write video stream");
		return;
	}
}

void Capture_WriteAudio (int samples, byte *sample_buffer)
{
	HRESULT		hr;
	unsigned long sample_bufsize;

	if (!m_audio_stream)
	{
		Con_PrintLinef ("ERROR: Audio stream is NULL");
		return;
	}

	sample_bufsize = samples * m_wave_format.nBlockAlign;
	if (m_audio_is_mp3)
	{
		MMRESULT	mmr;
		byte		*mp3_buffer;
		unsigned long mp3_bufsize;

		if ((mmr = qacmStreamSize(m_mp3_stream, sample_bufsize, &mp3_bufsize, ACM_STREAMSIZEF_SOURCE)))
		{
			Con_PrintLinef ("ERROR: Couldn't get mp3bufsize");
			return;
		}
		if (!mp3_bufsize)
		{
			Con_PrintLinef ("ERROR: mp3bufsize is zero");
			return;
		}
		mp3_buffer = calloc (mp3_bufsize, 1);

		memset (&m_mp3_stream_header, 0, sizeof(m_mp3_stream_header));
		m_mp3_stream_header.cbStruct = sizeof(m_mp3_stream_header);
		m_mp3_stream_header.pbSrc = sample_buffer;
		m_mp3_stream_header.cbSrcLength = sample_bufsize;
		m_mp3_stream_header.pbDst = mp3_buffer;
		m_mp3_stream_header.cbDstLength = mp3_bufsize;

		if ((mmr = qacmStreamPrepareHeader(m_mp3_stream, &m_mp3_stream_header, 0)))
		{
			Con_PrintLinef ("ERROR: Couldn't prepare header");
			free (mp3_buffer);
			return;
		}

		if ((mmr = qacmStreamConvert(m_mp3_stream, &m_mp3_stream_header, ACM_STREAMCONVERTF_BLOCKALIGN)))
		{
			Con_PrintLinef ("ERROR: Couldn't convert audio stream");
			goto clean;
		}

		hr = qAVIStreamWrite (m_audio_stream, m_audio_frame_counter++, 1, mp3_buffer, m_mp3_stream_header.cbDstLengthUsed, AVIIF_KEYFRAME, NULL, NULL);

clean:
		if ((mmr = qacmStreamUnprepareHeader(m_mp3_stream, &m_mp3_stream_header, 0)))
		{
			Con_PrintLinef ("ERROR: Couldn't unprepare header");
			free (mp3_buffer);
			return;
		}

		free (mp3_buffer);
	}
	else
	{
		hr = qAVIStreamWrite (m_audio_stream, m_audio_frame_counter++, 1, sample_buffer, sample_bufsize, AVIIF_KEYFRAME, NULL, NULL);
	}
	if (FAILED(hr))
	{
		Con_PrintLinef ("ERROR: Couldn't write audio stream");
		return;
	}
}
#endif // Baker change +
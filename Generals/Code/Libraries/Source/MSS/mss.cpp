// MSS/mss.cpp — Miles Sound System over OpenAL Soft.
// Single voice pool shared by 2D samples, 3D samples, streams and quick one-shots.
// Handles: HSAMPLE/HSTREAM/HAUDIO = 1-based pool index; H3DSAMPLE = (void*)(uintptr_t)index.
#include "MSS/mss.h"

#include <AL/al.h>
#include <AL/alc.h>

#include <windows.h>
#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdlib>

namespace {

constexpr int kMaxVoices = 512;
constexpr int kMaxUserData = 4;
// Sentinel handle for the OpenAL listener (distinct from voice indices)
constexpr uintptr_t kListenerHandle = 0xF0000001u;

enum VoiceKind { VK_NONE = 0, VK_2D, VK_3D, VK_STREAM, VK_QUICK };

struct Voice {
	bool used = false;
	VoiceKind kind = VK_NONE;
	ALuint source = 0;
	ALuint buffer = 0;
	bool started = false;
	bool paused = false;
	S32 loopCount = 0;          // -1 infinite, 0 none (Miles loops handled via EOS+restart)
	float volume = 1.0f;
	float pan = 0.5f;
	U32 userData[kMaxUserData] = {};
	AILSAMPLECB eos2d = nullptr;
	AIL3DSAMPLECB eos3d = nullptr;
	AILSTREAMCB eosStream = nullptr;
	std::vector<unsigned char> pcm;   // owns decoded PCM for the AL buffer
	U32 rate = 22050;
	U16 bits = 16;
	U16 channels = 2;
};

static CRITICAL_SECTION g_cs;
static bool g_csInited = false;
ALCdevice* g_dev = nullptr;
ALCcontext* g_ctx = nullptr;
bool g_started = false;
std::vector<Voice> g_voices;
const char* g_providerNames[] = {
	"Miles Fast 2D Positional Audio",
	"Dolby Surround",
};
constexpr int kProviderCount = 2;

static inline Voice* voiceFrom2D(HSAMPLE h) {
	if (h <= 0 || (size_t)h > g_voices.size()) return nullptr;
	Voice& v = g_voices[(size_t)h - 1];
	return (v.used && (v.kind == VK_2D || v.kind == VK_QUICK)) ? &v : nullptr;
}
static inline Voice* voiceFrom3D(H3DSAMPLE h) {
	uintptr_t id = (uintptr_t)h;
	if (id == 0 || id > g_voices.size()) return nullptr;
	Voice& v = g_voices[id - 1];
	return (v.used && v.kind == VK_3D) ? &v : nullptr;
}
static inline Voice* voiceFromStream(HSTREAM h) {
	if (h <= 0 || (size_t)h > g_voices.size()) return nullptr;
	Voice& v = g_voices[(size_t)h - 1];
	return (v.used && v.kind == VK_STREAM) ? &v : nullptr;
}
static inline Voice* voiceFromHandleAny(uintptr_t id) {
	if (id == 0 || id > g_voices.size()) return nullptr;
	Voice& v = g_voices[id - 1];
	return v.used ? &v : nullptr;
}

static void ensureVoices() {
	if (g_voices.empty()) g_voices.resize(kMaxVoices);
}

static bool ensureDevice() {
	if (g_started) return true;
	ensureVoices();
	g_dev = alcOpenDevice(nullptr);
	if (!g_dev) return false;
	g_ctx = alcCreateContext(g_dev, nullptr);
	if (!g_ctx) { alcCloseDevice(g_dev); g_dev = nullptr; return false; }
	alcMakeContextCurrent(g_ctx);
	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
	alListenerf(AL_GAIN, 1.0f);
	g_started = true;
	return true;
}

static void freeVoiceBuffers(Voice& v) {
	if (v.source && v.buffer) {
		alSourceStop(v.source);
		alSourcei(v.source, AL_BUFFER, 0);
		alDeleteBuffers(1, &v.buffer);
		v.buffer = 0;
	}
	v.pcm.clear();
	v.started = false;
	v.paused = false;
	v.eos2d = nullptr;
	v.eos3d = nullptr;
	v.eosStream = nullptr;
	v.volume = 1.0f;
	v.pan = 0.5f;
	v.loopCount = 0;
	for (int i = 0; i < kMaxUserData; ++i) v.userData[i] = 0;
}

static HSAMPLE allocVoice(VoiceKind kind) {
	if (!ensureDevice()) return 0;
	for (size_t i = 0; i < g_voices.size(); ++i) {
		if (!g_voices[i].used) {
			Voice& v = g_voices[i];
			v.used = true;
			v.kind = kind;
			if (!v.source) alGenSources(1, &v.source);
			freeVoiceBuffers(v);
			v.kind = kind;
			return (HSAMPLE)(i + 1);
		}
	}
	return 0;
}

// --- WAV -------------------------------------------------------------------

struct WavInfo {
	U16 format = 0;
	U16 channels = 0;
	U16 bits = 0;
	U32 rate = 0;
	const unsigned char* data = nullptr;
	U32 dataLen = 0;
};

static U32 rd32(const unsigned char* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((U32)p[3] << 24); }
static U32 rd32be(const unsigned char* p) { return ((U32)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }
static U16 rd16(const unsigned char* p) { return (U16)(p[0] | (p[1] << 8)); }

static bool parseWav(const unsigned char* buf, size_t len, WavInfo& out) {
	if (len < 12) return false;
	if (memcmp(buf, "RIFF", 4) != 0 || memcmp(buf + 8, "WAVE", 4) != 0) return false;
	size_t pos = 12;
	bool haveFmt = false;
	while (pos + 8 <= len) {
		const unsigned char* ch = buf + pos;
		U32 sz = rd32(ch + 4);
		size_t body = pos + 8;
		if (body + sz > len) break;
		if (memcmp(ch, "fmt ", 4) == 0 && sz >= 16) {
			out.format = rd16(buf + body);
			out.channels = rd16(buf + body + 2);
			out.rate = rd32(buf + body + 4);
			out.bits = rd16(buf + body + 14);
			if (out.format == 0xFFFE && sz >= 40) {
				// WAVE_FORMAT_EXTENSIBLE: subformat GUID first 2 bytes = format tag
				out.format = rd16(buf + body + 24);
			}
			haveFmt = true;
		} else if (memcmp(ch, "data", 4) == 0) {
			out.data = buf + body;
			out.dataLen = sz;
			if (haveFmt) return true;
		}
		pos = body + sz + (sz & 1);
	}
	return false;
}

static bool loadWavFile(const char* path, std::vector<unsigned char>& whole, WavInfo& info) {
	FILE* f = fopen(path, "rb");
	if (!f) return false;
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (sz <= 0 || sz > 64 * 1024 * 1024) { fclose(f); return false; }
	whole.resize((size_t)sz);
	if (fread(whole.data(), 1, (size_t)sz, f) != (size_t)sz) { fclose(f); return false; }
	fclose(f);
	return parseWav(whole.data(), whole.size(), info);
}

static ALenum alFormatFor(const WavInfo& w) {
	if (w.bits == 8 && w.channels == 1) return AL_FORMAT_MONO8;
	if (w.bits == 8 && w.channels == 2) return AL_FORMAT_STEREO8;
	if (w.bits == 16 && w.channels == 1) return AL_FORMAT_MONO16;
	if (w.bits == 16 && w.channels == 2) return AL_FORMAT_STEREO16;
	return 0;
}

// Fill voice.pcm from WAV memory buffer and upload to a fresh AL buffer.
static bool uploadWavToVoice(Voice& v, const unsigned char* file, size_t fileLen, U32 offset) {
	WavInfo w;
	if (!parseWav(file + offset, fileLen > offset ? fileLen - offset : 0, w)) return false;
	if (w.format != 1 /*PCM*/ && w.format != 3 /*IEEE float*/) return false;
	if (w.format == 3) return false; // float WAV not supported yet
	ALenum fmt = alFormatFor(w);
	if (!fmt || !w.data || !w.dataLen || !w.rate) return false;
	v.pcm.assign(w.data, w.data + w.dataLen);
	v.rate = w.rate;
	v.bits = w.bits;
	v.channels = w.channels;
	if (v.buffer) { alSourcei(v.source, AL_BUFFER, 0); alDeleteBuffers(1, &v.buffer); v.buffer = 0; }
	alGenBuffers(1, &v.buffer);
	alBufferData(v.buffer, fmt, v.pcm.data(), (ALsizei)v.pcm.size(), (ALsizei)w.rate);
	alSourcei(v.source, AL_BUFFER, (ALint)v.buffer);
	return true;
}

static void applyGain(Voice& v) {
	float g = v.volume < 0.0f ? 0.0f : (v.volume > 1.0f ? 1.0f : v.volume);
	alSourcef(v.source, AL_GAIN, g);
	if (v.kind == VK_3D) {
		alSourcei(v.source, AL_SOURCE_RELATIVE, AL_FALSE);
	} else {
		float x = (v.pan - 0.5f) * 2.0f;
		alSourcei(v.source, AL_SOURCE_RELATIVE, AL_TRUE);
		alSource3f(v.source, AL_POSITION, x, 0.0f, 0.0f);
	}
}

} // namespace

// Minimal critical-section guard (avoid libwinpthread / std::mutex)
struct CsLock {
	CsLock() {
		if (!g_csInited) { InitializeCriticalSection(&g_cs); g_csInited = true; }
		EnterCriticalSection(&g_cs);
	}
	~CsLock() { LeaveCriticalSection(&g_cs); }
	CsLock(const CsLock&) = delete;
	CsLock& operator=(const CsLock&) = delete;
};

// --- Lifecycle -------------------------------------------------------------

extern "C" int AIL_startup(void) {
	CsLock lk;
	return ensureDevice() ? 1 : 0;
}

extern "C" void AIL_shutdown(void) {
	CsLock lk;
	if (!g_started) return;
	for (auto& v : g_voices) {
		if (v.used && v.source) {
			alSourceStop(v.source);
			if (v.buffer) { alSourcei(v.source, AL_BUFFER, 0); alDeleteBuffers(1, &v.buffer); }
			alDeleteSources(1, &v.source);
			v.source = 0;
		}
		v.used = false;
		v.kind = VK_NONE;
	}
	alcMakeContextCurrent(nullptr);
	if (g_ctx) { alcDestroyContext(g_ctx); g_ctx = nullptr; }
	if (g_dev) { alcCloseDevice(g_dev); g_dev = nullptr; }
	g_started = false;
}

extern "C" void AIL_set_preference(int, int) {}
extern "C" void AIL_MSS_version(char* buf, int len) {
	if (buf && len > 0) { strncpy(buf, "OpenAL Soft shim 1.0", (size_t)len); buf[len - 1] = 0; }
}
extern "C" void AIL_set_redist_directory(const char*) {}
extern "C" void AIL_set_file_callbacks(AILFILEOPENCB, AILFILECLOSECB, AILFILESEEKCB, AILFILEREADCB) {}
extern "C" U32 AIL_get_timer_highest_delay(void) { return 0; }

// --- Service (EOS pump) ----------------------------------------------------

extern "C" void AIL_service(void) {
	// Snapshot callbacks under lock, invoke outside to avoid deadlock if a
	// callback re-enters AIL_* (notifyOfAudioCompletion restarts sounds).
	struct Fired { VoiceKind kind; uintptr_t id; AILSAMPLECB c2; AIL3DSAMPLECB c3; AILSTREAMCB cs; };
	Fired fired[64];
	int n = 0;
	{
		CsLock lk;
		if (!g_started) return;
		for (size_t i = 0; i < g_voices.size() && n < 64; ++i) {
			Voice& v = g_voices[i];
			if (!v.used || !v.started || v.paused || !v.source) continue;
			ALint st = AL_STOPPED;
			alGetSourcei(v.source, AL_SOURCE_STATE, &st);
			if (st != AL_STOPPED) continue;
			// Natural end (not AIL_stop_sample — that clears started)
			if (n < 64) {
				fired[n].kind = v.kind;
				fired[n].id = i + 1;
				fired[n].c2 = v.eos2d;
				fired[n].c3 = v.eos3d;
				fired[n].cs = v.eosStream;
				++n;
			}
			if (v.kind == VK_QUICK) {
				// One-shot with no owner: recycle immediately
				freeVoiceBuffers(v);
				v.used = false;
				v.kind = VK_NONE;
			} else {
				v.started = false; // wait for Miles to restart or release
			}
		}
	}
	for (int i = 0; i < n; ++i) {
		if (fired[i].kind == VK_2D && fired[i].c2) fired[i].c2((HSAMPLE)fired[i].id);
		else if (fired[i].kind == VK_3D && fired[i].c3) fired[i].c3((H3DSAMPLE)fired[i].id);
		else if (fired[i].kind == VK_STREAM && fired[i].cs) fired[i].cs((HSTREAM)fired[i].id);
	}
}

// --- 2D samples ------------------------------------------------------------

extern "C" void AIL_register_EOS_callback(HSAMPLE s, AILSAMPLECB cb) {
	CsLock lk;
	if (Voice* v = voiceFrom2D(s)) v->eos2d = cb;
}

extern "C" void AIL_set_sample_volume_pan(HSAMPLE s, float vol, float pan) {
	CsLock lk;
	if (Voice* v = voiceFrom2D(s)) { v->volume = vol; v->pan = pan; applyGain(*v); }
}
extern "C" void AIL_sample_volume_pan(HSAMPLE s, float* vol, float* pan) {
	CsLock lk;
	Voice* v = voiceFrom2D(s);
	if (vol) *vol = v ? v->volume : 1.0f;
	if (pan) *pan = v ? v->pan : 0.5f;
}
extern "C" void AIL_set_sample_playback_rate(HSAMPLE s, U32 rate) {
	CsLock lk;
	if (Voice* v = voiceFrom2D(s)) { v->rate = rate; alSourcef(v->source, AL_PITCH, 1.0f); }
}
extern "C" U32 AIL_sample_playback_rate(HSAMPLE s) {
	CsLock lk;
	if (Voice* v = voiceFrom2D(s)) return v->rate;
	return 22050;
}
extern "C" void AIL_start_sample(HSAMPLE s) {
	CsLock lk;
	if (Voice* v = voiceFrom2D(s)) {
		applyGain(*v);
		alSourcei(v->source, AL_LOOPING, AL_FALSE);
		alSourcePlay(v->source);
		v->started = true;
		v->paused = false;
	}
}
extern "C" void AIL_stop_sample(HSAMPLE s) {
	CsLock lk;
	if (Voice* v = voiceFrom2D(s)) { alSourceStop(v->source); v->started = false; v->paused = false; }
}
extern "C" void AIL_resume_sample(HSAMPLE s) {
	CsLock lk;
	if (Voice* v = voiceFrom2D(s)) { if (v->buffer) { alSourcePlay(v->source); v->started = true; v->paused = false; } }
}
extern "C" void AIL_release_sample_handle(HSAMPLE s) {
	CsLock lk;
	if (Voice* v = voiceFrom2D(s)) { freeVoiceBuffers(*v); v->used = false; v->kind = VK_NONE; }
}
extern "C" HSAMPLE AIL_allocate_sample_handle(HDIGDRIVER) {
	CsLock lk;
	return allocVoice(VK_2D);
}
extern "C" int AIL_init_sample(HSAMPLE s) {
	CsLock lk;
	if (Voice* v = voiceFrom2D(s)) { freeVoiceBuffers(*v); v->kind = VK_2D; v->used = true; return 1; }
	return 0;
}
extern "C" int AIL_set_sample_file(HSAMPLE s, void* file, U32 offset) {
	CsLock lk;
	if (!file) return 0;
	Voice* v = voiceFrom2D(s);
	if (!v) return 0;
	// file is a memory image of unknown length — AudioFileCache guarantees RIFF layout;
	// parse defensively by scanning bounded window (cache stores real size but doesn't pass it).
	// We rely on RIFF sizes inside the header.
	return uploadWavToVoice(*v, (const unsigned char*)file, (size_t)-1, offset) ? 1 : 0;
}
extern "C" void AIL_set_sample_user_data(HSAMPLE s, U32 index, void* val) {
	CsLock lk;
	Voice* v = voiceFrom2D(s);
	if (v && index < kMaxUserData) v->userData[index] = (U32)(uintptr_t)val;
}
extern "C" void* AIL_sample_user_data(HSAMPLE s, U32 index) {
	CsLock lk;
	Voice* v = voiceFrom2D(s);
	if (v && index < kMaxUserData) return (void*)(uintptr_t)v->userData[index];
	return nullptr;
}
extern "C" void AIL_set_sample_processor(HSAMPLE, U32, HPROVIDER) {}
extern "C" int AIL_set_filter_sample_preference(HSAMPLE, char const*, void*) { return 0; }

// --- 3D samples ------------------------------------------------------------

extern "C" void AIL_register_3D_EOS_callback(H3DSAMPLE s, AIL3DSAMPLECB cb) {
	CsLock lk;
	if (Voice* v = voiceFrom3D(s)) v->eos3d = cb;
}
extern "C" void AIL_start_3D_sample(H3DSAMPLE s) {
	CsLock lk;
	if (Voice* v = voiceFrom3D(s)) {
		applyGain(*v);
		alSourcePlay(v->source);
		v->started = true;
		v->paused = false;
	}
}
extern "C" void AIL_stop_3D_sample(H3DSAMPLE s) {
	CsLock lk;
	if (Voice* v = voiceFrom3D(s)) { alSourceStop(v->source); v->started = false; v->paused = false; }
}
extern "C" void AIL_resume_3D_sample(H3DSAMPLE s) {
	CsLock lk;
	if (Voice* v = voiceFrom3D(s)) { if (v->buffer) { alSourcePlay(v->source); v->started = true; v->paused = false; } }
}
extern "C" void AIL_release_3D_sample_handle(H3DSAMPLE s) {
	CsLock lk;
	if (Voice* v = voiceFrom3D(s)) { freeVoiceBuffers(*v); v->used = false; v->kind = VK_NONE; }
}
extern "C" H3DSAMPLE AIL_allocate_3D_sample_handle(HPROVIDER) {
	CsLock lk;
	HSAMPLE h = allocVoice(VK_3D);
	return (H3DSAMPLE)(uintptr_t)h;
}
extern "C" int AIL_set_3D_sample_file(H3DSAMPLE s, void* file) {
	CsLock lk;
	if (!file) return 0;
	Voice* v = voiceFrom3D(s);
	if (!v) return 0;
	return uploadWavToVoice(*v, (const unsigned char*)file, (size_t)-1, 0) ? 1 : 0;
}
extern "C" void AIL_set_3D_sample_volume(H3DSAMPLE s, float vol) {
	CsLock lk;
	if (Voice* v = voiceFrom3D(s)) { v->volume = vol; alSourcef(v->source, AL_GAIN, vol); }
}
extern "C" void AIL_set_3D_position(H3DSAMPLE s, float x, float y, float z) {
	CsLock lk;
	if ((uintptr_t)s == kListenerHandle) { alListener3f(AL_POSITION, x, y, z); return; }
	if (Voice* v = voiceFrom3D(s)) alSource3f(v->source, AL_POSITION, x, y, z);
}
extern "C" void AIL_set_3D_orientation(H3DPOBJECT o, float rx, float ry, float rz, float ux, float uy, float uz) {
	(void)o; // Miles passes either listener or unused; orientation is listener-global
	CsLock lk;
	float orient[6] = { rx, ry, rz, ux, uy, uz };
	alListenerfv(AL_ORIENTATION, orient);
}
extern "C" void AIL_set_3D_sample_distances(H3DSAMPLE s, float min, float max) {
	CsLock lk;
	if (Voice* v = voiceFrom3D(s)) {
		alSourcef(v->source, AL_REFERENCE_DISTANCE, min > 0.0f ? min : 1.0f);
		alSourcef(v->source, AL_MAX_DISTANCE, max > min ? max : 100000.0f);
	}
}
extern "C" void AIL_set_3D_user_data(H3DSAMPLE s, U32 index, void* val) {
	CsLock lk;
	if (Voice* v = voiceFrom3D(s)) if (index < kMaxUserData) v->userData[index] = (U32)(uintptr_t)val;
}
extern "C" U32 AIL_3D_user_data(H3DSAMPLE s, U32 index) {
	CsLock lk;
	if (Voice* v = voiceFrom3D(s)) if (index < kMaxUserData) return v->userData[index];
	return 0;
}
extern "C" void AIL_set_3D_sample_playback_rate(H3DSAMPLE s, U32 rate) {
	CsLock lk;
	if (Voice* v = voiceFrom3D(s)) v->rate = rate;
}
extern "C" U32 AIL_3D_sample_playback_rate(H3DSAMPLE s) {
	CsLock lk;
	if (Voice* v = voiceFrom3D(s)) return v->rate;
	return 22050;
}
extern "C" void AIL_set_3D_sample_occlusion(H3DSAMPLE, float) {}

extern "C" int AIL_open_3D_provider(HPROVIDER) { return g_started || AIL_startup() ? 0 : -1; }
extern "C" void AIL_close_3D_provider(HPROVIDER) {}
extern "C" int AIL_enumerate_3D_providers(HPROENUM* next, HPROVIDER* id, char** name) {
	if (!next || !id || !name) return 0;
	int idx = (int)(uintptr_t)*next;
	if (idx < 0 || idx >= kProviderCount) return 0;
	*id = (HPROVIDER)(uintptr_t)(idx + 1);
	*name = (char*)g_providerNames[idx];
	*next = (HPROENUM)(uintptr_t)(idx + 1);
	return 1;
}
extern "C" H3DPOBJECT AIL_open_3D_listener(HPROVIDER) {
	if (!g_started) AIL_startup();
	return (H3DPOBJECT)kListenerHandle;
}
extern "C" void AIL_close_3D_listener(H3DPOBJECT) {}
extern "C" void AIL_set_3D_speaker_type(HPROVIDER, U32) {}
extern "C" int AIL_enumerate_filters(HPROENUM* next, HPROVIDER*, char**) {
	if (next) *next = (HPROENUM)(uintptr_t)kProviderCount; // no filters
	return 0;
}

// --- Streams ---------------------------------------------------------------

extern "C" void AIL_register_stream_callback(HSTREAM s, AILSTREAMCB cb) {
	CsLock lk;
	if (Voice* v = voiceFromStream(s)) v->eosStream = cb;
}
extern "C" HSTREAM AIL_open_stream(HDIGDRIVER, const char* file, U32) {
	CsLock lk;
	if (!file || !file[0]) return 0;
	HSAMPLE h = allocVoice(VK_STREAM);
	if (!h) return 0;
	Voice& v = g_voices[(size_t)h - 1];
	std::vector<unsigned char> whole;
	WavInfo w;
	// parseWav needs full buffer; loadWavFile fills it
	if (!loadWavFile(file, whole, w) || w.format != 1 || !alFormatFor(w)) {
		freeVoiceBuffers(v); v.used = false; v.kind = VK_NONE;
		return 0;
	}
	v.pcm.assign(w.data, w.data + w.dataLen);
	v.rate = w.rate; v.bits = w.bits; v.channels = w.channels;
	alGenBuffers(1, &v.buffer);
	alBufferData(v.buffer, alFormatFor(w), v.pcm.data(), (ALsizei)v.pcm.size(), (ALsizei)w.rate);
	alSourcei(v.source, AL_BUFFER, (ALint)v.buffer);
	return h;
}
extern "C" void AIL_start_stream(HSTREAM s) {
	CsLock lk;
	if (Voice* v = voiceFromStream(s)) {
		alSourceStop(v->source);
		alSourcei(v->source, AL_BUFFER, (ALint)v->buffer);
		applyGain(*v);
		alSourcei(v->source, AL_SOURCE_RELATIVE, AL_TRUE);
		alSourcePlay(v->source);
		v->started = true;
		v->paused = false;
	}
}
extern "C" void AIL_close_stream(HSTREAM s) {
	CsLock lk;
	if (Voice* v = voiceFromStream(s)) { freeVoiceBuffers(*v); v->used = false; v->kind = VK_NONE; }
}
extern "C" void AIL_pause_stream(HSTREAM s, S32 pause) {
	CsLock lk;
	if (Voice* v = voiceFromStream(s)) {
		if (pause) { alSourcePause(v->source); v->paused = true; }
		else { alSourcePlay(v->source); v->paused = false; v->started = true; }
	}
}
extern "C" void AIL_set_stream_volume_pan(HSTREAM s, float vol, float pan) {
	CsLock lk;
	if (Voice* v = voiceFromStream(s)) { v->volume = vol; v->pan = pan; applyGain(*v); }
}
extern "C" void AIL_stream_volume_pan(HSTREAM s, float* vol, float* pan) {
	CsLock lk;
	Voice* v = voiceFromStream(s);
	if (vol) *vol = v ? v->volume : 1.0f;
	if (pan) *pan = v ? v->pan : 0.5f;
}
extern "C" void AIL_set_stream_loop_count(HSTREAM s, S32 count) {
	CsLock lk;
	if (Voice* v = voiceFromStream(s)) v->loopCount = count;
}
extern "C" S32 AIL_stream_loop_count(HSTREAM s) {
	CsLock lk;
	if (Voice* v = voiceFromStream(s)) return v->loopCount;
	return 0;
}
extern "C" void AIL_stream_ms_position(HSTREAM s, S32* ms_done, S32* ms_total) {
	CsLock lk;
	S32 total = 0;
	Voice* v = voiceFromStream(s);
	if (v && v->rate > 0 && v->channels > 0 && v->bits > 0) {
		double bytesPerSec = (double)v->rate * v->channels * (v->bits / 8);
		total = (S32)((double)v->pcm.size() / bytesPerSec * 1000.0);
	}
	// Caller quirk: getFileLengthMS passes (stream, &len, NULL) asking for TOTAL in arg1.
	if (ms_total) *ms_total = total;
	if (ms_done) *ms_done = total;
}

// --- Quick API -------------------------------------------------------------

extern "C" int AIL_quick_startup(int, int, U32, U32, U32) {
	return AIL_startup();
}
extern "C" void AIL_quick_handles(HDIGDRIVER* dig, HMDIDRIVER*, HSNDBUF*) {
	if (dig) *dig = 1; // dummy digital driver token
}
extern "C" HAUDIO AIL_quick_load_and_play(const char* file, U32 loop, U32) {
	CsLock lk;
	if (!file || !file[0]) return 0;
	HSAMPLE h = allocVoice(VK_QUICK);
	if (!h) return 0;
	Voice& v = g_voices[(size_t)h - 1];
	std::vector<unsigned char> whole;
	WavInfo w;
	if (!loadWavFile(file, whole, w) || w.format != 1 || !alFormatFor(w)) {
		freeVoiceBuffers(v); v.used = false; v.kind = VK_NONE;
		return 0;
	}
	v.pcm.assign(w.data, w.data + w.dataLen);
	v.rate = w.rate;
	alGenBuffers(1, &v.buffer);
	alBufferData(v.buffer, alFormatFor(w), v.pcm.data(), (ALsizei)v.pcm.size(), (ALsizei)w.rate);
	alSourcei(v.source, AL_BUFFER, (ALint)v.buffer);
	alSourcei(v.source, AL_SOURCE_RELATIVE, AL_TRUE);
	// loop: 0 = infinite per Miles quick API? Generals passes 1 for one-shots → treat 0 as infinite
	if (loop == 0) alSourcei(v.source, AL_LOOPING, AL_TRUE);
	applyGain(v);
	alSourcePlay(v.source);
	v.started = true;
	return h;
}
extern "C" void AIL_quick_set_volume(HAUDIO h, float left, float) {
	CsLock lk;
	if (Voice* v = voiceFromHandleAny((uintptr_t)h)) { v->volume = left; applyGain(*v); }
}
extern "C" void AIL_quick_unload(HAUDIO h) {
	CsLock lk;
	if (Voice* v = voiceFromHandleAny((uintptr_t)h)) { freeVoiceBuffers(*v); v->used = false; v->kind = VK_NONE; }
}

// --- Memory / WAV helpers --------------------------------------------------

extern "C" void AIL_mem_free_lock(void* p) { free(p); }

extern "C" int AIL_WAV_info(void* buf, AILSOUNDINFO* info) {
	if (!buf || !info) return 0;
	WavInfo w;
	if (!parseWav((const unsigned char*)buf, (size_t)-1, w)) return 0;
	info->format = (w.format == 1) ? 1u : (w.format == 0xFFFE ? 1u : w.format);
	info->data = (void*)w.data;
	info->data_byte_len = w.dataLen;
	info->rate = (float)w.rate;
	info->bits = w.bits;
	info->channels = w.channels;
	info->samples = w.dataLen / (w.channels ? w.channels : 1) / (w.bits ? w.bits / 8 : 1);
	info->block_size = (U32)(w.channels * (w.bits / 8));
	info->initial_ptr = info->data;
	return 1;
}

extern "C" int AIL_decompress_ADPCM(AILSOUNDINFO* in, void** out, U32* out_size) {
	// No ADPCM decoder yet: hand back a silence buffer of the same size so the
	// file cache bookkeeping stays consistent (audio will be silent for IMA WAVs).
	if (!in || !out || !out_size) return 0;
	U32 n = in->data_byte_len ? in->data_byte_len : 4096;
	if (n > 64u * 1024u * 1024u) n = 64u * 1024u * 1024u;
	*out = calloc(1, n);
	if (!*out) return 0;
	*out_size = n;
	return 1;
}

extern "C" void AIL_get_DirectSound_info(HSAMPLE, AILLPDIRECTSOUND* ds, void** buf) {
	if (ds) *ds = nullptr;
	if (buf) *buf = nullptr;
}

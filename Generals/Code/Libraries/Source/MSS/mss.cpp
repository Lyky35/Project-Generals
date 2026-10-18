// MSS/mss.cpp — Miles Sound System over OpenAL Soft.
// Single voice pool shared by 2D samples, 3D samples, streams and quick one-shots.
// Handles: HSAMPLE/HSTREAM/HAUDIO = 1-based pool index; H3DSAMPLE = (void*)(uintptr_t)index.
#include "MSS/mss.h"
#include "rts_bootlog.h"
struct BootMarkMss { BootMarkMss() { rtsBootLog("boot:after-mss-statics"); } };
__attribute__((init_priority(250))) static BootMarkMss boot_mark_mss;

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
	bool failed = false;	// set-file failed: complete on next AIL_service (EOS) so the slot can't leak
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
	U16 blockAlign = 0;
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
			out.blockAlign = rd16(buf + body + 12);
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
			if (v.failed) {
				// Port: set-file failed voice - complete immediately so the
				// manager's EOS path releases the pool slot.
				v.failed = false;
				v.started = false;
				if (n < 64) {
					fired[n].kind = v.kind;
					fired[n].id = i + 1;
					fired[n].c2 = v.eos2d;
					fired[n].c3 = v.eos3d;
					fired[n].cs = v.eosStream;
					++n;
				}
				continue;
			}
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
	{
		bool ok = uploadWavToVoice(*v, (const unsigned char*)file, (size_t)-1, offset) != 0;
		v->failed = !ok;
		if (!ok) {
			static unsigned f2 = 0; f2++;
			if (f2 <= 30) { FILE* lf = fopen("RTS_START.LOG", "a"); if (lf) { fprintf(lf, "VK: audio set_file FAILED 2D #%u\n", f2); fclose(lf); } }
		}
		return ok ? 1 : 0;
	}
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
		if (v->failed || !v->buffer) {
			// Port: no buffer (parse/decode failed) - mark started so the
			// next AIL_service fires EOS and the manager releases the slot
			// (previously such voices never completed = permanent pool leak).
			v->started = true;
			v->paused = false;
			return;
		}
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
	bool ok = uploadWavToVoice(*v, (const unsigned char*)file, (size_t)-1, 0) != 0;
	v->failed = !ok;
	if (!ok) {
		static unsigned f3 = 0; f3++;
		if (f3 <= 30) { FILE* lf = fopen("RTS_START.LOG", "a"); if (lf) { fprintf(lf, "VK: audio set_file FAILED 3D #%u\n", f3); fclose(lf); } }
	}
	return ok ? 1 : 0;
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
	// bits<8 (IMA-ADPCM=4) makes w.bits/8 == 0 -> integer divide-by-zero
	{
		unsigned bytesPerSample = (w.bits >= 8) ? (w.bits / 8) : 1;
		unsigned ch = w.channels ? w.channels : 1;
		info->samples = w.dataLen / ch / bytesPerSample;
	}
	if (w.format == 0x11 /*IMA ADPCM*/ && w.blockAlign) {
		// Report the real nBlockAlign: the decompressor needs it to walk
		// ADPCM blocks (the old byte-per-sample formula yielded 1).
		info->block_size = w.blockAlign;
	} else {
		info->block_size = (U32)(w.channels * ((w.bits >= 8) ? (w.bits / 8) : 1));
	}
	info->initial_ptr = info->data;
	return 1;
}

// --- IMA/DVI ADPCM (WAVE_FORMAT_IMA_ADPCM) decoder -------------------------
// Port: explosion/missile/UI SFX in Audio.big are mostly IMA ADPCM (fmt 0x11);
// the old stub returned a headerless silence buffer, so uploadWavToVoice's
// parseWav failed -> no AL buffer -> silent AND the voice never reached EOS,
// permanently leaking 3D sample-pool slots (after a few explosions every
// world sound got channel-culled).
static const int kImaStepTable[89] = {
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
	337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
	2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767 };
static const int kImaIndexTable[16] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };

static inline int imaClamp16(int v) { return v < -32768 ? -32768 : (v > 32767 ? 32767 : v); }

extern "C" int AIL_decompress_ADPCM(AILSOUNDINFO* in, void** out, U32* out_size) {
	if (!in || !out || !out_size || !in->data || !in->data_byte_len) return 0;
	unsigned ch = in->channels ? in->channels : 1;
	if (ch > 2) return 0;
	U32 blockAlign = in->block_size;
	if (blockAlign < (U32)(4 * ch) || blockAlign > 4096) return 0;
	U32 samplesPerBlock = ((blockAlign - 4 * ch) * 8) / (4 * ch) + 1;
	U32 blocks = in->data_byte_len / blockAlign;
	if (blocks == 0) return 0;
	U32 totalSamples = blocks * samplesPerBlock * ch;	// int16 frames, interleaved
	if (totalSamples == 0 || totalSamples > 64u * 1024u * 1024u) return 0;

	// Output a full RIFF PCM16 WAV - uploadWavToVoice parseWav's the result.
	U32 dataBytes = totalSamples * 2;
	U32 riffSize = 36 + dataBytes;
	unsigned char* wav = (unsigned char*)malloc(44 + dataBytes);
	if (!wav) return 0;
	memcpy(wav, "RIFF", 4);
	wav[4] = (unsigned char)(riffSize); wav[5] = (unsigned char)(riffSize >> 8);
	wav[6] = (unsigned char)(riffSize >> 16); wav[7] = (unsigned char)(riffSize >> 24);
	memcpy(wav + 8, "WAVEfmt ", 8);
	wav[16] = 16; wav[17] = 0; wav[18] = 0; wav[19] = 0;		// fmt chunk size
	wav[20] = 1; wav[21] = 0;									// PCM
	wav[22] = (unsigned char)ch; wav[23] = 0;
	U32 rate = in->rate ? in->rate : 22050;
	wav[24] = (unsigned char)rate; wav[25] = (unsigned char)(rate >> 8);
	wav[26] = (unsigned char)(rate >> 16); wav[27] = (unsigned char)(rate >> 24);
	U32 byteRate = rate * ch * 2;
	wav[28] = (unsigned char)byteRate; wav[29] = (unsigned char)(byteRate >> 8);
	wav[30] = (unsigned char)(byteRate >> 16); wav[31] = (unsigned char)(byteRate >> 24);
	U16 blk = (U16)(ch * 2);
	wav[32] = (unsigned char)blk; wav[33] = 0;
	wav[34] = 16; wav[35] = 0;
	memcpy(wav + 36, "data", 4);
	wav[40] = (unsigned char)(dataBytes); wav[41] = (unsigned char)(dataBytes >> 8);
	wav[42] = (unsigned char)(dataBytes >> 16); wav[43] = (unsigned char)(dataBytes >> 24);
	short* pcm = (short*)(wav + 44);

	const unsigned char* src = (const unsigned char*)in->data;
	int pred[2] = { 0, 0 };
	int idx[2] = { 0, 0 };
	for (U32 b = 0; b < blocks; ++b) {
		const unsigned char* block = src + b * blockAlign;
		for (unsigned c = 0; c < ch; ++c) {
			const unsigned char* h = block + c * 4;
			pred[c] = (short)(h[0] | (h[1] << 8));
			idx[c] = h[2];
			if (idx[c] > 88) idx[c] = 88;
		}
		// first (header) sample per channel
		for (unsigned c = 0; c < ch; ++c)
			*pcm++ = (short)pred[c];
		U32 dataBytesInBlock = blockAlign - 4 * ch;
		if (ch == 1) {
			for (U32 i = 0; i < dataBytesInBlock; ++i) {
				unsigned char byte = block[4 + i];
				for (int half = 0; half < 2; ++half) {
					int nib = (half == 0) ? (byte & 0x0f) : ((byte >> 4) & 0x0f);
					int step = kImaStepTable[idx[0]];
					int diff = step >> 3;
					if (nib & 1) diff += step >> 2;
					if (nib & 2) diff += step >> 1;
					if (nib & 4) diff += step;
					pred[0] = imaClamp16((nib & 8) ? (pred[0] - diff) : (pred[0] + diff));
					idx[0] += kImaIndexTable[nib];
					if (idx[0] < 0) idx[0] = 0; else if (idx[0] > 88) idx[0] = 88;
					*pcm++ = (short)pred[0];
				}
			}
		} else {
			for (U32 i = 0; i < dataBytesInBlock; ++i) {
				unsigned char byte = block[4 + ch * 0 + i];	// headers are per-channel at block start
				// MS IMA stereo layout: after BOTH channel headers, bytes
				// interleave nibbles: low = ch0, high = ch1.
				int n0 = byte & 0x0f;
				int n1 = (byte >> 4) & 0x0f;
				for (unsigned c = 0; c < 2; ++c) {
					int nib = (c == 0) ? n0 : n1;
					int step = kImaStepTable[idx[c]];
					int diff = step >> 3;
					if (nib & 1) diff += step >> 2;
					if (nib & 2) diff += step >> 1;
					if (nib & 4) diff += step;
					pred[c] = imaClamp16((nib & 8) ? (pred[c] - diff) : (pred[c] + diff));
					idx[c] += kImaIndexTable[nib];
					if (idx[c] < 0) idx[c] = 0; else if (idx[c] > 88) idx[c] = 88;
					*pcm++ = (short)pred[c];
				}
			}
		}
	}
	*out = wav;
	*out_size = 44 + dataBytes;
	return 1;
}

extern "C" void AIL_get_DirectSound_info(HSAMPLE, AILLPDIRECTSOUND* ds, void** buf) {
	if (ds) *ds = nullptr;
	if (buf) *buf = nullptr;
}

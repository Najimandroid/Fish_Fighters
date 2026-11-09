// Fish Fighters' sound effects, made as code: synthesized layers (bubbles, splashes, zaps, metal clangs, booms)
// and a few SoundFont notes (GeneralUser GS) for the musical ones, all with an underwater touch.
// Writes assets/audios/sfx/<name>[_<variant>].ogg and assets/audios/sfx/sfx.json, the game's list of sounds:
//   volume (0-1), pitch (random +- variation), cooldown (seconds before it can play again), maxInstances.
//
// Usage: node tools/music/make_sfx.js     (needs ffmpeg on the PATH and the SoundFont, like render_stems.js)

const fs = require("fs");
const os = require("os");
const path = require("path");
const { execFileSync } = require("child_process");
const { SoundFont, render } = require("./sf2_render");

const ROOT = path.join(__dirname, "..", "..");
const OUT = path.join(ROOT, "assets", "audios", "sfx");
const SOUNDFONT = path.join(ROOT, "audio_sources", "tools", "soundfont", "GeneralUser-GS.sf2");
const RATE = 48000;
const font = new SoundFont(SOUNDFONT);

// ===== Randomness (seeded, so the files are the same every time) =====
let state = 1;
const seed = n => { state = (n >>> 0) || 1; };
const random = () => { state ^= state << 13; state ^= state >>> 17; state ^= state << 5; return (state >>> 0) / 4294967296; };

// ===== A sound being built: interleaved stereo =====
class Sound {
    constructor(seconds) { this.data = new Float32Array(Math.ceil(seconds * RATE) * 2); }
    add(frame, left, right) {
        if (frame >= 0 && frame * 2 + 1 < this.data.length) { this.data[frame * 2] += left; this.data[frame * 2 + 1] += right; }
    }
}
const panGains = pan => [Math.cos((pan + 1) * Math.PI / 4) * Math.SQRT2, Math.sin((pan + 1) * Math.PI / 4) * Math.SQRT2];
const value = (v, t) => typeof v === "function" ? v(t) : v;

// RBJ biquad coefficients
function biquad(type, freq, q = 0.707) {
    const w = 2 * Math.PI * Math.min(freq, RATE * 0.45) / RATE, alpha = Math.sin(w) / (2 * q), cos = Math.cos(w), a0 = 1 + alpha;
    if (type === "lowpass") return [(1 - cos) / 2 / a0, (1 - cos) / a0, (1 - cos) / 2 / a0, -2 * cos / a0, (1 - alpha) / a0];
    if (type === "highpass") return [(1 + cos) / 2 / a0, -(1 + cos) / a0, (1 + cos) / 2 / a0, -2 * cos / a0, (1 - alpha) / a0];
    return [alpha / a0, 0, -alpha / a0, -2 * cos / a0, (1 - alpha) / a0];   // band-pass
}
class Filter {
    constructor() { this.x1 = this.x2 = this.y1 = this.y2 = 0; }
    run([b0, b1, b2, a1, a2], x) {
        const y = b0 * x + b1 * this.x1 + b2 * this.x2 - a1 * this.y1 - a2 * this.y2;
        this.x2 = this.x1; this.x1 = x; this.y2 = this.y1; this.y1 = y;
        return y;
    }
}

// Envelope: attack, then an exponential decay (tau) or held, with a short release at the end
function envelope(t, dur, attack, tau, release) {
    let e = Math.min(1, t / Math.max(attack, 1e-4)) * (tau ? Math.exp(-t / tau) : 1);
    if (t > dur - release) e *= Math.max(0, (dur - t) / release);
    return e;
}

// ===== Layers =====

// A tone. freq: Hz or t -> Hz. wave: sine, triangle, square, saw (square and saw are meant to be filtered)
function tone(s, { at = 0, dur, freq, wave = "sine", amp = 0.5, attack = 0.003, tau = null, release = 0.02, pan = 0, vibrato = 0, vibratoHz = 6, filter = null }) {
    const [gl, gr] = panGains(pan), first = Math.round(at * RATE), f = new Filter();
    let phase = 0, coefficients = null;
    for (let i = 0; i < Math.round(dur * RATE); i++) {
        const t = i / RATE;
        phase += value(freq, t) * (1 + vibrato * Math.sin(2 * Math.PI * vibratoHz * t)) / RATE;
        const p = phase % 1;
        let v = wave === "sine" ? Math.sin(2 * Math.PI * p) : wave === "square" ? (p < 0.5 ? 0.6 : -0.6)
            : wave === "saw" ? (2 * p - 1) * 0.6 : 1 - 4 * Math.abs(p - 0.5);
        if (filter) {
            if (i % 32 === 0) coefficients = biquad(filter.type, value(filter.freq, t), filter.q || 0.707);
            v = f.run(coefficients, v);
        }
        const e = envelope(t, dur, attack, tau, release) * amp * v;
        s.add(first + i, e * gl, e * gr);
    }
}

// Filtered noise (splashes, whooshes, crunches). filter.freq can follow t
function noise(s, { at = 0, dur, amp = 0.5, attack = 0.002, tau = null, release = 0.02, filter = null, pan = 0, width = 0.3 }) {
    const [gl, gr] = panGains(pan), first = Math.round(at * RATE), fl = new Filter(), fr = new Filter();
    let coefficients = null;
    for (let i = 0; i < Math.round(dur * RATE); i++) {
        const t = i / RATE;
        const center = random() * 2 - 1;
        let l = center * (1 - width) + (random() * 2 - 1) * width, r = center * (1 - width) + (random() * 2 - 1) * width;
        if (filter) {
            if (i % 32 === 0) coefficients = biquad(filter.type, value(filter.freq, t), filter.q || 0.707);
            l = fl.run(coefficients, l); r = fr.run(coefficients, r);
        }
        const e = envelope(t, dur, attack, tau, release) * amp;
        s.add(first + i, l * e * gl, r * e * gr);
    }
}

// A bubble: a sine rising as it shrinks (like the music's bubbles)
function bubble(s, { at = 0, freq = 800, size = 0.06, amp = 0.35, pan = 0, rise = 1.3 }) {
    const [gl, gr] = panGains(pan), first = Math.round(at * RATE);
    let phase = 0;
    for (let i = 0; i < Math.round(size * 4 * RATE); i++) {
        const t = i / RATE;
        phase += 2 * Math.PI * freq * Math.pow(2, rise * Math.min(t / size, 1.6)) / RATE;
        const v = Math.sin(phase) * amp * (1 - Math.exp(-t / 0.0015)) * Math.exp(-t / (size * 0.45));
        s.add(first + i, v * gl, v * gr);
    }
}
function bubbles(s, { at = 0, count = 6, spread = 0.4, low = 500, high = 1500, size = [0.03, 0.08], amp = 0.25 }) {
    for (let i = 0; i < count; i++) {
        bubble(s, { at: at + spread * (i + random() * 0.8) / count, freq: low * Math.pow(high / low, (i + random()) / count),
            size: size[0] + (size[1] - size[0]) * random(), amp: amp * (0.6 + 0.4 * random()), pan: (random() - 0.5) * 1.2 });
    }
}

// A metallic strike: frequency modulation with an inharmonic ratio (coins, clangs, latches)
function bell(s, { at = 0, freq, ratio = 2.76, index = 3, tau = 0.3, amp = 0.4, pan = 0 }) {
    const [gl, gr] = panGains(pan), first = Math.round(at * RATE);
    for (let i = 0; i < Math.round(tau * 6 * RATE); i++) {
        const t = i / RATE;
        const v = Math.sin(2 * Math.PI * freq * t + index * Math.exp(-t / (tau * 0.4)) * Math.sin(2 * Math.PI * freq * ratio * t));
        const e = v * amp * Math.min(1, t / 0.001) * Math.exp(-t / tau);
        s.add(first + i, e * gl, e * gr);
    }
}

// An electric zap: band-passed crackles, a clipped buzz and a falling whine (like the jellyfish music's zaps)
function zap(s, { at = 0, dur = 0.25, amp = 0.5, pan = 0 }) {
    const [gl, gr] = panGains(pan), first = Math.round(at * RATE);
    const buzzHz = 90 + random() * 60, whineFrom = 2500 + random() * 2500, f = 2 * Math.sin(Math.PI * 3500 / RATE);
    let low = 0, band = 0, crackle = 0, buzzPhase = 0, whinePhase = 0;
    for (let i = 0; i < Math.round(dur * RATE); i++) {
        const t = i / RATE, x = t / dur;
        crackle = random() < 0.03 * (1 - x) + 0.003 ? random() * 2 - 1 : crackle * 0.7;
        low += f * band;
        band += f * (crackle - low - 0.7 * band);
        buzzPhase += 2 * Math.PI * buzzHz * (1 + 0.1 * Math.sin(t * 60)) / RATE;
        whinePhase += 2 * Math.PI * whineFrom * Math.pow(0.15, x) / RATE;
        const v = amp * (1 - Math.exp(-t / 0.002)) * Math.pow(1 - x, 1.5) *
            (band * 2.5 + Math.tanh(4 * Math.sin(buzzPhase)) * 0.35 * (random() < 0.5 ? 1 : 0.6) + Math.sin(whinePhase) * 0.25 * Math.exp(-t / (dur * 0.3)));
        s.add(first + i, v * gl, v * gr);
    }
}

// A SoundFont note (program on bank 0, or a drum kit note with drums: true)
function note(s, { at = 0, program = 0, key, velocity = 100, len = 0.3, drums = false, amp = 1, pan = 0, bend = 0 }) {
    const x = render(font, { notes: [[0, len, key, velocity, bend]], channel: drums ? 10 : 1, program, volume: 127 }, RATE, 2);
    const [gl, gr] = panGains(pan), first = Math.round(at * RATE);
    for (let i = 0; i < x.length / 2; i++) s.add(first + i, x[i * 2] * amp * gl, x[i * 2 + 1] * amp * gr);
}

// ===== Whole-sound effects =====
function filterAll(s, type, freq, q) {
    const c = biquad(type, freq, q), fl = new Filter(), fr = new Filter();
    for (let i = 0; i < s.data.length; i += 2) { s.data[i] = fl.run(c, s.data[i]); s.data[i + 1] = fr.run(c, s.data[i + 1]); }
}
function softClip(s, drive = 1.5) {
    for (let i = 0; i < s.data.length; i++) s.data[i] = Math.tanh(s.data[i] * drive) / Math.tanh(drive);
}
// A small watery room (Freeverb-like: 4 damped combs and 2 all-passes per side)
function reverb(s, mix = 0.15, size = 0.8) {
    const dry = s.data.slice();
    for (let ch = 0; ch < 2; ch++) {
        const combs = [1116, 1188, 1277, 1356].map(d => ({ buffer: new Float32Array(Math.round((d + ch * 23) * size * RATE / 44100)), index: 0, store: 0 }));
        const passes = [556, 441].map(d => ({ buffer: new Float32Array(Math.round((d + ch * 23) * RATE / 44100)), index: 0 }));
        for (let i = ch; i < s.data.length; i += 2) {
            const input = dry[i] * 0.3;
            let out = 0;
            for (const c of combs) {
                const y = c.buffer[c.index];
                c.store = y * 0.6 + c.store * 0.4;                   // damping
                c.buffer[c.index] = input + c.store * 0.82;
                c.index = (c.index + 1) % c.buffer.length;
                out += y;
            }
            for (const p of passes) {
                const b = p.buffer[p.index];
                p.buffer[p.index] = out + b * 0.5;
                out = b - out;
                p.index = (p.index + 1) % p.buffer.length;
            }
            s.data[i] = dry[i] * (1 - mix * 0.5) + out * mix;
        }
    }
}

// Every sound at the same loudness (its loudest 100 ms at -14 dB RMS), peaks under -4 dBFS (the Vorbis encoding
// overshoots by up to 2 dB, and the game decodes to 16 bits), trailing silence trimmed with a short fade.
// The relative levels are sfx.json's volumes.
function finish(s) {
    let peak = 0, loudest = 0;
    for (const v of s.data) peak = Math.max(peak, Math.abs(v));
    const window = RATE / 10;
    for (let start = 0; start + window <= s.data.length / 2; start += window / 4) {
        let sum = 0;
        for (let i = start * 2; i < (start + window) * 2; i++) sum += s.data[i] * s.data[i];
        loudest = Math.max(loudest, Math.sqrt(sum / (window * 2)));
    }
    const gain = peak > 0 ? Math.min(Math.pow(10, -14 / 20) / loudest, Math.pow(10, -4 / 20) / peak) : 1;
    let frames = s.data.length / 2;
    while (frames > 0 && Math.abs(s.data[frames * 2 - 1] * gain) < 3e-4 && Math.abs(s.data[frames * 2 - 2] * gain) < 3e-4) frames--;
    frames = Math.min(s.data.length / 2, frames + RATE / 100);
    const out = s.data.slice(0, frames * 2);
    const fade = Math.min(frames, RATE / 100);
    for (let i = 0; i < out.length; i++) out[i] *= gain;
    for (let i = 0; i < fade; i++) for (let ch = 0; ch < 2; ch++) out[(frames - fade + i) * 2 + ch] *= 1 - i / fade;
    return out;
}

// ===== The sounds =====
// Each: [name, settings for sfx.json, variants (functions building a Sound, given a pitch factor)]
const SOUNDS = [];
const define = (name, settings, variants, build) => SOUNDS.push({ name, settings, variants, build });
const PITCHES = [1, 0.92, 1.09];

// ----- Menus -----
define("ui_click", { volume: 0.55, pitch: 0.04, cooldown: 0.03, maxInstances: 2 }, 1, p => {
    const s = new Sound(0.4);
    tone(s, { dur: 0.06, freq: t => 950 * p * Math.pow(1.5, t / 0.06), amp: 0.35, tau: 0.025 });          // a bubbly blip, rising
    noise(s, { dur: 0.012, amp: 0.25, tau: 0.003, filter: { type: "highpass", freq: 3000 } });           // the click
    bubble(s, { at: 0.02, freq: 1400 * p, size: 0.03, amp: 0.15 });
    reverb(s, 0.12);
    return s;
});
define("ui_back", { volume: 0.5, pitch: 0.03, cooldown: 0.03, maxInstances: 2 }, 1, p => {
    const s = new Sound(0.4);
    tone(s, { dur: 0.08, freq: t => 820 * p * Math.pow(0.6, t / 0.08), amp: 0.35, tau: 0.035 });         // the blip, falling
    noise(s, { dur: 0.012, amp: 0.2, tau: 0.003, filter: { type: "highpass", freq: 2500 } });
    bubble(s, { at: 0.03, freq: 700, size: 0.04, amp: 0.12 });
    reverb(s, 0.12);
    return s;
});
define("ui_select", { volume: 0.45, pitch: 0.05, cooldown: 0.03, maxInstances: 2 }, 1, p => {
    const s = new Sound(0.6);
    note(s, { program: 108, key: 84, velocity: 90, len: 0.12, amp: 0.8 });                              // a kalimba tink
    bubble(s, { at: 0.01, freq: 1600 * p, size: 0.025, amp: 0.12 });
    reverb(s, 0.15);
    return s;
});
define("ui_transition", { volume: 0.4, pitch: 0.03, cooldown: 0.1, maxInstances: 1 }, 1, p => {
    const s = new Sound(0.9);
    noise(s, { dur: 0.45, amp: 0.5, attack: 0.12, release: 0.25, filter: { type: "bandpass", q: 1.2,
        freq: t => 300 + 2200 * Math.sin(Math.PI * Math.min(1, t / 0.45)) } });                          // a swish of water
    bubbles(s, { at: 0.05, count: 8, spread: 0.4, low: 450, high: 1600, amp: 0.2 });
    reverb(s, 0.2);
    return s;
});
define("ui_error", { volume: 0.55, pitch: 0, cooldown: 0.15, maxInstances: 1 }, 1, () => {
    const s = new Sound(0.6);
    const lp = { type: "lowpass", freq: 1100 };
    tone(s, { dur: 0.09, freq: 196, wave: "square", amp: 0.5, tau: 0.06, filter: lp });                 // bonk...
    tone(s, { at: 0.12, dur: 0.14, freq: 156, wave: "square", amp: 0.5, tau: 0.08, filter: lp });       // ...bonk
    note(s, { program: 115, key: 60, velocity: 90, len: 0.1, amp: 0.4 });
    reverb(s, 0.1);
    return s;
});
define("ui_pause", { volume: 0.5, pitch: 0, cooldown: 0.1, maxInstances: 1 }, 1, () => {
    const s = new Sound(1.2);
    tone(s, { dur: 0.5, freq: t => 1150 * Math.pow(0.85, t / 0.5), amp: 0.3, tau: 0.18 });              // a soft sonar ping
    tone(s, { at: 0.18, dur: 0.4, freq: 980, amp: 0.1, tau: 0.15 });                                    // its echo
    noise(s, { dur: 0.2, amp: 0.15, attack: 0.05, filter: { type: "lowpass", freq: 600 } });
    reverb(s, 0.25);
    return s;
});
define("ui_unpause", { volume: 0.5, pitch: 0, cooldown: 0.1, maxInstances: 1 }, 1, () => {
    const s = new Sound(0.8);
    tone(s, { dur: 0.25, freq: t => 880 * Math.pow(1.4, t / 0.25), amp: 0.3, tau: 0.12 });
    bubbles(s, { at: 0.05, count: 4, spread: 0.2, low: 700, high: 1500, amp: 0.15 });
    reverb(s, 0.2);
    return s;
});
define("ui_start", { volume: 0.9, pitch: 0, cooldown: 0.5, maxInstances: 1 }, 1, () => {
    const s = new Sound(2.5);
    noise(s, { dur: 0.6, amp: 0.6, tau: 0.18, filter: { type: "lowpass", freq: t => 3500 * Math.pow(0.2, t / 0.6) } });   // splash
    [72, 77, 81, 84, 89].forEach((key, i) => note(s, { at: 0.05 + i * 0.06, program: 114, key, velocity: 105, len: 0.4, amp: 0.7, pan: -0.3 + i * 0.15 }));
    note(s, { at: 0.35, program: 9, key: 89, velocity: 90, len: 0.6, amp: 0.5 });
    bubbles(s, { at: 0.1, count: 16, spread: 0.9, low: 400, high: 2200, amp: 0.25 });
    reverb(s, 0.25);
    return s;
});

// ----- Fish Tank -----
define("purchase", { volume: 0.8, pitch: 0, cooldown: 0.1, maxInstances: 2 }, 1, () => {
    const s = new Sound(1.6);
    [2200, 2650, 3150].forEach((freq, i) => bell(s, { at: i * 0.07, freq, ratio: 1.41, index: 2, tau: 0.12, amp: 0.35, pan: -0.3 + i * 0.3 }));   // shells clinking
    [89, 93, 96].forEach((key, i) => note(s, { at: 0.2 + i * 0.06, program: 9, key, velocity: 95, len: 0.3, amp: 0.6 }));                     // sparkle
    bubbles(s, { at: 0.15, count: 6, spread: 0.4, low: 800, high: 2000, amp: 0.15 });
    reverb(s, 0.2);
    return s;
});
define("unlock", { volume: 0.9, pitch: 0, cooldown: 0.5, maxInstances: 1 }, 1, () => {
    const s = new Sound(3);
    [65, 69, 72, 77, 81].forEach((key, i) => note(s, { at: i * 0.08, program: 114, key, velocity: 110, len: 0.5, amp: 0.7 }));   // F major on steel drums
    [53, 57, 60, 65].forEach(key => note(s, { at: 0.4, program: 61, key, velocity: 100, len: 0.9, amp: 0.45 }));                 // brass chord
    note(s, { at: 0.4, program: 9, key: 89, velocity: 100, len: 1, amp: 0.5 });
    bell(s, { at: 0.4, freq: 2800, ratio: 1.41, index: 1.5, tau: 0.5, amp: 0.2 });
    bubbles(s, { at: 0.35, count: 20, spread: 1, low: 400, high: 2400, amp: 0.22 });
    reverb(s, 0.25);
    return s;
});
define("equip", { volume: 0.6, pitch: 0.04, cooldown: 0.05, maxInstances: 2 }, 1, p => {
    const s = new Sound(0.7);
    tone(s, { dur: 0.12, freq: t => 140 * p * Math.pow(0.45, t / 0.12), amp: 0.6, tau: 0.05 });       // thump
    bell(s, { freq: 420 * p, ratio: 2.41, index: 4, tau: 0.08, amp: 0.3 });                             // metal latch
    bell(s, { at: 0.005, freq: 1150 * p, ratio: 1.73, index: 2, tau: 0.05, amp: 0.2 });
    bubble(s, { at: 0.05, freq: 600, size: 0.05, amp: 0.15 });
    reverb(s, 0.15);
    return s;
});
define("unequip", { volume: 0.5, pitch: 0.04, cooldown: 0.05, maxInstances: 2 }, 1, p => {
    const s = new Sound(0.6);
    bell(s, { freq: 900 * p, ratio: 1.73, index: 2, tau: 0.05, amp: 0.25 });
    bubble(s, { at: 0.02, freq: 500 * p, size: 0.07, amp: 0.3, rise: 1.6 });                            // pops out
    noise(s, { dur: 0.15, amp: 0.15, attack: 0.03, filter: { type: "bandpass", freq: 1200, q: 1 } });
    reverb(s, 0.15);
    return s;
});
define("battle_start", { volume: 0.8, pitch: 0, cooldown: 1, maxInstances: 1 }, 1, () => {
    const s = new Sound(2.6);
    // The submarine dives: "AH-OO-GAH" klaxon, then the splash and the bubbles
    const klaxon = t => 230 + 190 * Math.min(1, t / 0.32) * (1 - Math.exp(-t / 0.08)) - (t > 0.45 ? 25 * (t - 0.45) : 0);
    tone(s, { dur: 0.85, freq: klaxon, wave: "saw", amp: 0.55, attack: 0.02, release: 0.1, filter: { type: "bandpass", q: 1.4, freq: t => 700 + 500 * Math.min(1, t / 0.35) } });
    tone(s, { dur: 0.85, freq: t => klaxon(t) * 1.505, wave: "saw", amp: 0.25, attack: 0.02, release: 0.1, filter: { type: "lowpass", freq: 1800 } });
    softClip(s, 2);
    noise(s, { at: 0.8, dur: 0.7, amp: 0.45, tau: 0.25, filter: { type: "lowpass", freq: t => 2500 * Math.pow(0.15, t / 0.7) } });
    bubbles(s, { at: 0.85, count: 22, spread: 1.2, low: 300, high: 1800, amp: 0.25 });
    reverb(s, 0.2);
    return s;
});

// ----- Battle -----
define("deploy", { volume: 0.6, pitch: 0.06, cooldown: 0.05, maxInstances: 3 }, 2, p => {
    const s = new Sound(0.8);
    bubble(s, { freq: 340 * p, size: 0.09, amp: 0.55, rise: 1.6 });                                      // bloop
    noise(s, { dur: 0.18, amp: 0.25, tau: 0.05, filter: { type: "bandpass", freq: 1500, q: 0.9 } });      // splash
    bubbles(s, { at: 0.08, count: 3, spread: 0.2, low: 900, high: 1600, amp: 0.15 });
    reverb(s, 0.15);
    return s;
});
define("cash_up", { volume: 0.75, pitch: 0, cooldown: 0.1, maxInstances: 1 }, 1, () => {
    const s = new Sound(1.4);
    noise(s, { dur: 0.04, amp: 0.3, tau: 0.01, filter: { type: "highpass", freq: 2000 } });             // ka-
    [2500, 3300].forEach((freq, i) => bell(s, { at: 0.03 + i * 0.08, freq, ratio: 1.41, index: 2, tau: 0.15, amp: 0.35 }));   // -ching
    [84, 88, 91, 96].forEach((key, i) => note(s, { at: 0.12 + i * 0.05, program: 114, key, velocity: 100, len: 0.25, amp: 0.55 }));
    reverb(s, 0.18);
    return s;
});

// Attacks: one sound per kind, chosen per fish / enemy in the editor ("attackSound"), 3 variants each
define("attack_slap", { volume: 0.45, pitch: 0.06, cooldown: 0.04, maxInstances: 6 }, 3, p => {
    const s = new Sound(0.5);
    noise(s, { dur: 0.05, amp: 0.55, tau: 0.015, filter: { type: "highpass", freq: 1500 } });             // the slap
    tone(s, { dur: 0.12, freq: t => 230 * p * Math.pow(0.6, t / 0.12), amp: 0.5, tau: 0.04 });          // thwack
    noise(s, { at: 0.01, dur: 0.15, amp: 0.2, tau: 0.05, filter: { type: "lowpass", freq: 900 } });      // wet
    reverb(s, 0.1);
    return s;
});
define("attack_bite", { volume: 0.45, pitch: 0.06, cooldown: 0.04, maxInstances: 6 }, 3, p => {
    const s = new Sound(0.5);
    [0, 0.045].forEach(at => noise(s, { at, dur: 0.05, amp: 0.6, tau: 0.015, filter: { type: "bandpass", freq: 2500 * p, q: 1.2 } }));   // chomp chomp
    tone(s, { dur: 0.1, freq: t => 180 * p * Math.pow(0.6, t / 0.1), amp: 0.45, tau: 0.03 });
    softClip(s, 1.8);
    reverb(s, 0.08);
    return s;
});
define("attack_pinch", { volume: 0.45, pitch: 0.06, cooldown: 0.04, maxInstances: 6 }, 3, p => {
    const s = new Sound(0.4);
    [0, 0.035].forEach((at, i) => {                                                                       // snap snap
        noise(s, { at, dur: 0.02, amp: 0.6, tau: 0.005, filter: { type: "bandpass", freq: 4000 * p * (1 - i * 0.15), q: 3 } });
        tone(s, { at, dur: 0.03, freq: 2200 * p * (1 - i * 0.15), amp: 0.25, tau: 0.008 });
    });
    tone(s, { dur: 0.06, freq: 300 * p, amp: 0.3, tau: 0.02 });
    reverb(s, 0.1);
    return s;
});
define("attack_zap", { volume: 0.45, pitch: 0.05, cooldown: 0.04, maxInstances: 6 }, 3, () => {
    const s = new Sound(0.5);
    zap(s, { dur: 0.18, amp: 0.7 });
    bubble(s, { at: 0.03, freq: 900, size: 0.03, amp: 0.12 });
    reverb(s, 0.12);
    return s;
});
define("attack_heavy", { volume: 0.5, pitch: 0.05, cooldown: 0.05, maxInstances: 4 }, 3, p => {
    const s = new Sound(0.9);
    tone(s, { dur: 0.35, freq: t => 110 * p * Math.pow(0.4, t / 0.35), amp: 0.7, tau: 0.12 });          // a big thump
    noise(s, { dur: 0.25, amp: 0.45, tau: 0.08, filter: { type: "lowpass", freq: t => 1200 * Math.pow(0.25, t / 0.25) } });
    note(s, { program: 116, key: 45, velocity: 110, len: 0.3, amp: 0.5 });
    softClip(s, 1.5);
    reverb(s, 0.15);
    return s;
});
define("attack_squirt", { volume: 0.45, pitch: 0.06, cooldown: 0.04, maxInstances: 6 }, 3, p => {
    const s = new Sound(0.6);
    noise(s, { dur: 0.22, amp: 0.6, attack: 0.01, tau: 0.1, filter: { type: "bandpass", q: 4, freq: t => 2400 * p * Math.pow(0.3, t / 0.22) } });   // a water jet
    bubbles(s, { at: 0.08, count: 3, spread: 0.15, low: 700, high: 1400, amp: 0.15 });
    reverb(s, 0.12);
    return s;
});

define("base_hit", { volume: 0.55, pitch: 0.05, cooldown: 0.1, maxInstances: 2 }, 3, p => {
    const s = new Sound(1.4);
    bell(s, { freq: 180 * p, ratio: 2.76, index: 4, tau: 0.35, amp: 0.4 });                               // the hull clangs
    bell(s, { freq: 520 * p, ratio: 1.47, index: 2, tau: 0.2, amp: 0.25 });
    tone(s, { dur: 0.15, freq: t => 90 * Math.pow(0.55, t / 0.15), amp: 0.5, tau: 0.06 });
    noise(s, { dur: 0.06, amp: 0.3, tau: 0.02, filter: { type: "lowpass", freq: 2500 } });
    reverb(s, 0.2);
    return s;
});
define("knockback", { volume: 0.5, pitch: 0.06, cooldown: 0.05, maxInstances: 4 }, 1, p => {
    const s = new Sound(0.7);
    tone(s, { dur: 0.35, freq: t => (330 + 260 * Math.sin(2 * Math.PI * 11 * t) * Math.exp(-t / 0.12)) * p, amp: 0.35, tau: 0.15 });   // boing
    noise(s, { dur: 0.3, amp: 0.3, attack: 0.02, tau: 0.12, filter: { type: "bandpass", q: 1.2, freq: t => 900 * Math.pow(0.4, t / 0.3) } });  // whoosh
    reverb(s, 0.12);
    return s;
});
define("death_unit", { volume: 0.55, pitch: 0.06, cooldown: 0.03, maxInstances: 4 }, 1, p => {
    const s = new Sound(1);
    tone(s, { dur: 0.12, freq: t => 700 * p * Math.pow(0.5, t / 0.12), amp: 0.35, tau: 0.06 });          // bloop...
    tone(s, { at: 0.14, dur: 0.16, freq: t => 480 * p * Math.pow(0.5, t / 0.16), amp: 0.35, tau: 0.08 }); // ...bloop (fainting)
    noise(s, { dur: 0.2, amp: 0.15, attack: 0.02, tau: 0.08, filter: { type: "lowpass", freq: 700 } });
    bubbles(s, { at: 0.25, count: 5, spread: 0.4, low: 600, high: 1500, amp: 0.15 });
    reverb(s, 0.18);
    return s;
});
define("death_enemy", { volume: 0.5, pitch: 0.06, cooldown: 0.03, maxInstances: 4 }, 1, p => {
    const s = new Sound(0.9);
    noise(s, { dur: 0.03, amp: 0.55, tau: 0.008, filter: { type: "highpass", freq: 1000 } });             // pop!
    tone(s, { dur: 0.08, freq: t => 900 * p * Math.pow(0.35, t / 0.08), amp: 0.4, tau: 0.03 });
    note(s, { at: 0.05, program: 9, key: 96, velocity: 80, len: 0.15, amp: 0.4 });                       // a shell sparkle
    bubbles(s, { at: 0.04, count: 6, spread: 0.3, low: 700, high: 1800, amp: 0.18 });
    reverb(s, 0.15);
    return s;
});
define("boss_shockwave", { volume: 1, pitch: 0, cooldown: 1, maxInstances: 1 }, 1, () => {
    const s = new Sound(3.5);
    tone(s, { dur: 1.8, freq: t => 70 * Math.pow(0.4, t / 1.8), amp: 0.9, tau: 0.55 });                 // a deep boom
    noise(s, { dur: 1.5, amp: 0.7, tau: 0.35, filter: { type: "lowpass", freq: t => 2000 * Math.pow(0.08, t / 1.5) } });
    note(s, { program: 47, key: 33, velocity: 127, len: 1.2, amp: 0.6 });
    noise(s, { at: 0.15, dur: 0.9, amp: 0.35, attack: 0.3, release: 0.4, filter: { type: "bandpass", q: 1, freq: t => 300 + 1300 * t } });   // the surge
    bubbles(s, { at: 0.1, count: 28, spread: 1.4, low: 250, high: 1800, size: [0.04, 0.12], amp: 0.25 });
    softClip(s, 1.4);
    reverb(s, 0.25, 1);
    return s;
});
define("base_destroyed", { volume: 1, pitch: 0, cooldown: 1, maxInstances: 1 }, 1, () => {
    const s = new Sound(4);
    noise(s, { dur: 2.2, amp: 0.85, tau: 0.5, filter: { type: "lowpass", freq: t => 4500 * Math.pow(0.05, t / 2.2) } });   // the blast
    tone(s, { dur: 2, freq: t => 60 * Math.pow(0.4, t / 2), amp: 0.8, tau: 0.6 });
    for (let i = 0; i < 10; i++) bell(s, { at: 0.15 + random() * 1.4, freq: 600 + random() * 2400, ratio: 1.3 + random() * 2, index: 2, tau: 0.08 + random() * 0.1, amp: 0.15, pan: random() * 1.6 - 0.8 });   // debris
    zap(s, { at: 0.05, dur: 0.6, amp: 0.25 });
    bubbles(s, { at: 0.2, count: 36, spread: 2, low: 200, high: 2000, size: [0.04, 0.14], amp: 0.25 });
    softClip(s, 1.5);
    reverb(s, 0.3, 1);
    return s;
});

// Statuses (when a hit gives one)
define("status_paralyze", { volume: 0.5, pitch: 0.05, cooldown: 0.08, maxInstances: 2 }, 1, () => {
    const s = new Sound(0.8);
    zap(s, { dur: 0.35, amp: 0.8 });
    zap(s, { at: 0.12, dur: 0.2, amp: 0.4, pan: 0.4 });
    reverb(s, 0.12);
    return s;
});
define("status_slow", { volume: 0.5, pitch: 0.05, cooldown: 0.08, maxInstances: 2 }, 1, () => {
    const s = new Sound(0.9);
    tone(s, { dur: 0.4, freq: t => 380 * Math.pow(0.4, t / 0.4), amp: 0.45, tau: 0.2, vibrato: 0.08, vibratoHz: 9, filter: { type: "lowpass", freq: 900 } });   // blorp
    noise(s, { dur: 0.35, amp: 0.2, tau: 0.15, filter: { type: "lowpass", freq: 450 } });
    bubble(s, { at: 0.1, freq: 300, size: 0.1, amp: 0.25, rise: 0.8 });
    reverb(s, 0.15);
    return s;
});
define("status_weaken", { volume: 0.5, pitch: 0.05, cooldown: 0.08, maxInstances: 2 }, 1, () => {
    const s = new Sound(0.9);
    const wah = { type: "lowpass", q: 2, freq: t => 2200 * Math.pow(0.2, t / 0.35) };
    tone(s, { dur: 0.35, freq: t => 520 * Math.pow(0.6, t / 0.35), wave: "triangle", amp: 0.45, tau: 0.2, filter: wah });   // a sad "wah"
    tone(s, { dur: 0.35, freq: t => 437 * Math.pow(0.6, t / 0.35), wave: "triangle", amp: 0.3, tau: 0.2, filter: wah });
    reverb(s, 0.15);
    return s;
});

// ===== Write =====
fs.mkdirSync(OUT, { recursive: true });
for (const old of fs.readdirSync(OUT)) if (old.endsWith(".ogg")) fs.rmSync(path.join(OUT, old));
const manifest = {};
SOUNDS.forEach(({ name, settings, variants, build }, index) => {
    const files = [];
    for (let v = 0; v < variants; v++) {
        seed(1000 * (index + 1) + v);
        const samples = finish(build(PITCHES[v % PITCHES.length]));
        const file = name + (variants > 1 ? "_" + (v + 1) : "") + ".ogg";
        const temp = path.join(os.tmpdir(), "fish_fighters_sfx.f32");
        fs.writeFileSync(temp, Buffer.from(samples.buffer, samples.byteOffset, samples.byteLength));
        execFileSync("ffmpeg", ["-hide_banner", "-loglevel", "error", "-y", "-f", "f32le", "-ar", String(RATE), "-ac", "2", "-i", temp,
            "-c:a", "libvorbis", "-q:a", "5", path.join(OUT, file)]);
        fs.rmSync(temp);
        files.push("assets/audios/sfx/" + file);
    }
    manifest[name] = { files, ...settings };
    console.log(name.padEnd(16), files.length, "file(s)");
});
fs.writeFileSync(path.join(OUT, "sfx.json"), JSON.stringify(manifest, null, 4) + "\n");

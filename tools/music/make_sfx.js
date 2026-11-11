// Fish Fighters' sound effects, made as code: synthesized layers (bubbles, splashes, zaps, metal clangs, booms)
// and a few SoundFont notes (GeneralUser GS) for the musical ones, all with an underwater touch.
// Writes assets/audios/sfx/<name>[_<variant>].ogg and assets/audios/sfx/sfx.json, the game's list of sounds:
//   volume (0-1), pitch (random +- variation), cooldown (seconds before it can play again), maxInstances.
//
// Usage: node tools/music/make_sfx.js [sound...]     (needs ffmpeg on the PATH and the SoundFont, like render_stems.js)
//        Without sounds, every sound is made again; with some, only those (the others keep their files)

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

// A boom that reads on small speakers: a falling sine plus a triangle an octave up, saturated (harmonics 120-400 Hz)
function boom(s, { at = 0, from, to, dur, amp, drive = 2.2 }) {
    const b = new Sound(dur * 2 + 0.1);
    const f = t => from * Math.pow(to / from, Math.min(1, t / dur));
    tone(b, { dur: dur * 1.6, freq: f, amp, attack: 0.002, tau: dur * 0.45 });
    tone(b, { dur: dur * 1.2, freq: t => 2 * f(t), wave: "triangle", amp: amp * 0.4, attack: 0.002, tau: dur * 0.3 });
    softClip(b, drive);
    const first = Math.round(at * RATE);
    for (let i = 0; i < b.data.length / 2; i++) s.add(first + i, b.data[i * 2], b.data[i * 2 + 1]);
}
// A cartoon "mouth" sound: a saw through two vowel formants (no recording)
const VOWELS = { uh: [640, 1190], oh: [450, 800], ah: [730, 1090], ee: [300, 2300], oo: [325, 700] };
function mouth(s, { at = 0, dur, freq, vowel, amp, pan = 0 }) {
    const [f1, f2] = VOWELS[vowel];
    const common = { at, dur, freq, wave: "saw", attack: 0.015, release: 0.04, vibrato: 0.012, vibratoHz: 5.5, pan };
    tone(s, { ...common, amp, filter: { type: "bandpass", freq: f1, q: 4 } });
    tone(s, { ...common, amp: amp * 0.6, filter: { type: "bandpass", freq: f2, q: 5 } });
}

// ===== The sounds =====
// Each: [name, settings for sfx.json, variants (functions building a Sound, given a pitch factor and the variant index)]
const SOUNDS = [];
const define = (name, settings, variants, build) => SOUNDS.push({ name, settings, variants, build });
const PITCHES = [1, 0.92, 1.09];

// ----- Menus -----
// The sounds of what gets clicked all the time are short, soft and low: a muted tap, not a melody
define("ui_click", { volume: 0.32, pitch: 0.03, cooldown: 0.04, maxInstances: 1, priority: 90 }, 1, p => {
    const s = new Sound(0.25);
    tone(s, { dur: 0.04, freq: t => 560 * p * Math.pow(0.8, t / 0.04), wave: "triangle", amp: 0.45, tau: 0.012 });   // a muted tap
    noise(s, { dur: 0.008, amp: 0.12, tau: 0.002, filter: { type: "bandpass", freq: 2200, q: 1.5 } });
    noise(s, { dur: 0.005, amp: 0.12, attack: 0.0003, tau: 0.0015, filter: { type: "bandpass", freq: 3200, q: 1.5 } }); // a faint tick: reads on any speaker
    filterAll(s, "lowpass", 6500);
    reverb(s, 0.06);
    return s;
});
define("ui_back", { volume: 0.28, pitch: 0.03, cooldown: 0.04, maxInstances: 1 }, 1, p => {
    const s = new Sound(0.25);
    tone(s, { dur: 0.05, freq: t => 420 * p * Math.pow(0.75, t / 0.05), wave: "triangle", amp: 0.45, tau: 0.015 });  // the tap, lower
    noise(s, { dur: 0.008, amp: 0.1, tau: 0.002, filter: { type: "bandpass", freq: 1800, q: 1.5 } });
    filterAll(s, "lowpass", 2500);
    reverb(s, 0.06);
    return s;
});
define("ui_select", { volume: 0.22, pitch: 0.04, cooldown: 0.06, maxInstances: 1 }, 1, p => {
    const s = new Sound(0.3);
    tone(s, { dur: 0.06, freq: 1320 * p, amp: 0.35, tau: 0.018 });                                       // a little glass tick
    tone(s, { dur: 0.04, freq: 1980 * p, amp: 0.12, tau: 0.01 });
    reverb(s, 0.08);
    return s;
});
define("ui_transition", { volume: 0.18, pitch: 0.03, cooldown: 0.2, maxInstances: 1 }, 1, p => {
    const s = new Sound(0.6);
    noise(s, { dur: 0.28, amp: 0.45, attack: 0.08, release: 0.15, filter: { type: "bandpass", q: 1,
        freq: t => 350 + 900 * Math.sin(Math.PI * Math.min(1, t / 0.28)) } });                           // a soft swish of water
    bubbles(s, { at: 0.06, count: 3, spread: 0.15, low: 600, high: 1100, amp: 0.12 });
    reverb(s, 0.12);
    return s;
});
// "Nuh-uh": two short blips stepping down, in the mid band (reads over the battle music and on laptops)
define("ui_error", { volume: 0.32, pitch: 0.02, cooldown: 0.18, maxInstances: 1, priority: 90 }, 1, () => {
    const s = new Sound(0.2);
    const blip = (at, f, extra = {}) => tone(s, { at, dur: 0.045, freq: t => f * Math.pow(0.94, t / 0.045), amp: 0.4,
        attack: 0.002, tau: 0.018, release: 0.008, ...extra });
    blip(0, 330, { wave: "square", filter: { type: "lowpass", freq: 2000, q: 0.9 } });
    blip(0.105, 294, { wave: "square", filter: { type: "lowpass", freq: 2000, q: 0.9 } });
    blip(0.105, 294, { wave: "saw", amp: 0.15, filter: { type: "bandpass", freq: 900, q: 2 } });
    reverb(s, 0.02, 0.5);
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
define("purchase", { volume: 0.65, pitch: 0, cooldown: 0.1, maxInstances: 2 }, 1, () => {
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
define("equip", { volume: 0.5, pitch: 0.04, cooldown: 0.05, maxInstances: 2 }, 1, p => {
    const s = new Sound(0.7);
    tone(s, { dur: 0.12, freq: t => 140 * p * Math.pow(0.45, t / 0.12), amp: 0.6, tau: 0.05 });       // thump
    bell(s, { freq: 420 * p, ratio: 2.41, index: 4, tau: 0.08, amp: 0.3 });                             // metal latch
    bell(s, { at: 0.005, freq: 1150 * p, ratio: 1.73, index: 2, tau: 0.05, amp: 0.2 });
    bubble(s, { at: 0.05, freq: 600, size: 0.05, amp: 0.15 });
    reverb(s, 0.15);
    return s;
});
define("unequip", { volume: 0.4, pitch: 0.04, cooldown: 0.05, maxInstances: 2 }, 1, p => {
    const s = new Sound(0.6);
    bell(s, { freq: 900 * p, ratio: 1.73, index: 2, tau: 0.05, amp: 0.25 });
    bubble(s, { at: 0.02, freq: 500 * p, size: 0.07, amp: 0.3, rise: 1.6 });                            // pops out
    noise(s, { dur: 0.15, amp: 0.15, attack: 0.03, filter: { type: "bandpass", freq: 1200, q: 1 } });
    reverb(s, 0.15);
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
define("cash_drop", { volume: 0.3, pitch: 0.06, cooldown: 0.06, maxInstances: 2 }, 1, p => {
    const s = new Sound(0.6);
    bell(s, { freq: 2600 * p, ratio: 1.41, index: 1.5, tau: 0.07, amp: 0.4 });                          // a small shell clinks
    bell(s, { at: 0.05, freq: 3300 * p, ratio: 1.41, index: 1.2, tau: 0.06, amp: 0.25 });
    reverb(s, 0.12);
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
// The hit of every fish and every enemy (one sound for all, like a lane battler's): a smack, a mid-band body,
// a short wet splat and a small thump, 250 ms at most. 4 real variants (body, smack and splat move), not only pitch
const SLAP = { b0: [520, 470, 580, 545], b1: [300, 280, 330, 290], smack: [2200, 1800, 2600, 3000], s0: [1200, 1000, 1400, 1100], s1: [500, 450, 550, 480], p: [1, 0.97, 1.03, 1] };
define("attack_slap", { volume: 0.5, pitch: 0.04, cooldown: 0.035, maxInstances: 6, priority: 40 }, 4, (_, v) => {
    const s = new Sound(0.25), p = SLAP.p[v];
    noise(s, { dur: 0.012, amp: 0.6, attack: 0.0005, tau: 0.004, release: 0.002, filter: { type: "bandpass", freq: SLAP.smack[v], q: 0.9 } });
    noise(s, { dur: 0.005, amp: 0.22, attack: 0.0003, tau: 0.0015, release: 0.001, filter: { type: "highpass", freq: 4500 } });
    tone(s, { dur: 0.09, freq: t => p * SLAP.b0[v] * Math.pow(SLAP.b1[v] / SLAP.b0[v], Math.min(1, t / 0.07)), wave: "triangle",
        amp: 0.55, attack: 0.001, tau: 0.028, release: 0.01, filter: { type: "lowpass", freq: 2500 } });
    noise(s, { at: 0.004, dur: 0.1, amp: 0.25, attack: 0.003, tau: 0.03, filter: { type: "bandpass", q: 1.4,
        freq: t => SLAP.s0[v] * Math.pow(SLAP.s1[v] / SLAP.s0[v], Math.min(1, t / 0.08)) } });
    tone(s, { dur: 0.07, freq: t => 150 * p * Math.pow(0.63, t / 0.06), amp: 0.22, attack: 0.001, tau: 0.025 });
    softClip(s, 1.6);
    reverb(s, 0.03, 0.6);
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

// The fish bowl hit (by an enemy): thick glass, water sloshing inside - a danger cue
define("base_hit_bowl", { volume: 0.6, pitch: 0.03, cooldown: 0.12, maxInstances: 2, priority: 80 }, 3, (p, v) => {
    const s = new Sound(0.6), g = [1, 1.05, 0.96][v];
    tone(s, { dur: 0.09, freq: t => 320 * p * Math.pow(0.66, t / 0.06), amp: 0.45, attack: 0.001, tau: 0.03 });
    bell(s, { freq: 1180 * g, ratio: 2.32, index: 1.6, tau: 0.12, amp: 0.32 });
    bell(s, { at: 0.004, freq: 1870 * g, ratio: 1.41, index: 1.0, tau: 0.08, amp: 0.16 });
    noise(s, { at: 0.03, dur: 0.3, amp: 0.2, attack: 0.04, tau: 0.12, width: 0.6,
        filter: { type: "bandpass", q: 1.2, freq: t => 900 * Math.pow(0.39, Math.min(1, t / 0.25)) } });
    bubble(s, { at: 0.12, freq: 420, size: 0.06, amp: 0.14 });
    bubble(s, { at: 0.17, freq: 560, size: 0.05, amp: 0.1 });
    reverb(s, 0.08, 0.7);
    return s;
});
// The rock cave hit (by a fish): a stone thock, a crack, grit and a trickle of pebbles - a progress cue
define("base_hit_cave", { volume: 0.5, pitch: 0.05, cooldown: 0.1, maxInstances: 2, priority: 50 }, 3, (p) => {
    const s = new Sound(0.45);
    tone(s, { dur: 0.08, freq: t => 210 * p * Math.pow(0.67, t / 0.05), wave: "triangle", amp: 0.5, attack: 0.001, tau: 0.022,
        filter: { type: "lowpass", freq: 1800 } });
    noise(s, { dur: 0.025, amp: 0.5, attack: 0.0005, tau: 0.008, filter: { type: "bandpass", freq: 1100 * p, q: 1.2 } });
    [0, 0.018, 0.041].forEach(at => noise(s, { at, dur: 0.006, amp: 0.25, attack: 0.0003, tau: 0.002, filter: { type: "highpass", freq: 2500 } }));
    for (let i = 0; i < 5; i++) bell(s, { at: 0.06 + random() * 0.2, freq: 2200 + random() * 2000, ratio: 1.3 + random() * 0.8,
        index: 1, tau: 0.015, amp: 0.08, pan: (random() - 0.5) * 0.8 });
    reverb(s, 0.06, 0.6);
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
// A boss arrives: a boom you can hear on a laptop (50-90 Hz with harmonics, not a 30 Hz rumble), a crack,
// the displaced water and the surge, bubbles
define("boss_shockwave", { volume: 1, pitch: 0, cooldown: 1, maxInstances: 1, priority: 100 }, 1, () => {
    const s = new Sound(2.6);
    boom(s, { from: 92, to: 50, dur: 0.9, amp: 0.8 });
    noise(s, { dur: 0.035, amp: 0.7, attack: 0.0005, tau: 0.009, filter: { type: "bandpass", freq: 1800, q: 0.7 } });
    noise(s, { dur: 0.015, amp: 0.3, attack: 0.0003, tau: 0.004, filter: { type: "highpass", freq: 3500 } });
    noise(s, { dur: 1.0, amp: 0.5, attack: 0.01, tau: 0.35, filter: { type: "lowpass", freq: t => 1500 * Math.pow(0.13, Math.min(1, t / 0.9)) } });
    noise(s, { at: 0.12, dur: 0.9, amp: 0.3, attack: 0.3, release: 0.4, filter: { type: "bandpass", q: 1, freq: t => 300 + 1300 * t } });
    note(s, { program: 47, key: 38, velocity: 127, len: 1.0, amp: 0.45 });
    bubbles(s, { at: 0.1, count: 28, spread: 1.4, low: 250, high: 1800, size: [0.04, 0.12], amp: 0.22 });
    filterAll(s, "highpass", 35);
    reverb(s, 0.2, 1);
    return s;
});
// Victory: the enemy's rock cave crumbles (rock chunks, then one last pebble: "plink")
define("base_destroyed_cave", { volume: 1, pitch: 0, cooldown: 1, maxInstances: 1, priority: 100 }, 1, () => {
    const s = new Sound(2.6);
    boom(s, { from: 85, to: 52, dur: 0.8, amp: 0.75 });
    noise(s, { dur: 0.04, amp: 0.8, attack: 0.0005, tau: 0.01, filter: { type: "bandpass", freq: 1600, q: 0.8 } });
    noise(s, { dur: 0.02, amp: 0.4, attack: 0.0003, tau: 0.005, filter: { type: "highpass", freq: 3000 } });
    noise(s, { dur: 1.8, amp: 0.5, attack: 0.02, tau: 0.5, filter: { type: "lowpass", freq: t => 2500 * Math.pow(0.16, Math.min(1, t / 1.6)) } });
    for (let i = 0; i < 14; i++) {
        const at = 0.1 + Math.pow(random(), 1.6) * 1.3, pan = (random() - 0.5) * 1.2;
        tone(s, { at, dur: 0.06, freq: 120 + random() * 140, amp: 0.2 + random() * 0.15, attack: 0.001, tau: 0.018, pan });
        noise(s, { at, dur: 0.008, amp: 0.15, attack: 0.0003, tau: 0.003, filter: { type: "bandpass", freq: 1500, q: 1 }, pan });
    }
    bell(s, { at: 1.65, freq: 3200, ratio: 1.6, index: 1, tau: 0.04, amp: 0.15, pan: 0.3 });
    bubbles(s, { at: 0.2, count: 20, spread: 1.2, low: 250, high: 1600, size: [0.04, 0.12], amp: 0.2 });
    filterAll(s, "highpass", 35);
    reverb(s, 0.2, 1);
    return s;
});
// Defeat: the fish bowl shatters, the water gushes out, one sad "blub"
define("base_destroyed_bowl", { volume: 0.95, pitch: 0, cooldown: 1, maxInstances: 1, priority: 100 }, 1, () => {
    const s = new Sound(2.6);
    boom(s, { from: 140, to: 80, dur: 0.3, amp: 0.5, drive: 1.6 });
    noise(s, { dur: 0.06, amp: 0.7, attack: 0.0005, tau: 0.025, filter: { type: "highpass", freq: 2200 } });
    for (let i = 0; i < 24; i++) bell(s, { at: 0.02 + Math.pow(random(), 2) * 0.9, freq: 2500 + random() * 4000, ratio: 1.8 + random() * 1.1,
        index: 1.2, tau: 0.03 + random() * 0.06, amp: 0.06 + random() * 0.08, pan: (random() - 0.5) * 1.4 });
    noise(s, { at: 0.05, dur: 1.3, amp: 0.45, attack: 0.06, tau: 0.6, width: 0.7,
        filter: { type: "bandpass", q: 0.9, freq: t => 1800 * Math.pow(0.28, Math.min(1, t / 1.2)) } });
    noise(s, { at: 0.05, dur: 1.0, amp: 0.2, attack: 0.08, tau: 0.5, filter: { type: "lowpass", freq: 600 } });
    bubble(s, { at: 1.35, freq: 300, size: 0.15, amp: 0.4, rise: 0.6 });
    filterAll(s, "highpass", 40);
    reverb(s, 0.2, 1);
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

// When the voices run out, a sound takes the place of a lower one (AudioManager)
const PRIORITIES = {
    ui_click: 90, ui_back: 90, ui_select: 85, ui_transition: 70, ui_error: 90, ui_pause: 90, ui_unpause: 90, ui_start: 90,
    purchase: 85, unlock: 90, equip: 85, unequip: 85, deploy: 65, cash_drop: 30, cash_up: 70,
    attack_slap: 40, attack_bite: 40, attack_pinch: 40, attack_zap: 40, attack_heavy: 40, attack_squirt: 40,
    knockback: 45, death_unit: 60, death_enemy: 55, status_paralyze: 50, status_slow: 50, status_weaken: 50,
};

// ----- Boss, alarms, readiness, results -----
// The boss music change: a brass cluster (the b9 is the menace), trombones falling, timpani, a china,
// a boom and a gong shimmer, and a cartoon "uh-oh". Played at the new theme's key (AudioManager pitch)
define("boss_stinger", { volume: 0.85, pitch: 0, cooldown: 3, maxInstances: 1, priority: 95 }, 1, () => {
    const s = new Sound(2.4);
    [38, 45, 50, 51].forEach(key => note(s, { program: 61, key, velocity: 122, len: 0.7, amp: 0.45 }));
    [38, 45].forEach(key => note(s, { program: 57, key, velocity: 118, len: 0.9, amp: 0.4, bend: -1 }));
    note(s, { program: 47, key: 38, velocity: 127, len: 1.0, amp: 0.6 });
    note(s, { drums: true, program: 0, key: 52, velocity: 118, len: 1.5, amp: 0.4 });
    boom(s, { from: 90, to: 55, dur: 0.8, amp: 0.5 });
    bell(s, { freq: 146, ratio: 1.41, index: 5, tau: 0.8, amp: 0.18 });
    noise(s, { dur: 1.4, amp: 0.14, attack: 0.12, tau: 0.5, release: 0.3, filter: { type: "bandpass", freq: 900, q: 0.8 } });
    mouth(s, { at: 0.55, dur: 0.14, freq: 330, vowel: "uh", amp: 0.3 });
    mouth(s, { at: 0.72, dur: 0.28, freq: t => 247 * Math.pow(0.95, t / 0.28), vowel: "oh", amp: 0.3 });
    softClip(s, 1.3);
    filterAll(s, "highpass", 35);
    reverb(s, 0.18, 1);
    return s;
});
// The fish bowl is in danger (below 30%): "bwee-bwoo" with a sonar edge, mid band so it cuts through
define("base_alarm", { volume: 0.5, pitch: 0, cooldown: 4, maxInstances: 1, priority: 85 }, 1, () => {
    const s = new Sound(0.8);
    const sq = { wave: "square", amp: 0.3, attack: 0.005, vibrato: 0.006, vibratoHz: 7 };
    tone(s, { ...sq, dur: 0.16, freq: 660, release: 0.03, filter: { type: "lowpass", freq: 2200 } });
    tone(s, { ...sq, at: 0.2, dur: 0.22, freq: 523, release: 0.05, filter: { type: "lowpass", freq: 2000 } });
    tone(s, { dur: 0.4, freq: 1450, amp: 0.12, attack: 0.002, tau: 0.25 });
    bubble(s, { at: 0.45, freq: 700, size: 0.04, amp: 0.1 });
    reverb(s, 0.12, 0.7);
    return s;
});
// A slow fish is ready again: a soft rising "bip" and a tiny glass tick (felt more than heard)
// A fish of the hub's tank was clicked: a round "blub" and a few bubbles
define("fish_blub", { volume: 0.4, pitch: 0.06, cooldown: 0.08, maxInstances: 2, priority: 30 }, 3, p => {
    const s = new Sound(0.5);
    bubble(s, { freq: 380 * p, size: 0.08, amp: 0.45, rise: 1.7 });
    bubble(s, { at: 0.08, freq: 560 * p, size: 0.06, amp: 0.32, rise: 1.6 });
    bubbles(s, { at: 0.13, count: 4, spread: 0.2, low: 900 * p, high: 1800 * p, amp: 0.1 });
    reverb(s, 0.1, 0.5);
    return s;
});
define("unit_ready", { volume: 0.22, pitch: 0.03, cooldown: 0.25, maxInstances: 1, priority: 35 }, 1, () => {
    const s = new Sound(0.25);
    bubble(s, { freq: 900, size: 0.035, amp: 0.3, rise: 1.0 });
    tone(s, { at: 0.03, dur: 0.05, freq: 1760, amp: 0.15, attack: 0.001, tau: 0.02 });
    reverb(s, 0.05, 0.5);
    return s;
});
// The wallet is full: three little clinks and a gulp
define("wallet_full", { volume: 0.45, pitch: 0, cooldown: 2, maxInstances: 1, priority: 45 }, 1, () => {
    const s = new Sound(0.7);
    [2600, 3300, 3900].forEach((freq, i) => bell(s, { at: i * 0.06, freq, ratio: 1.41, index: 1.4, tau: 0.08, amp: 0.25, pan: -0.2 + i * 0.2 }));
    bubble(s, { at: 0.22, freq: 260, size: 0.1, amp: 0.35, rise: 0.9 });
    reverb(s, 0.1, 0.6);
    return s;
});
// The results screen: a tick per step of the shells counting up (pitched up as it goes), and the landing
define("reward_tick", { volume: 0.2, pitch: 0, cooldown: 0.045, maxInstances: 2, priority: 70 }, 1, () => {
    const s = new Sound(0.1);
    bell(s, { freq: 3200, ratio: 1.41, index: 0.8, tau: 0.025, amp: 0.35 });
    noise(s, { dur: 0.003, amp: 0.1, attack: 0.0003, tau: 0.001, filter: { type: "highpass", freq: 5000 } });
    return s;
});
define("shell_collect", { volume: 0.6, pitch: 0, cooldown: 0.5, maxInstances: 1, priority: 75 }, 1, () => {
    const s = new Sound(1.0);
    [2200, 2800, 3500].forEach((freq, i) => bell(s, { at: i * 0.05, freq, ratio: 1.41, index: 2, tau: 0.12, amp: 0.3, pan: -0.3 + i * 0.3 }));
    [89, 93, 96].forEach((key, i) => note(s, { at: 0.12 + i * 0.05, program: 9, key, velocity: 92, len: 0.25, amp: 0.5 }));
    bubbles(s, { at: 0.1, count: 6, spread: 0.3, low: 800, high: 2000, amp: 0.14 });
    reverb(s, 0.15, 0.7);
    return s;
});

// ===== Write =====
const only = process.argv.slice(2);
for (const name of only) if (!SOUNDS.some(sound => sound.name === name)) throw new Error(name + ": no such sound");
const previousFile = path.join(OUT, "sfx.json");
const previous = fs.existsSync(previousFile) ? JSON.parse(fs.readFileSync(previousFile, "utf8")) : {};
const isMade = name => !only.length || only.includes(name);

fs.mkdirSync(OUT, { recursive: true });
for (const old of fs.readdirSync(OUT)) {
    const owner = old.replace(/(_\d+)?\.ogg$/, "");
    if (old.endsWith(".ogg") && (!only.length || (isMade(owner) && SOUNDS.some(sound => sound.name === owner)))) fs.rmSync(path.join(OUT, old));
}
const manifest = {};
SOUNDS.forEach(({ name, settings, variants, build }, index) => {
    if (!isMade(name)) {
        if (previous[name]) manifest[name] = previous[name];   // Not made this time: its files stay
        else console.warn(name + ": never made, run without arguments");
        return;
    }
    const files = [];
    for (let v = 0; v < variants; v++) {
        seed([...name].reduce((h, c) => Math.imul(h ^ c.charCodeAt(0), 16777619) >>> 0, 2166136261) + v);
        const samples = finish(build(PITCHES[v % PITCHES.length], v));
        const file = name + (variants > 1 ? "_" + (v + 1) : "") + ".ogg";
        const temp = path.join(os.tmpdir(), "fish_fighters_sfx.f32");
        fs.writeFileSync(temp, Buffer.from(samples.buffer, samples.byteOffset, samples.byteLength));
        execFileSync("ffmpeg", ["-hide_banner", "-loglevel", "error", "-y", "-f", "f32le", "-ar", String(RATE), "-ac", "2", "-i", temp,
            "-c:a", "libvorbis", "-q:a", "5", path.join(OUT, file)]);
        fs.rmSync(temp);
        files.push("assets/audios/sfx/" + file);
    }
    manifest[name] = { files, ...settings, priority: settings.priority ?? PRIORITIES[name] ?? 50 };
    console.log(name.padEnd(16), files.length, "file(s)");
});
fs.writeFileSync(path.join(OUT, "sfx.json"), JSON.stringify(manifest, null, 4) + "\n");

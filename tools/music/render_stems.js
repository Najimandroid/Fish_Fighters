// Renders the stems written by compose_music.js to 24-bit WAVs for the Waveform mix: every instrument with
// sf2_render.js and the GeneralUser GS SoundFont (free for any music, github.com/mrbumpy409/GeneralUser-GS),
// then its own effects (pan, chorus, echo, low-pass: the "underwater" colour), then summed into its group
// (lead, brass, keys, bass, drums...: at most 8 tracks per song in Waveform). Bubbles and sonar pings are synthesized here.
// The reverb and the limiter are on Waveform's master.
//
// The groups are balanced by loudness against the song's mix.json targets (dB relative to the lead group),
// then share one gain (the full mix peaks at -3 dB), so the mix holds with all the Waveform faders at 0 dB.
//
// Usage: node tools/music/render_stems.js [song]
// Needs audio_sources/tools/soundfont/GeneralUser-GS.sf2 (not in git).

const fs = require("fs");
const path = require("path");
const { SoundFont, render, writeWav } = require("./sf2_render");
const { loudness } = require("./loudness");

const ROOT = path.join(__dirname, "..", "..");
const STEMS_MIDI = path.join(ROOT, "audio_sources", "midi", "stems");
const STEMS_WAV = path.join(ROOT, "audio_sources", "stems");
const SOUNDFONT = path.join(ROOT, "audio_sources", "tools", "soundfont", "GeneralUser-GS.sf2");
const RATE = 48000;
const FX_TAIL = 4; // seconds of silence added before the effects, for echo tails

// ===== Synthesized sounds =====

// A bubble is a sine whose pitch rises as it shrinks (the Minnaert resonance), with a very short envelope.
// events: [[seconds, frequency, velocity, size (seconds), pan (-1..1)]]
function renderBubbles(events) {
    const end = Math.max(0, ...events.map(e => e[0] + e[3] * 4)) + 1;
    const out = new Float32Array(Math.ceil(end * RATE) * 2);
    for (const [start, frequency, velocity, size, pan = 0] of events) {
        const left = Math.cos((pan + 1) * Math.PI / 4), right = Math.sin((pan + 1) * Math.PI / 4);
        const amplitude = Math.pow(velocity / 127, 1.5) * 0.45;
        const frames = Math.round(size * 4 * RATE), first = Math.round(start * RATE);
        let phase = 0;
        for (let f = 0; f < frames; f++) {
            const t = f / RATE;
            phase += 2 * Math.PI * frequency * Math.pow(2, 1.3 * Math.min(t / size, 1.6)) / RATE;
            const s = Math.sin(phase) * amplitude * (1 - Math.exp(-t / 0.0015)) * Math.exp(-t / (size * 0.45));
            out[(first + f) * 2] += s * left;
            out[(first + f) * 2 + 1] += s * right;
        }
    }
    return out;
}

// Lab beeps and boops (the encyclopedia's theme): short computer tones, a soft square (odd harmonics) or a pure sine,
// sliding while they sound (a "boop" falls, a "bweep" rises), a quick attack and a short decay
// events: [[seconds, frequency, velocity, length (seconds), slide (semitones), square (0 or 1)]]
function renderBeeps(events) {
    const end = Math.max(0, ...events.map(e => e[0] + e[3])) + 0.5;
    const out = new Float32Array(Math.ceil(end * RATE) * 2);
    for (const [start, frequency, velocity, length, slide = 0, square = 0] of events) {
        const amplitude = Math.pow(velocity / 127, 1.5) * (square ? 0.13 : 0.22), first = Math.round(start * RATE);
        const frames = Math.round((length + 0.04) * RATE);
        let phase = 0;
        for (let i = 0; i < frames; i++) {
            const t = i / RATE;
            phase += 2 * Math.PI * frequency * Math.pow(2, slide * Math.min(1, t / length) / 12) / RATE;
            const wave = square ? Math.sin(phase) + Math.sin(3 * phase) / 3 + Math.sin(5 * phase) / 5 : Math.sin(phase);
            const envelope = Math.min(1, t / 0.003) * (t < length ? 1 : Math.max(0, 1 - (t - length) / 0.04)) * Math.exp(-t / (length * 2 + 0.03));
            const sample = amplitude * wave * envelope;
            out[(first + i) * 2] += sample;
            out[(first + i) * 2 + 1] += sample;
        }
    }
    return out;
}

// A sonar ping: a pure tone with a soft second partial and a long decay (the echo effect makes the returns)
// events: [[seconds, frequency, velocity]]
function renderPings(events) {
    const end = Math.max(0, ...events.map(e => e[0])) + 3.5;
    const out = new Float32Array(Math.ceil(end * RATE) * 2);
    for (const [start, frequency, velocity] of events) {
        const amplitude = Math.pow(velocity / 127, 1.5) * 0.3, first = Math.round(start * RATE);
        for (let f = 0; f < 3 * RATE; f++) {
            const t = f / RATE;
            const s = amplitude * (1 - Math.exp(-t / 0.002)) *
                (Math.sin(2 * Math.PI * frequency * t) * Math.exp(-t / 0.6) + 0.25 * Math.sin(2 * Math.PI * frequency * 2.01 * t) * Math.exp(-t / 0.15));
            out[(first + f) * 2] += s;
            out[(first + f) * 2 + 1] += s;
        }
    }
    return out;
}

// A theremin (the creature feature): a sine with a little 2nd and 3rd harmonic, a vibrato fading in (6 Hz, up to
// +-35 cents), a slight tremolo, gliding between legato notes; a phrase's first note can swoop up into place.
// events: [[seconds, frequency, length (beats), velocity, glide (seconds), swoop (semitones)]]
function renderTheremin(events, tempo) {
    const spb = 60 / tempo;
    const end = Math.max(0, ...events.map(e => e[0] + e[2] * spb)) + 1;
    const out = new Float32Array(Math.ceil(end * RATE) * 2);
    let phase = 0, previous = null;
    for (const [start, frequency, beats, velocity, glide = 0.07, swoop = 0] of events) {
        const length = beats * spb, first = Math.round(start * RATE);
        const frames = Math.round((length + 0.15) * RATE);
        // Glide from the previous note when it ends right before this one
        const from = previous && start - previous.end < 0.05 && glide > 0 ? previous.frequency
            : swoop > 0 ? frequency * Math.pow(2, -swoop / 12) : frequency;
        const glideTime = previous && start - previous.end < 0.05 && glide > 0 ? glide : swoop > 0 ? 0.2 : 0;
        const amplitude = Math.pow(velocity / 127, 1.5) * 0.35;
        let lowpassed = 0;
        const lowpassAlpha = 1 - Math.exp(-2 * Math.PI * 3500 / RATE);
        for (let i = 0; i < frames; i++) {
            const t = i / RATE;
            const f0 = glideTime > 0 && t < glideTime ? from * Math.pow(frequency / from, t / glideTime) : frequency;
            const vibrato = 35 * Math.min(1, t / 0.25) * Math.sin(2 * Math.PI * 6 * t);
            phase += 2 * Math.PI * f0 * Math.pow(2, vibrato / 1200) / RATE;
            const voice = Math.sin(phase) + 0.18 * Math.sin(2 * phase) + 0.06 * Math.sin(3 * phase);
            const envelope = Math.min(1, t / 0.04) * (t > length ? Math.max(0, 1 - (t - length) / 0.15) : 1);
            const tremolo = 1 - 0.05 * (0.5 + 0.5 * Math.sin(2 * Math.PI * 6 * t));
            lowpassed += lowpassAlpha * (voice * amplitude * envelope * tremolo - lowpassed);
            out[(first + i) * 2] += lowpassed;
            out[(first + i) * 2 + 1] += lowpassed;
        }
        previous = { end: start + length, frequency };
    }
    return out;
}

// A synth vibraphone (the encyclopedia's lab, doubling the Fish Tank's vibraphone so the two blend): a mallet's
// envelope (a quick strike, a long ring, the damper at the note's end) and a vibraphone's motor tremolo, made of sines:
// the bar's fundamental and its 4th partial (the two octaves up a vibraphone bar is tuned to), an FM shimmer on the
// strike that fades into an almost pure tone (a little of it kept, slowly breathing), two voices a few cents apart
// spread left and right (a slow chorus), and a pitch settling from a few cents above in the first moment (the
// synth's blip). Each note's sound depends only on its own time, so both body copies are identical.
// events: [[seconds, frequency, beats, velocity]]
function renderSynthVibes(events, tempo) {
    const spb = 60 / tempo, RING = 1.3, DAMP = 0.12, TREMOLO_HZ = 5.2, TREMOLO = 0.22;
    const end = Math.max(0, ...events.map(e => e[0] + e[2] * spb)) + 1;
    const out = new Float32Array(Math.ceil(end * RATE) * 2);
    for (const [start, frequency, beats, velocity] of events) {
        const length = beats * spb, first = Math.round(start * RATE), frames = Math.round((length + DAMP) * RATE);
        const amplitude = Math.pow(velocity / 127, 1.5) * 0.3, strike = 0.5 + 1.4 * velocity / 127;
        [-0.0025, 0.0025].forEach((detune, side) => {
            let phase = 0, partial = 0, modulator = 0;
            for (let i = 0; i < frames; i++) {
                const t = i / RATE;
                const f = frequency * (1 + detune) * (1 + 0.004 * Math.exp(-t / 0.012));
                phase += 2 * Math.PI * f / RATE;
                partial += 2 * Math.PI * f * 4 / RATE;
                modulator += 2 * Math.PI * f / RATE;
                const index = strike * Math.exp(-t / 0.08) + 0.12 + 0.08 * Math.sin(2 * Math.PI * 0.7 * t);
                const tone = Math.sin(phase + index * Math.sin(modulator)) + 0.3 * Math.exp(-t / 0.35) * Math.sin(partial);
                const envelope = Math.min(1, t / 0.003) * Math.exp(-t / RING) * (t > length ? Math.max(0, 1 - (t - length) / DAMP) : 1);
                const tremolo = 1 - TREMOLO * (0.5 - 0.5 * Math.cos(2 * Math.PI * TREMOLO_HZ * t));
                out[(first + i) * 2 + side] += tone * amplitude * envelope * tremolo;
            }
        });
    }
    return out;
}

// A small deterministic noise source (xorshift), so both body copies of a synthesized sound are identical
function noiseSource(seed) {
    let state = (seed >>> 0) || 1;
    return () => { state ^= state << 13; state ^= state >>> 17; state ^= state << 5; return (state >>> 0) / 2147483648 - 1; };
}

// A riser (the build before a drop or a big section): white noise through a resonant band-pass sweeping up from
// 350 Hz to 9 kHz, getting louder all the way, stopping dead on the downbeat. Each side its own noise (wide).
// events: [[seconds, beats, velocity, seed]]
function renderRisers(events, tempo) {
    const spb = 60 / tempo;
    const end = Math.max(0, ...events.map(e => e[0] + e[1] * spb)) + 1;
    const out = new Float32Array(Math.ceil(end * RATE) * 2);
    for (const [start, beats, velocity, seed] of events) {
        const length = beats * spb, first = Math.round(start * RATE), frames = Math.round(length * RATE);
        const amplitude = Math.pow(velocity / 127, 1.5) * 0.5;
        for (let ch = 0; ch < 2; ch++) {
            const noise = noiseSource(seed * 2 + ch + 1);
            let low = 0, band = 0;
            for (let i = 0; i < frames; i++) {
                const x = i / frames;   // 0 to 1 over the riser
                const cutoff = 350 * Math.pow(9000 / 350, x * x * (3 - 2 * x) * 0.6 + x * 0.4);
                const f = 2 * Math.sin(Math.PI * Math.min(cutoff, 12000) / RATE), q = 0.35;
                const input = noise();
                low += f * band;
                const high = input - low - q * band;
                band += f * high;
                const envelope = Math.pow(x, 1.6) * Math.min(1, (frames - i) / (0.004 * RATE));
                out[(first + i) * 2 + ch] += band * amplitude * envelope;
            }
        }
    }
    return out;
}

// An impact (the downbeat of a drop): a sub boom falling from 90 Hz to 38 Hz and a short dark noise crack on top
// events: [[seconds, velocity, seed]]
function renderImpacts(events) {
    const end = Math.max(0, ...events.map(e => e[0])) + 3;
    const out = new Float32Array(Math.ceil(end * RATE) * 2);
    for (const [start, velocity, seed = 1] of events) {
        const first = Math.round(start * RATE), frames = Math.round(2.2 * RATE);
        const amplitude = Math.pow(velocity / 127, 1.5) * 0.9;
        for (let ch = 0; ch < 2; ch++) {
            const noise = noiseSource(seed * 2 + ch + 7);
            let phase = 0, dark = 0;
            for (let i = 0; i < frames; i++) {
                const t = i / RATE;
                const f = 38 + 52 * Math.exp(-t / 0.12);
                phase += 2 * Math.PI * f / RATE;
                const boom = Math.sin(phase) * Math.exp(-t / 0.55) * Math.min(1, t / 0.002);
                dark += (1 - Math.exp(-2 * Math.PI * 1800 / RATE)) * (noise() - dark);
                const crack = dark * Math.exp(-t / 0.09) * 1.6;
                out[(first + i) * 2 + ch] += (boom + crack) * amplitude;
            }
        }
    }
    return out;
}

// A supersaw (the wide chord stabs of dance music): seven saws detuned up to 28 cents apart, spread across the
// stereo field, band-limited (polyBLEP), through a soft low-pass that opens with the velocity; a quick envelope.
// events: [[seconds, frequency, beats, velocity]]
function renderSupersaw(events, tempo) {
    const spb = 60 / tempo, DETUNE = [-28, -17, -8, 0, 8, 17, 28], SPREAD = [-0.9, 0.6, -0.4, 0, 0.4, -0.6, 0.9];
    const end = Math.max(0, ...events.map(e => e[0] + e[2] * spb)) + 1;
    const out = new Float32Array(Math.ceil(end * RATE) * 2);
    const blep = (t, dt) => t < dt ? (t /= dt, t + t - t * t - 1) : t > 1 - dt ? (t = (t - 1) / dt, t * t + t + t + 1) : 0;
    for (const [start, frequency, beats, velocity] of events) {
        const length = beats * spb, first = Math.round(start * RATE), frames = Math.round((length + 0.18) * RATE);
        const amplitude = Math.pow(velocity / 127, 1.5) * 0.09;
        const cutoff = 1800 + 5200 * velocity / 127, a = 1 - Math.exp(-2 * Math.PI * cutoff / RATE);
        const voices = DETUNE.map((cents, v) => ({ step: frequency * Math.pow(2, cents / 1200) / RATE,
            phase: (Math.sin(start * 1000 + frequency * 7 + v * 13.7) * 0.5 + 0.5) % 1, left: Math.cos((SPREAD[v] + 1) * Math.PI / 4), right: Math.sin((SPREAD[v] + 1) * Math.PI / 4) }));
        let l1 = 0, r1 = 0, l2 = 0, r2 = 0;
        for (let i = 0; i < frames; i++) {
            const t = i / RATE;
            let left = 0, right = 0;
            for (const voice of voices) {
                const s = 2 * voice.phase - 1 - blep(voice.phase, voice.step);
                voice.phase += voice.step; if (voice.phase >= 1) voice.phase -= 1;
                left += s * voice.left; right += s * voice.right;
            }
            l1 += a * (left - l1); l2 += a * (l1 - l2); r1 += a * (right - r1); r2 += a * (r1 - r2);
            const envelope = Math.min(1, t / 0.004) * (0.72 + 0.28 * Math.exp(-t / 0.25)) * (t > length ? Math.max(0, 1 - (t - length) / 0.18) : 1);
            out[(first + i) * 2] += l2 * amplitude * envelope;
            out[(first + i) * 2 + 1] += r2 * amplitude * envelope;
        }
    }
    return out;
}

// Electric zaps (jellyfish): crackles (sparse clicks through a band-pass), a clipped mains-like buzz and a falling
// "zzap" whine, all fading out. Each zap draws its randomness from its own seed, so both body copies are identical.
// events: [[seconds, velocity, duration (seconds), seed, pan (-1..1)]]
function renderZaps(events) {
    const end = Math.max(0, ...events.map(e => e[0] + e[2])) + 1;
    const out = new Float32Array(Math.ceil(end * RATE) * 2);
    for (const [start, velocity, duration, seed, pan = 0] of events) {
        let state = (seed >>> 0) || 1;
        const random = () => { state ^= state << 13; state ^= state >>> 17; state ^= state << 5; return (state >>> 0) / 4294967296; };
        const left = Math.cos((pan + 1) * Math.PI / 4), right = Math.sin((pan + 1) * Math.PI / 4);
        const amplitude = Math.pow(velocity / 127, 1.5) * 0.5;
        const frames = Math.round(duration * RATE), first = Math.round(start * RATE);
        const buzzHz = 90 + random() * 60, whineFrom = 2500 + random() * 2500;
        const f = 2 * Math.sin(Math.PI * 3500 / RATE), damping = 0.7;   // band-pass around 3.5 kHz (state variable filter)
        let low = 0, band = 0, crackle = 0, buzzPhase = 0, whinePhase = 0;
        for (let i = 0; i < frames; i++) {
            const t = i / RATE, x = t / duration;
            const envelope = (1 - Math.exp(-t / 0.002)) * Math.pow(1 - x, 1.5);
            crackle = random() < 0.02 * (1 - x) + 0.002 ? random() * 2 - 1 : crackle * 0.7;   // denser at the start
            low += f * band;
            band += f * (crackle - low - damping * band);
            buzzPhase += 2 * Math.PI * buzzHz * (1 + 0.1 * Math.sin(t * 60)) / RATE;
            const buzz = Math.tanh(4 * Math.sin(buzzPhase)) * 0.35 * (random() < 0.5 ? 1 : 0.6);
            whinePhase += 2 * Math.PI * whineFrom * Math.pow(0.15, x) / RATE;
            const whine = Math.sin(whinePhase) * 0.25 * Math.exp(-t / (duration * 0.3));
            const s = amplitude * envelope * (band * 2.5 + buzz + whine);
            out[(first + i) * 2] += s * left;
            out[(first + i) * 2 + 1] += s * right;
        }
    }
    return out;
}

// A crowd at a concert. "hey": every person shouts once - an aspirated "h", then a voiced "ei" glide through three
// formant filters (a sawtooth glottis), each with their own pitch, timing and place in the stereo. "cheer": a roar
// swelling and fading - noise through the voice band, wobbling as many people shout at once.
// Each event draws from its own seed, so both body copies are identical.
// events: [[seconds, velocity, kind ("hey" | "cheer"), seed, duration (seconds, cheers only)]]
function renderChant(events) {
    const end = Math.max(0, ...events.map(e => e[0] + (e[4] || 0.4))) + 1;
    const out = new Float32Array(Math.ceil(end * RATE) * 2);
    for (const [start, velocity, kind, seed, duration = 1.5] of events) {
        let state = (seed >>> 0) || 1;
        const random = () => { state ^= state << 13; state ^= state >>> 17; state ^= state << 5; return (state >>> 0) / 4294967296; };
        const amplitude = Math.pow(velocity / 127, 1.5) * (kind === "cheer" ? 0.12 : 0.22);
        const people = kind === "cheer" ? 8 : 12;
        for (let person = 0; person < people; person++) {
            const pan = random() * 1.6 - 0.8;
            const left = Math.cos((pan + 1) * Math.PI / 4), right = Math.sin((pan + 1) * Math.PI / 4);
            const first = Math.round((start + random() * (kind === "cheer" ? 0.2 : 0.035)) * RATE);
            if (kind === "cheer") {
                const length = duration * (0.8 + 0.3 * random()), frames = Math.round(length * RATE);
                const centre = 700 + random() * 1800, f = 2 * Math.sin(Math.PI * centre / RATE), q = 0.6;
                const wobbleHz = 3 + random() * 4, wobblePhase = random() * 6.283;
                let low = 0, band = 0;
                for (let i = 0; i < frames; i++) {
                    const t = i / RATE, x = t / length;
                    const envelope = Math.min(1, x / 0.15) * Math.min(1, (1 - x) / 0.45);
                    const wobble = 0.65 + 0.35 * Math.sin(2 * Math.PI * wobbleHz * t + wobblePhase);
                    low += f * band;
                    band += f * ((random() * 2 - 1) - low - q * band);
                    const s = amplitude * band * q * envelope * wobble;
                    out[(first + i) * 2] += s * left;
                    out[(first + i) * 2 + 1] += s * right;
                }
                continue;
            }
            const f0 = 230 + random() * 250;           // A crowd shouting: voices pushed high (men ~230 Hz, women and kids up to ~480)
            const length = 0.24 + random() * 0.08, frames = Math.round(length * RATE), breath = 0.035;
            const bands = [[0, 0], [0, 0], [0, 0]], gains = [1, 0.7, 0.4], q = 0.14;
            let phase = 0;
            for (let i = 0; i < frames; i++) {
                const t = i / RATE, x = t / length;
                const glide = Math.min(1, Math.max(0, (t - breath) / (length - breath)));   // "e" towards "i"
                const formants = [680 - 280 * glide, 1950 + 550 * glide, 2750 + 350 * glide];   // Shouted: the mouth wide open, then "i"
                phase += f0 * (1 + 0.1 * Math.sin(Math.PI * Math.min(1, x * 1.4))) / RATE;   // up then down: shouted
                if (phase >= 1) phase -= 1;
                const voiced = t < breath ? 0 : Math.min(1, (t - breath) / 0.008);
                const source = voiced * (2 * phase - 1) + (random() * 2 - 1) * (t < breath ? 0.8 : 0.1);
                let sample = 0;
                for (let k = 0; k < 3; k++) {
                    const f = 2 * Math.sin(Math.PI * formants[k] / RATE), st = bands[k];
                    st[0] += f * st[1];
                    st[1] += f * (source - st[0] - q * st[1]);
                    sample += st[1] * q * gains[k];
                }
                const envelope = Math.min(1, t / 0.006) * Math.exp(-Math.max(0, t - breath) / (length * 0.6)) * (1 - Math.pow(x, 4));
                const s = amplitude * Math.tanh(sample * 2) * envelope;
                out[(first + i) * 2] += s * left;
                out[(first + i) * 2 + 1] += s * right;
            }
        }
    }
    return out;
}

// The dubstep wobble: two detuned saws and a square an octave down through a resonant low-pass whose cutoff an LFO
// opens and closes at the given rate (the "wub wub"), then driven into a growl.
// events: [[seconds, frequency (Hz), duration (seconds), velocity, wobbles per second]]
function renderWobble(events) {
    const end = Math.max(0, ...events.map(e => e[0] + e[2])) + 0.5;
    const out = new Float32Array(Math.ceil(end * RATE) * 2);
    for (const [start, frequency, duration, velocity, rate] of events) {
        const amplitude = Math.pow(velocity / 127, 1.5) * 0.4;
        const frames = Math.round(duration * RATE), first = Math.round(start * RATE);
        let p1 = 0, p2 = 0.37, p3 = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0, b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        for (let i = 0; i < frames; i++) {
            const t = i / RATE;
            if (i % 32 === 0) {
                // The LFO starts closed (a "wub" opens on each beat), squared so it snaps open
                const lfo = 0.5 - 0.5 * Math.cos(2 * Math.PI * rate * t);
                const cutoff = 110 + 2600 * lfo * lfo, q = 5;
                const w = 2 * Math.PI * Math.min(cutoff, RATE * 0.45) / RATE, alpha = Math.sin(w) / (2 * q), cos = Math.cos(w), a0 = 1 + alpha;
                b0 = (1 - cos) / 2 / a0; b1 = (1 - cos) / a0; b2 = b0; a1 = -2 * cos / a0; a2 = (1 - alpha) / a0;
            }
            p1 += frequency * 1.006 / RATE; if (p1 >= 1) p1 -= 1;
            p2 += frequency * 0.994 / RATE; if (p2 >= 1) p2 -= 1;
            p3 += frequency * 0.5 / RATE; if (p3 >= 1) p3 -= 1;
            const x = (2 * p1 - 1) * 0.5 + (2 * p2 - 1) * 0.5 + (p3 < 0.5 ? 0.6 : -0.6);
            const y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x; y2 = y1; y1 = y;
            const envelope = Math.min(1, t / 0.004) * Math.min(1, (duration - t) / 0.02);
            const s = amplitude * Math.tanh(y * 2.2) * envelope;
            out[(first + i) * 2] += s;
            out[(first + i) * 2 + 1] += s;
        }
    }
    return out;
}

// ===== Effects (all time-invariant or periodic over the loop, so the loop seam stays seamless) =====

function lowpass(samples, cutoff, q = 0.707) {
    const w = 2 * Math.PI * cutoff / RATE, alpha = Math.sin(w) / (2 * q), cos = Math.cos(w), a0 = 1 + alpha;
    const b0 = (1 - cos) / 2 / a0, b1 = (1 - cos) / a0, b2 = b0, a1 = -2 * cos / a0, a2 = (1 - alpha) / a0;
    for (let ch = 0; ch < 2; ch++) {
        let x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        for (let i = ch; i < samples.length; i += 2) {
            const x = samples[i], y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x; y2 = y1; y1 = y; samples[i] = y;
        }
    }
    return samples;
}

// Two modulated delays (left and right in quadrature). The LFO rate is rounded so it fits the loop a whole number of times.
function chorus(samples, { mix = 0.4, delayMs = 14, depthMs = 2.5, rateHz = 0.7 }, loopSeconds) {
    const rateFit = loopSeconds ? Math.max(1, Math.round(rateHz * loopSeconds)) / loopSeconds : rateHz;
    const frames = samples.length / 2, out = new Float32Array(samples.length);
    for (let i = 0; i < frames; i++) {
        for (let ch = 0; ch < 2; ch++) {
            const d = (delayMs + depthMs * Math.sin(2 * Math.PI * rateFit * i / RATE + ch * Math.PI / 2)) * RATE / 1000;
            const j = i - d, k = Math.floor(j), frac = j - k;
            const wet = k >= 0 ? samples[k * 2 + ch] * (1 - frac) + samples[(k + 1) * 2 + ch] * frac : 0;
            out[i * 2 + ch] = samples[i * 2 + ch] * (1 - mix * 0.5) + wet * mix;
        }
    }
    return out;
}

// Sidechain pump (dance music's breathing): the level ducks on every beat, as if the kick pushed it down, then swells
// back over `release` of the beat. On the body's own beat grid, so both copies get the same curve
function pump(samples, { depth = 0.5, release = 0.6 }, stem) {
    const beatFrames = 60 / stem.tempo * RATE, intro = Math.round((stem.introSeconds || 0) * RATE);
    const loop = stem.loopSeconds ? Math.round(stem.loopSeconds * RATE) : 0, frames = samples.length / 2;
    for (let i = 0; i < frames; i++) {
        const local = loop && i >= intro ? (i - intro) % loop : i - intro;
        const phase = ((local / beatFrames) % 1 + 1) % 1;
        const x = Math.min(1, phase / release), duck = phase < 0.015 ? phase / 0.015 : 1 - x * x * (3 - 2 * x);
        const gain = 1 - depth * duck;
        samples[i * 2] *= gain; samples[i * 2 + 1] *= gain;
    }
    return samples;
}

// Ping-pong echo in time with the song, darker at every repeat
function echo(samples, { beats = 0.75, feedback = 0.3, mix = 0.2, tone = 3500 }, tempo) {
    const delay = Math.round(beats * 60 / tempo * RATE), frames = samples.length / 2;
    const wet = new Float32Array(samples.length), a = 1 - Math.exp(-2 * Math.PI * tone / RATE);
    const state = [0, 0];
    for (let i = delay; i < frames; i++) {
        for (let ch = 0; ch < 2; ch++) {
            const from = (i - delay) * 2 + (1 - ch);   // the other side: ping-pong
            state[ch] += a * (samples[from] * 0.5 + wet[from] * feedback - state[ch]);
            wet[i * 2 + ch] = state[ch];
        }
    }
    for (let i = 0; i < samples.length; i++) samples[i] += wet[i] * mix;
    return samples;
}

function applyEffects(samples, stem) {
    const fx = stem.fx || {};
    let out = new Float32Array(samples.length + FX_TAIL * RATE * 2);
    out.set(samples);
    if (fx.lowpass) out = lowpass(out, fx.lowpass);
    if (fx.chorus) out = chorus(out, fx.chorus, stem.loopSeconds);
    if (fx.echo) out = echo(out, fx.echo, stem.tempo);
    if (fx.pump) out = pump(out, fx.pump, stem);
    // Constant-power panning, a bit wider than written (center = unchanged level on both sides)
    const gain = Math.pow(10, (stem.gainDb || 0) / 20), pan = Math.max(-0.9, Math.min(0.9, (stem.pan || 0) * 1.5));
    const left = gain * Math.SQRT2 * Math.cos((pan + 1) * Math.PI / 4), right = gain * Math.SQRT2 * Math.sin((pan + 1) * Math.PI / 4);
    for (let i = 0; i < out.length; i += 2) { out[i] *= left; out[i + 1] *= right; }
    return out;
}

// ===== Songs =====

const font = new SoundFont(SOUNDFONT);
const only = process.argv[2];
const db = x => (20 * Math.log10(Math.max(x, 1e-9))).toFixed(1).padStart(6);

for (const song of fs.readdirSync(STEMS_MIDI)) {
    if (only && song !== only) continue;
    const files = fs.readdirSync(path.join(STEMS_MIDI, song)).filter(f => f.endsWith(".json") && f !== "mix.json").sort();

    // Render every instrument with its effects, summed into its group (groups keep the order they first appear in)
    const groups = new Map();
    for (const file of files) {
        const stem = JSON.parse(fs.readFileSync(path.join(STEMS_MIDI, song, file), "utf8"));
        const dry = stem.synth === "bubbles" ? renderBubbles(stem.events)
            : stem.synth === "ping" ? renderPings(stem.events)
            : stem.synth === "beeps" ? renderBeeps(stem.events)
            : stem.synth === "zaps" ? renderZaps(stem.events)
            : stem.synth === "chant" ? renderChant(stem.events)
            : stem.synth === "wobble" ? renderWobble(stem.events)
            : stem.synth === "theremin" ? renderTheremin(stem.events, stem.tempo)
            : stem.synth === "synthvibes" ? renderSynthVibes(stem.events, stem.tempo)
            : stem.synth === "riser" ? renderRisers(stem.events, stem.tempo)
            : stem.synth === "impact" ? renderImpacts(stem.events)
            : stem.synth === "supersaw" ? renderSupersaw(stem.events, stem.tempo)
            : render(font, stem, RATE);
        const wet = applyEffects(dry, stem);
        const group = groups.get(stem.group) || { parts: [], samples: new Float32Array(0) };
        if (wet.length > group.samples.length) { const grown = new Float32Array(wet.length); grown.set(group.samples); group.samples = grown; }
        for (let i = 0; i < wet.length; i++) group.samples[i] += wet[i];
        group.parts.push(stem.name);
        groups.set(stem.group, group);
    }

    // Balance: each group's loudness (while it plays) set relative to the reference group (target 0 in mix.json)
    const mixFile = path.join(STEMS_MIDI, song, "mix.json");
    const targets = fs.existsSync(mixFile) ? JSON.parse(fs.readFileSync(mixFile, "utf8")) : {};
    const measured = new Map([...groups].map(([name, g]) => [name, loudness(g.samples)]));
    const reference = Object.keys(targets).find(name => targets[name] === 0 && measured.has(name));
    for (const [name, g] of groups) {
        g.balance = reference && name in targets ? measured.get(reference) + targets[name] - measured.get(name) : 0;
        const gain = Math.pow(10, g.balance / 20);
        for (let i = 0; i < g.samples.length; i++) g.samples[i] *= gain;
    }

    // Same length for every group (trailing silence trimmed), one gain so the full mix peaks at -3 dB
    let frames = Math.max(...[...groups.values()].map(g => g.samples.length / 2));
    const mix = new Float32Array(frames * 2);
    for (const g of groups.values()) for (let i = 0; i < g.samples.length; i++) mix[i] += g.samples[i];
    while (frames > 0 && Math.abs(mix[frames * 2 - 1]) < 1e-5 && Math.abs(mix[frames * 2 - 2]) < 1e-5) frames--;
    frames = Math.min(mix.length / 2, frames + RATE / 10);
    let peak = 0, mixSquares = 0;
    for (let i = 0; i < frames * 2; i++) { peak = Math.max(peak, Math.abs(mix[i])); mixSquares += mix[i] * mix[i]; }
    const scale = 0.708 / peak;

    const out = path.join(STEMS_WAV, song);
    fs.mkdirSync(out, { recursive: true });
    for (const old of fs.readdirSync(out)) if (old.endsWith(".wav")) fs.rmSync(path.join(out, old));
    let index = 1;
    for (const [name, g] of groups) {
        const padded = new Float32Array(frames * 2);
        padded.set(g.samples.subarray(0, frames * 2));
        let squares = 0;
        for (let i = 0; i < padded.length; i++) squares += padded[i] * padded[i];
        const file = String(index++).padStart(2, "0") + "_" + name + ".wav";
        writeWav(path.join(out, file), padded, RATE, scale);
        console.log(("  " + file).padEnd(26), "rms", db(Math.sqrt(squares / padded.length) * scale), "dB  balance",
            (g.balance >= 0 ? "+" : "") + g.balance.toFixed(1), "dB  <-", g.parts.join(", "));
    }
    console.log(song + ": " + (frames / RATE).toFixed(1) + " s, mix rms " + db(Math.sqrt(mixSquares / (frames * 2)) * scale).trim() + " dB");
}

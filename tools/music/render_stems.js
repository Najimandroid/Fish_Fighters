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
    const gain = Math.pow(10, (stem.gainDb || 0) / 20), pan = stem.pan || 0;
    const left = gain * Math.min(1, 1 - pan), right = gain * Math.min(1, 1 + pan);
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
            : stem.synth === "zaps" ? renderZaps(stem.events)
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

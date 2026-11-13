// Mixes the stem groups of each piece into the file export_music.js reads (it replaces the Waveform session):
//   - the groups summed as render_stems.js balanced them (mix.json targets)
//   - a reverb on a send: the high instruments get more of it, the bass and the drums stay almost dry
//     (a reverb on the whole mix made the low end muddy); the send is filtered (350 Hz - 7.5 kHz) and pre-delayed
//   - the loudness target (-16 LUFS, jingles -15), then a look-ahead limiter at -1.5 dBFS
// The reverb is deterministic: both body copies are preceded by the same music, so their reverb is identical too
// and the game's loop point (in the second copy) joins them without a seam.
//
// Usage: node tools/music/mix_music.js [piece]   (all pieces without an argument)
// Reads audio_sources/stems/<piece>/*.wav (render_stems.js), audio_sources/midi/loops.json (the kind of each piece)
// Writes audio_sources/waveform/Fish Fighters Music/Exported/<piece>.wav (48 kHz, 32-bit float)

const fs = require("fs");
const path = require("path");
const { loudness, themeGainDb } = require("./loudness");

const ROOT = path.join(__dirname, "..", "..");
const STEMS = path.join(ROOT, "audio_sources", "stems");
const LOOPS = path.join(ROOT, "audio_sources", "midi", "loops.json");
const OUT = path.join(ROOT, "audio_sources", "waveform", "Fish Fighters Music", "Exported");
const RATE = 48000;

// ===== Settings =====
const SENDS = { bass: 0.03, drums: 0.12, percussion: 0.18, fx: 0.3 };   // Reverb send per group (others: DEFAULT_SEND)
const DEFAULT_SEND = 0.3;
const REVERB = { preDelay: 0.025, room: 0.82, damping: 0.3, lowCut: 350, highCut: 7500, wet: 0.7, tail: 3 };
const TARGET_LUFS = { jingle: -15, other: -16 };
const CEILING = Math.pow(10, -1.5 / 20);
const LOOKAHEAD = Math.round(0.005 * RATE), RELEASE = 0.08;

// ===== Files =====
function readWav(file) {
    const b = fs.readFileSync(file);
    let p = 12, format = null, data = null;
    while (p + 8 <= b.length) {
        const id = b.toString("ascii", p, p + 4), size = b.readUInt32LE(p + 4);
        if (id === "fmt ") format = { tag: b.readUInt16LE(p + 8), channels: b.readUInt16LE(p + 10), bits: b.readUInt16LE(p + 22) };
        if (id === "data") data = { start: p + 8, size };
        p += 8 + size + (size & 1);
    }
    if (!format || !data || format.channels !== 2) throw new Error("Unsupported WAV: " + file);
    const bytes = format.bits / 8, count = data.size / bytes;
    const samples = new Float32Array(count);
    for (let i = 0; i < count; i++) {
        const at = data.start + i * bytes;
        samples[i] = format.tag === 3 ? b.readFloatLE(at) : format.bits === 24 ? b.readIntLE(at, 3) / 8388608 : b.readInt16LE(at) / 32768;
    }
    return samples;
}
function writeFloatWav(file, samples) {
    const b = Buffer.alloc(44 + samples.length * 4);
    b.write("RIFF", 0); b.writeUInt32LE(36 + samples.length * 4, 4); b.write("WAVE", 8);
    b.write("fmt ", 12); b.writeUInt32LE(16, 16); b.writeUInt16LE(3, 20); b.writeUInt16LE(2, 22);
    b.writeUInt32LE(RATE, 24); b.writeUInt32LE(RATE * 8, 28); b.writeUInt16LE(8, 32); b.writeUInt16LE(32, 34);
    b.write("data", 36); b.writeUInt32LE(samples.length * 4, 40);
    for (let i = 0; i < samples.length; i++) b.writeFloatLE(samples[i], 44 + i * 4);
    fs.writeFileSync(file, b);
}

// ===== Filters =====
function biquad(type, cutoff) {
    const w = 2 * Math.PI * cutoff / RATE, alpha = Math.sin(w) / (2 * Math.SQRT1_2), cos = Math.cos(w), a0 = 1 + alpha;
    const c = type === "low"
        ? { b0: (1 - cos) / 2, b1: 1 - cos, b2: (1 - cos) / 2 }
        : { b0: (1 + cos) / 2, b1: -(1 + cos), b2: (1 + cos) / 2 };
    const k = { b0: c.b0 / a0, b1: c.b1 / a0, b2: c.b2 / a0, a1: -2 * cos / a0, a2: (1 - alpha) / a0 };
    let x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    return x => { const y = k.b0 * x + k.b1 * x1 + k.b2 * x2 - k.a1 * y1 - k.a2 * y2; x2 = x1; x1 = x; y2 = y1; y1 = y; return y; };
}

// ===== Reverb (Freeverb: 8 damped combs in parallel, 4 allpasses in series, per channel) =====
function reverb(send) {
    const frames = send.length / 2, tail = Math.round(REVERB.tail * RATE);
    const out = new Float32Array((frames + tail) * 2);
    const scale = RATE / 44100, spread = Math.round(23 * scale);
    const combTunings = [1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617].map(t => Math.round(t * scale));
    const allpassTunings = [556, 441, 341, 225].map(t => Math.round(t * scale));
    const feedback = REVERB.room * 0.28 + 0.7, damp = REVERB.damping * 0.4;
    const preDelay = Math.round(REVERB.preDelay * RATE);
    for (let channel = 0; channel < 2; channel++) {
        const offset = channel === 0 ? 0 : spread;
        const combs = combTunings.map(t => ({ buffer: new Float32Array(t + offset), index: 0, store: 0 }));
        const allpasses = allpassTunings.map(t => ({ buffer: new Float32Array(t + offset), index: 0 }));
        const lowCut = biquad("high", REVERB.lowCut), highCut = biquad("low", REVERB.highCut);
        for (let i = 0; i < frames + tail; i++) {
            const source = i - preDelay;
            const input = highCut(lowCut(source >= 0 && source < frames ? send[source * 2 + channel] : 0)) * 0.015;
            let sum = 0;
            for (const comb of combs) {
                const y = comb.buffer[comb.index];
                comb.store = y * (1 - damp) + comb.store * damp;
                comb.buffer[comb.index] = input + comb.store * feedback;
                if (++comb.index >= comb.buffer.length) comb.index = 0;
                sum += y;
            }
            for (const allpass of allpasses) {
                const buffered = allpass.buffer[allpass.index];
                allpass.buffer[allpass.index] = sum + buffered * 0.5;
                sum = buffered - sum;
                if (++allpass.index >= allpass.buffer.length) allpass.index = 0;
            }
            out[i * 2 + channel] = sum;
        }
    }
    return out;
}

// ===== Limiter: the gain each sample needs, looked ahead, smoothed (never above what the peaks need), released =====
function limit(samples) {
    const frames = samples.length / 2;
    const needed = new Float32Array(frames);
    for (let i = 0; i < frames; i++) {
        const peak = Math.max(Math.abs(samples[i * 2]), Math.abs(samples[i * 2 + 1]));
        needed[i] = peak > CEILING ? CEILING / peak : 1;
    }
    // The lowest need over the next LOOKAHEAD samples (a monotonic deque)
    const ahead = new Float32Array(frames), deque = [];
    for (let i = frames - 1; i >= 0; i--) {
        while (deque.length && needed[deque[deque.length - 1]] >= needed[i]) deque.pop();
        deque.push(i);
        while (deque[0] > i + LOOKAHEAD) deque.shift();
        ahead[i] = needed[deque[0]];
    }
    // Averaged over the previous LOOKAHEAD samples: a smooth attack that still reaches every peak's need in time
    // (before the start, the window counts as unity gain)
    const smooth = new Float32Array(frames);
    let sum = LOOKAHEAD;
    for (let i = 0; i < frames; i++) {
        sum += ahead[i] - (i >= LOOKAHEAD ? ahead[i - LOOKAHEAD] : 1);
        smooth[i] = Math.min(ahead[i], sum / LOOKAHEAD);
    }
    const release = 1 - Math.exp(-1 / (RELEASE * RATE));
    let gain = 1, reduction = 1;
    for (let i = 0; i < frames; i++) {
        gain = Math.min(smooth[i], gain + (1 - gain) * release);
        reduction = Math.min(reduction, gain);
        samples[i * 2] *= gain;
        samples[i * 2 + 1] *= gain;
    }
    return reduction;
}

// ===== Pieces =====
const loops = JSON.parse(fs.readFileSync(LOOPS, "utf8"));
const only = process.argv[2];
fs.mkdirSync(OUT, { recursive: true });

for (const piece of Object.keys(loops)) {
    if (only && piece !== only) continue;
    const folder = path.join(STEMS, piece);
    if (!fs.existsSync(folder)) { console.log(piece.padEnd(22), "no stems (render_stems.js first)"); continue; }

    // Dry groups summed, and the reverb send
    let dry = null, send = null;
    for (const file of fs.readdirSync(folder).filter(f => f.endsWith(".wav")).sort()) {
        const group = file.replace(/^\d+_/, "").replace(/\.wav$/, "");
        const samples = readWav(path.join(folder, file));
        if (!dry) { dry = new Float32Array(samples.length); send = new Float32Array(samples.length); }
        const amount = SENDS[group] ?? DEFAULT_SEND;
        for (let i = 0; i < samples.length; i++) { dry[i] += samples[i]; send[i] += samples[i] * amount; }
    }
    const wet = reverb(send);
    const mix = new Float32Array(wet.length);
    mix.set(dry);
    for (let i = 0; i < wet.length; i++) mix[i] += wet[i] * REVERB.wet;

    // Loudness target, then the limiter for the peaks
    // (the themes: -16 LUFS, or louder for a bass-heavy one so small speakers hear it as loud: themeGainDb)
    // (loudnessDb: a song meant to sit lower or higher than the others, set in compose_music.js: the plain target
    // plus that, without the small-speaker lift, which would undo a muffled theme)
    const offset = loops[piece].loudnessDb;
    const gainDb = loops[piece].kind === "jingle" ? TARGET_LUFS.jingle - loudness(mix)
        : offset ? TARGET_LUFS.other + offset - loudness(mix) : themeGainDb(mix);
    const gain = Math.pow(10, gainDb / 20);
    for (let i = 0; i < mix.length; i++) mix[i] *= gain;
    const reduction = limit(mix);

    writeFloatWav(path.join(OUT, piece + ".wav"), mix);
    console.log(piece.padEnd(22), (mix.length / 2 / RATE).toFixed(1).padStart(6) + " s",
        "  gain " + (20 * Math.log10(gain)).toFixed(1) + " dB", "  limiter " + (20 * Math.log10(reduction)).toFixed(1) + " dB at most");
}

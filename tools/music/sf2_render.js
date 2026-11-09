// A small SoundFont 2 sample player: reads an .sf2 file and renders notes to a stereo buffer.
// Enough of the SF2 spec for General MIDI banks: presets and instruments with their zones (key / velocity
// ranges, global zones), sample loops, tuning, volume envelope, attenuation, pan, a static low-pass filter
// and exclusive classes (open / closed hi-hat). No modulators beyond the default velocity curve, no effects:
// the reverb and the mix happen in Waveform.

const fs = require("fs");

// ===== Reading the file =====
function readChunks(buffer, start, end) {
    const chunks = {};
    let position = start;
    while (position + 8 <= end) {
        const id = buffer.toString("ascii", position, position + 4);
        const size = buffer.readUInt32LE(position + 4);
        const dataStart = position + 8;
        if (id === "LIST") {
            const listType = buffer.toString("ascii", dataStart, dataStart + 4);
            chunks[listType] = readChunks(buffer, dataStart + 4, dataStart + size);
        } else {
            chunks[id] = { start: dataStart, size };
        }
        position = dataStart + size + (size % 2);
    }
    return chunks;
}

function records(buffer, chunk, size, read) {
    const list = [];
    for (let p = chunk.start; p + size <= chunk.start + chunk.size; p += size) list.push(read(p));
    return list;
}

const GEN = {
    startAddrsOffset: 0, endAddrsOffset: 1, startloopAddrsOffset: 2, endloopAddrsOffset: 3, startAddrsCoarseOffset: 4,
    initialFilterFc: 8, initialFilterQ: 9, endAddrsCoarseOffset: 12, pan: 17, delayVolEnv: 33, attackVolEnv: 34, holdVolEnv: 35,
    decayVolEnv: 36, sustainVolEnv: 37, releaseVolEnv: 38, instrument: 41, keyRange: 43, velRange: 44,
    startloopAddrsCoarseOffset: 45, initialAttenuation: 48, endloopAddrsCoarseOffset: 50, coarseTune: 51, fineTune: 52,
    sampleID: 53, sampleModes: 54, scaleTuning: 56, exclusiveClass: 57, overridingRootKey: 58,
};
const DEFAULTS = {
    [GEN.initialFilterFc]: 13500, [GEN.delayVolEnv]: -12000, [GEN.attackVolEnv]: -12000, [GEN.holdVolEnv]: -12000,
    [GEN.decayVolEnv]: -12000, [GEN.releaseVolEnv]: -12000, [GEN.scaleTuning]: 100, [GEN.overridingRootKey]: -1,
};

class SoundFont {
    constructor(file) {
        const buffer = fs.readFileSync(file);
        if (buffer.toString("ascii", 0, 4) !== "RIFF" || buffer.toString("ascii", 8, 12) !== "sfbk") throw new Error("Not a SoundFont: " + file);
        const riff = readChunks(buffer, 12, buffer.length);
        const pdta = riff.pdta;

        // 16-bit samples as floats
        const smpl = riff.sdta.smpl;
        this.data = new Float32Array(smpl.size / 2);
        for (let i = 0; i < this.data.length; i++) this.data[i] = buffer.readInt16LE(smpl.start + i * 2) / 32768;

        const name = p => buffer.toString("ascii", p, p + 20).replace(/\0.*$/, "");
        const presets = records(buffer, pdta.phdr, 38, p => ({ name: name(p), program: buffer.readUInt16LE(p + 20), bank: buffer.readUInt16LE(p + 22), bag: buffer.readUInt16LE(p + 24) }));
        const pbag = records(buffer, pdta.pbag, 4, p => buffer.readUInt16LE(p));
        const pgen = records(buffer, pdta.pgen, 4, p => ({ op: buffer.readUInt16LE(p), amount: buffer.readInt16LE(p + 2), lo: buffer[p + 2], hi: buffer[p + 3] }));
        const insts = records(buffer, pdta.inst, 22, p => ({ name: name(p), bag: buffer.readUInt16LE(p + 20) }));
        const ibag = records(buffer, pdta.ibag, 4, p => buffer.readUInt16LE(p));
        const igen = records(buffer, pdta.igen, 4, p => ({ op: buffer.readUInt16LE(p), amount: buffer.readInt16LE(p + 2), lo: buffer[p + 2], hi: buffer[p + 3] }));
        this.samples = records(buffer, pdta.shdr, 46, p => ({
            name: name(p), start: buffer.readUInt32LE(p + 20), end: buffer.readUInt32LE(p + 24),
            loopStart: buffer.readUInt32LE(p + 28), loopEnd: buffer.readUInt32LE(p + 32), rate: buffer.readUInt32LE(p + 36),
            pitch: buffer[p + 40], correction: buffer.readInt8(p + 41),
        }));

        // Zones: a list of generator maps; the first one is global if it lacks the terminal generator
        const zones = (bags, gens, from, to, terminal) => {
            const list = [];
            for (let b = from; b < to; b++) {
                const zone = {};
                for (let g = bags[b]; g < bags[b + 1]; g++) zone[gens[g].op] = gens[g];
                list.push(zone);
            }
            const global = list.length && list[0][terminal] === undefined ? list.shift() : {};
            return { global, zones: list.filter(z => z[terminal] !== undefined) };
        };
        this.instruments = insts.slice(0, -1).map((inst, i) => zones(ibag, igen, inst.bag, insts[i + 1].bag, GEN.sampleID));
        this.presets = new Map();
        presets.slice(0, -1).forEach((preset, i) => {
            this.presets.set(preset.bank * 1000 + preset.program, { name: preset.name, ...zones(pbag, pgen, preset.bag, presets[i + 1].bag, GEN.instrument) });
        });
    }

    preset(bank, program) {
        return this.presets.get(bank * 1000 + program) || this.presets.get(bank * 1000) || this.presets.get(0);
    }

    // Everything a voice needs to play this key at this velocity: one entry per matching instrument zone
    regions(bank, program, key, velocity) {
        const preset = this.preset(bank, program);
        const inRange = (zone, global, op, value) => {
            const range = zone[op] || global[op];
            return !range || (value >= range.lo && value <= range.hi);
        };
        const result = [];
        for (const pzone of preset.zones) {
            if (!inRange(pzone, preset.global, GEN.keyRange, key) || !inRange(pzone, preset.global, GEN.velRange, velocity)) continue;
            const instrument = this.instruments[pzone[GEN.instrument].amount];
            for (const izone of instrument.zones) {
                if (!inRange(izone, instrument.global, GEN.keyRange, key) || !inRange(izone, instrument.global, GEN.velRange, velocity)) continue;
                // Instrument values (zone over global), plus the preset's offsets (zone over global)
                const value = op => {
                    const base = izone[op] ? izone[op].amount : instrument.global[op] ? instrument.global[op].amount : (DEFAULTS[op] ?? 0);
                    const offset = pzone[op] ? pzone[op].amount : preset.global[op] ? preset.global[op].amount : 0;
                    return op === GEN.sampleID || op === GEN.sampleModes || op === GEN.overridingRootKey || op === GEN.exclusiveClass ? base : base + offset;
                };
                result.push({ value, sample: this.samples[izone[GEN.sampleID].amount] });
            }
        }
        return result;
    }
}

// ===== Rendering =====
const timecents = tc => Math.pow(2, tc / 1200);
const centibels = cb => Math.pow(10, -cb / 200);

// bend: semitones the pitch slides by over the held part of the note (0 = none)
function renderVoice(font, region, key, velocity, start, length, out, rate, gain, bend = 0) {
    const v = region.value, sample = region.sample;
    const data = font.data;

    const sampleStart = sample.start + v(GEN.startAddrsOffset) + v(GEN.startAddrsCoarseOffset) * 32768;
    const sampleEnd = sample.end + v(GEN.endAddrsOffset) + v(GEN.endAddrsCoarseOffset) * 32768;
    const loopStart = sample.loopStart + v(GEN.startloopAddrsOffset) + v(GEN.startloopAddrsCoarseOffset) * 32768;
    const loopEnd = sample.loopEnd + v(GEN.endloopAddrsOffset) + v(GEN.endloopAddrsCoarseOffset) * 32768;
    const mode = v(GEN.sampleModes) & 3;
    const isLooping = (mode === 1 || mode === 3) && loopEnd > loopStart + 2;

    // Pitch
    const rootKey = v(GEN.overridingRootKey) >= 0 ? v(GEN.overridingRootKey) : sample.pitch;
    const cents = (key - rootKey) * v(GEN.scaleTuning) + v(GEN.coarseTune) * 100 + v(GEN.fineTune) + sample.correction;
    const step = Math.pow(2, cents / 1200) * sample.rate / rate;

    // Level: attenuation, the default velocity curve (concave), pan
    const velocityGain = Math.pow(velocity / 127, 2);
    const amplitude = gain * centibels(Math.max(0, v(GEN.initialAttenuation)) * 0.4) * velocityGain;
    const pan = Math.max(-500, Math.min(500, v(GEN.pan))) / 500;
    const left = Math.cos((pan + 1) * Math.PI / 4) * amplitude, right = Math.sin((pan + 1) * Math.PI / 4) * amplitude;

    // Volume envelope
    const delay = timecents(v(GEN.delayVolEnv)), attack = timecents(v(GEN.attackVolEnv)), hold = timecents(v(GEN.holdVolEnv));
    const decay = timecents(v(GEN.decayVolEnv)), release = Math.min(3, timecents(v(GEN.releaseVolEnv)));
    const sustain = centibels(Math.max(0, Math.min(1440, v(GEN.sustainVolEnv))));
    const envelopeAt = t => {
        if (t < delay) return 0;
        t -= delay;
        if (t < attack) return t / attack;
        t -= attack;
        if (t < hold) return 1;
        t -= hold;
        // Decay in dB (linear in the log domain), to the sustain level
        return t < decay ? Math.pow(sustain, t / decay) : sustain;
    };

    // Low-pass (biquad), cutoff from initialFilterFc, lowered at low velocities like the default modulator
    const cutoffCents = v(GEN.initialFilterFc) - (velocity < 64 ? 2400 * (1 - velocity / 64) * 0.25 : 0);
    const cutoff = Math.min(rate * 0.45, 8.176 * Math.pow(2, cutoffCents / 1200));
    const q = Math.max(0.707, Math.pow(10, Math.max(0, v(GEN.initialFilterQ)) / 200));
    const useFilter = cutoff < 18000;
    let b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    if (useFilter) {
        const w = 2 * Math.PI * cutoff / rate, alpha = Math.sin(w) / (2 * q), cos = Math.cos(w), a0 = 1 + alpha;
        b0 = (1 - cos) / 2 / a0; b1 = (1 - cos) / a0; b2 = b0; a1 = -2 * cos / a0; a2 = (1 - alpha) / a0;
    }
    let x1 = 0, x2 = 0, y1 = 0, y2 = 0;

    const first = Math.round(start * rate);
    const heldFrames = Math.round(length * rate);
    const totalFrames = heldFrames + Math.round(release * rate);
    let position = sampleStart;
    let releaseFrom = 0;

    for (let f = 0; f < totalFrames; f++) {
        const index = first + f;
        if (index * 2 + 1 >= out.length) break;

        // Envelope (release from the level reached when the key went up)
        let level;
        if (f < heldFrames) { level = envelopeAt(f / rate); releaseFrom = level; }
        else level = releaseFrom * Math.pow(0.001, (f - heldFrames) / (release * rate));   // -60 dB over the release
        if (level < 1e-5 && f > heldFrames) break;

        // Sample (linear interpolation), looping while the key is held (mode 3) or always (mode 1)
        if (isLooping && (mode === 1 || f < heldFrames)) {
            while (position >= loopEnd) position -= loopEnd - loopStart;
        } else if (position >= sampleEnd - 1) break;
        const i = Math.floor(position), frac = position - i;
        let s = data[i] + (data[i + 1] - data[i]) * frac;
        position += bend ? step * Math.pow(2, bend * Math.min(1, f / Math.max(1, heldFrames)) / 12) : step;

        if (useFilter) {
            const y = b0 * s + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = s; y2 = y1; y1 = y; s = y;
        }
        out[index * 2] += s * level * left;
        out[index * 2 + 1] += s * level * right;
    }
}

// notes: [[startSeconds, lengthSeconds, key, velocity, bend?], ...]. Returns interleaved stereo Float32Array.
// Channel 10 plays the drum kit numbered program (bank 128: 0 Standard, 8 Room, 16 Power, 40 Brush...)
function render(font, { notes, channel, program, volume = 100 }, rate = 48000, tail = 3) {
    const isDrums = channel === 10;
    const bank = isDrums ? 128 : 0;
    const end = Math.max(0, ...notes.map(n => n[0] + n[1])) + tail;
    const out = new Float32Array(Math.ceil(end * rate) * 2);
    const gain = Math.pow(volume / 127, 2);

    // Exclusive classes (hi-hats): a new note cuts the previous one of its class
    const sorted = [...notes].sort((a, b) => a[0] - b[0]);
    const lastByClass = new Map();
    const cuts = sorted.map(() => Infinity);
    sorted.forEach((note, i) => {
        for (const region of font.regions(bank, program, note[2], note[3])) {
            const exclusive = region.value(GEN.exclusiveClass);
            if (!exclusive) continue;
            if (lastByClass.has(exclusive)) { const j = lastByClass.get(exclusive); cuts[j] = Math.min(cuts[j], note[0] - sorted[j][0]); }
            lastByClass.set(exclusive, i);
        }
    });

    sorted.forEach(([start, length, key, velocity, bend = 0], i) => {
        for (const region of font.regions(bank, program, key, velocity)) {
            // Drums play their whole sample (one-shots), unless cut by their exclusive class
            const held = isDrums ? Math.min(cuts[i], 4) : length;
            renderVoice(font, region, key, velocity, start, held, out, rate, gain, bend);
        }
    });
    return out;
}

// 24-bit stereo WAV
function writeWav(file, samples, rate, scale = 1) {
    const frames = samples.length / 2;
    const buffer = Buffer.alloc(44 + frames * 6);
    buffer.write("RIFF", 0); buffer.writeUInt32LE(36 + frames * 6, 4); buffer.write("WAVE", 8);
    buffer.write("fmt ", 12); buffer.writeUInt32LE(16, 16); buffer.writeUInt16LE(1, 20); buffer.writeUInt16LE(2, 22);
    buffer.writeUInt32LE(rate, 24); buffer.writeUInt32LE(rate * 6, 28); buffer.writeUInt16LE(6, 32); buffer.writeUInt16LE(24, 34);
    buffer.write("data", 36); buffer.writeUInt32LE(frames * 6, 40);
    for (let i = 0; i < samples.length; i++) {
        const value = Math.max(-8388608, Math.min(8388607, Math.round(samples[i] * scale * 8388607)));
        buffer.writeIntLE(value, 44 + i * 3, 3);
    }
    fs.writeFileSync(file, buffer);
}

module.exports = { SoundFont, render, writeWav };

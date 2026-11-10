// A small SoundFont 2 sample player: reads an .sf2 file and renders notes to a stereo buffer.
// Enough of the SF2 spec for General MIDI banks: presets and instruments with their zones (key / velocity
// ranges, global zones), sample loops, tuning, volume envelope, attenuation, pan, exclusive classes (open /
// closed hi-hat), and what makes the instruments breathe: modulators (velocity and key sources, the curve
// shapes, FluidSynth's defaults), the modulation envelope and the vibrato and modulation LFOs moving the
// pitch and a resonant low-pass filter. Without them, many GeneralUser GS instruments (vibraphone, Rhodes,
// basses, horns, trumpets) stayed behind a closed filter. No effects: the reverb and the mix happen in Waveform.

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
    modLfoToPitch: 5, vibLfoToPitch: 6, modEnvToPitch: 7, initialFilterFc: 8, initialFilterQ: 9, modLfoToFilterFc: 10,
    modEnvToFilterFc: 11, endAddrsCoarseOffset: 12, modLfoToVolume: 13, pan: 17,
    delayModLFO: 21, freqModLFO: 22, delayVibLFO: 23, freqVibLFO: 24,
    delayModEnv: 25, attackModEnv: 26, holdModEnv: 27, decayModEnv: 28, sustainModEnv: 29, releaseModEnv: 30,
    keynumToModEnvHold: 31, keynumToModEnvDecay: 32,
    delayVolEnv: 33, attackVolEnv: 34, holdVolEnv: 35, decayVolEnv: 36, sustainVolEnv: 37, releaseVolEnv: 38,
    keynumToVolEnvHold: 39, keynumToVolEnvDecay: 40, instrument: 41, keyRange: 43, velRange: 44,
    startloopAddrsCoarseOffset: 45, initialAttenuation: 48, endloopAddrsCoarseOffset: 50, coarseTune: 51, fineTune: 52,
    sampleID: 53, sampleModes: 54, scaleTuning: 56, exclusiveClass: 57, overridingRootKey: 58,
};
const DEFAULTS = {
    [GEN.initialFilterFc]: 13500, [GEN.delayModLFO]: -12000, [GEN.delayVibLFO]: -12000,
    [GEN.delayModEnv]: -12000, [GEN.attackModEnv]: -12000, [GEN.holdModEnv]: -12000, [GEN.decayModEnv]: -12000, [GEN.releaseModEnv]: -12000,
    [GEN.delayVolEnv]: -12000, [GEN.attackVolEnv]: -12000, [GEN.holdVolEnv]: -12000,
    [GEN.decayVolEnv]: -12000, [GEN.releaseVolEnv]: -12000, [GEN.scaleTuning]: 100, [GEN.overridingRootKey]: -1,
};

// ===== Modulators (SF2 8.2) =====
// A modulator adds amount x source x amountSource to a generator, per note. Only note-on sources change between
// notes here (velocity, key); MIDI controllers sit at their power-on values (no CC messages are rendered).
const CC_DEFAULTS = { 1: 0, 7: 100, 10: 64, 11: 127, 91: 40, 93: 0 };
// The curves as FluidSynth computes them (the reference player GeneralUser GS was tuned on)
const concave = x => Math.min(1, Math.max(0, x >= 1 ? 1 : -(5 / 6) * Math.log10(1 - x)));
const convex = x => Math.min(1, Math.max(0, x <= 0 ? 0 : 1 + (5 / 6) * Math.log10(x)));
function sourceValue(oper, key, velocity) {
    const index = oper & 0x7f, isCC = (oper >> 7) & 1, isNegative = (oper >> 8) & 1, isBipolar = (oper >> 9) & 1, type = (oper >> 10) & 0x3f;
    let raw;
    if (!isCC) {
        if (index === 0) return 1;                 // No controller: full amount
        if (index === 127) return null;            // Linked modulators: not supported
        raw = index === 2 ? velocity : index === 3 ? key : index === 14 ? 64 : index === 16 ? 2 : 0;   // Wheel centred, 2 semitones range
    } else raw = CC_DEFAULTS[index] ?? 0;
    let u = raw / 127;
    if (isNegative) u = 1 - u;
    const shape = x => type === 1 ? concave(x) : type === 2 ? convex(x) : type === 3 ? (x >= 0.5 ? 1 : 0) : x;
    if (!isBipolar) return shape(u);
    return u > 0.5 ? shape(2 * u - 1) : -shape(1 - 2 * u);
}
// FluidSynth's default "velocity to filter cutoff" (the only default modulator that isn't constant here, besides
// velocity to attenuation, which the velocity curve in renderVoice does)
const DEFAULT_MODULATORS = [{ src: 0x0102, dest: GEN.initialFilterFc, amount: -2400, amtSrc: 0x0C02, trans: 0 }];
const sameModulator = (a, b) => a.src === b.src && a.dest === b.dest && a.amtSrc === b.amtSrc && a.trans === b.trans;
// Later lists override identical modulators of the earlier ones (defaults < global zone < zone)
function mergeModulators(...lists) {
    const merged = [];
    for (const list of lists) for (const mod of list) {
        const i = merged.findIndex(m => sameModulator(m, mod));
        if (i >= 0) merged[i] = mod; else merged.push(mod);
    }
    return merged;
}

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
        const bag = p => ({ gen: buffer.readUInt16LE(p), mod: buffer.readUInt16LE(p + 2) });
        const gen = p => ({ op: buffer.readUInt16LE(p), amount: buffer.readInt16LE(p + 2), lo: buffer[p + 2], hi: buffer[p + 3] });
        const mod = p => ({ src: buffer.readUInt16LE(p), dest: buffer.readUInt16LE(p + 2), amount: buffer.readInt16LE(p + 4),
            amtSrc: buffer.readUInt16LE(p + 6), trans: buffer.readUInt16LE(p + 8) });
        const pbag = records(buffer, pdta.pbag, 4, bag);
        const pgen = records(buffer, pdta.pgen, 4, gen);
        const pmod = records(buffer, pdta.pmod, 10, mod);
        const insts = records(buffer, pdta.inst, 22, p => ({ name: name(p), bag: buffer.readUInt16LE(p + 20) }));
        const ibag = records(buffer, pdta.ibag, 4, bag);
        const igen = records(buffer, pdta.igen, 4, gen);
        const imod = records(buffer, pdta.imod, 10, mod);
        this.samples = records(buffer, pdta.shdr, 46, p => ({
            name: name(p), start: buffer.readUInt32LE(p + 20), end: buffer.readUInt32LE(p + 24),
            loopStart: buffer.readUInt32LE(p + 28), loopEnd: buffer.readUInt32LE(p + 32), rate: buffer.readUInt32LE(p + 36),
            pitch: buffer[p + 40], correction: buffer.readInt8(p + 41),
        }));

        // Zones: a list of generator maps (with their modulators in .mods); the first one is global if it lacks
        // the terminal generator
        const zones = (bags, gens, mods, from, to, terminal) => {
            const list = [];
            for (let b = from; b < to; b++) {
                const zone = {};
                for (let g = bags[b].gen; g < bags[b + 1].gen; g++) zone[gens[g].op] = gens[g];
                Object.defineProperty(zone, "mods", { value: mods.slice(bags[b].mod, bags[b + 1].mod), enumerable: false });
                list.push(zone);
            }
            const global = list.length && list[0][terminal] === undefined ? list.shift() : Object.defineProperty({}, "mods", { value: [] });
            return { global, zones: list.filter(z => z[terminal] !== undefined) };
        };
        this.instruments = insts.slice(0, -1).map((inst, i) => zones(ibag, igen, imod, inst.bag, insts[i + 1].bag, GEN.sampleID));
        this.presets = new Map();
        presets.slice(0, -1).forEach((preset, i) => {
            this.presets.set(preset.bank * 1000 + preset.program, { name: preset.name, ...zones(pbag, pgen, pmod, preset.bag, presets[i + 1].bag, GEN.instrument) });
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
                // Modulators: the instrument's (defaults < global zone < zone) plus the preset's (global < zone),
                // summed per destination for this key and velocity. Attenuation ones are left out: the velocity
                // curve does the default one, and the controller ones are constant here
                const modulation = {};
                const modulators = [...mergeModulators(DEFAULT_MODULATORS, instrument.global.mods, izone.mods),
                    ...mergeModulators(preset.global.mods, pzone.mods)];
                for (const m of modulators) {
                    if (m.dest & 0x8000 || m.dest === GEN.initialAttenuation) continue;
                    const a = sourceValue(m.src, key, velocity), b = sourceValue(m.amtSrc, key, velocity);
                    if (a === null || b === null) continue;
                    let amount = m.amount * a * b;
                    if (m.trans === 2) amount = Math.abs(amount);
                    modulation[m.dest] = (modulation[m.dest] || 0) + amount;
                }
                // Instrument values (zone over global), plus the preset's offsets (zone over global), plus modulation
                const value = op => {
                    const base = izone[op] ? izone[op].amount : instrument.global[op] ? instrument.global[op].amount : (DEFAULTS[op] ?? 0);
                    const offset = pzone[op] ? pzone[op].amount : preset.global[op] ? preset.global[op].amount : 0;
                    return op === GEN.sampleID || op === GEN.sampleModes || op === GEN.overridingRootKey || op === GEN.exclusiveClass ? base
                        : base + offset + (modulation[op] || 0);
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

    // Pitch (cents from the sample's root), moved by the modulation envelope and the LFOs below
    const rootKey = v(GEN.overridingRootKey) >= 0 ? v(GEN.overridingRootKey) : sample.pitch;
    const cents = (key - rootKey) * v(GEN.scaleTuning) + v(GEN.coarseTune) * 100 + v(GEN.fineTune) + sample.correction;
    const baseStep = Math.pow(2, cents / 1200) * sample.rate / rate;

    // Level: attenuation, the default velocity curve (concave), pan
    const velocityGain = Math.pow(velocity / 127, 2);
    const amplitude = gain * centibels(Math.max(0, v(GEN.initialAttenuation)) * 0.4) * velocityGain;
    const pan = Math.max(-500, Math.min(500, v(GEN.pan))) / 500;
    const left = Math.cos((pan + 1) * Math.PI / 4) * amplitude, right = Math.sin((pan + 1) * Math.PI / 4) * amplitude;

    // Volume envelope (hold and decay scaled by the key, 60 = unchanged)
    const delay = timecents(v(GEN.delayVolEnv)), attack = timecents(v(GEN.attackVolEnv));
    const hold = timecents(v(GEN.holdVolEnv) + v(GEN.keynumToVolEnvHold) * (60 - key));
    const decay = timecents(v(GEN.decayVolEnv) + v(GEN.keynumToVolEnvDecay) * (60 - key));
    const release = Math.min(3, timecents(v(GEN.releaseVolEnv)));
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

    // Modulation envelope (0 to 1, linear segments): opens the filter and bends the pitch of many instruments
    const modDelay = timecents(v(GEN.delayModEnv)), modAttack = timecents(v(GEN.attackModEnv));
    const modHold = timecents(v(GEN.holdModEnv) + v(GEN.keynumToModEnvHold) * (60 - key));
    const modDecay = timecents(v(GEN.decayModEnv) + v(GEN.keynumToModEnvDecay) * (60 - key));
    const modSustain = 1 - Math.max(0, Math.min(1000, v(GEN.sustainModEnv))) / 1000;
    const modRelease = Math.min(3, timecents(v(GEN.releaseModEnv)));
    const modEnvAt = t => {
        if (t < modDelay) return 0;
        t -= modDelay;
        if (t < modAttack) return t / modAttack;
        t -= modAttack;
        if (t < modHold) return 1;
        t -= modHold;
        return t < modDecay ? 1 - (1 - modSustain) * t / modDecay : modSustain;
    };
    const modEnvToPitch = v(GEN.modEnvToPitch), modEnvToFilter = v(GEN.modEnvToFilterFc);

    // LFOs: triangles from 0 after their delay. Vibrato: pitch; modulation: pitch, filter, volume
    const triangle = phase => 1 - 4 * Math.abs(((phase + 0.25) % 1) - 0.5);
    const vibDelay = timecents(v(GEN.delayVibLFO)), vibFreq = 8.176 * Math.pow(2, v(GEN.freqVibLFO) / 1200), vibToPitch = v(GEN.vibLfoToPitch);
    const modLfoDelay = timecents(v(GEN.delayModLFO)), modLfoFreq = 8.176 * Math.pow(2, v(GEN.freqModLFO) / 1200);
    const modLfoToPitch = v(GEN.modLfoToPitch), modLfoToFilter = v(GEN.modLfoToFilterFc), modLfoToVolume = v(GEN.modLfoToVolume);
    const isPitchModulated = modEnvToPitch !== 0 || vibToPitch !== 0 || modLfoToPitch !== 0;

    // Low-pass (biquad) from initialFilterFc (with its modulators), the modulation envelope and LFO; like FluidSynth:
    // off above 13500 cents when nothing moves it, at least 1500 cents, Q in cB (0 = 0.707), level compensated
    const filterFc = v(GEN.initialFilterFc);
    const qLinear = Math.pow(10, (Math.max(0, v(GEN.initialFilterQ)) / 10 - 3.01) / 20);
    const filterGain = 1 / Math.sqrt(Math.max(1, qLinear));
    const useFilter = filterFc < 13500 || modEnvToFilter !== 0 || modLfoToFilter !== 0;
    let b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    const setFilter = fcCents => {
        const cutoff = Math.min(rate * 0.45, 8.176 * Math.pow(2, Math.max(1500, Math.min(13500, fcCents)) / 1200));
        const w = 2 * Math.PI * cutoff / rate, alpha = Math.sin(w) / (2 * qLinear), cos = Math.cos(w), a0 = 1 + alpha;
        b0 = (1 - cos) / 2 / a0 * filterGain; b1 = (1 - cos) / a0 * filterGain; b2 = b0; a1 = -2 * cos / a0; a2 = (1 - alpha) / a0;
    };
    let x1 = 0, x2 = 0, y1 = 0, y2 = 0;

    const BLOCK = 32;   // Pitch, filter and LFO volume are updated every 32 samples
    const first = Math.round(start * rate);
    const heldFrames = Math.round(length * rate);
    const totalFrames = heldFrames + Math.round(Math.max(release, modRelease) * rate);
    let position = sampleStart;
    let releaseFrom = 0, modReleaseFrom = 0;
    let step = baseStep, lfoVolume = 1;

    for (let f = 0; f < totalFrames; f++) {
        const index = first + f;
        if (index * 2 + 1 >= out.length) break;
        const t = f / rate;

        // Envelope (release from the level reached when the key went up)
        let level;
        if (f < heldFrames) { level = envelopeAt(t); releaseFrom = level; }
        else level = releaseFrom * Math.pow(0.001, (f - heldFrames) / (release * rate));   // -60 dB over the release
        if (level < 1e-5 && f > heldFrames) break;

        if (f % BLOCK === 0) {
            let modEnv;
            if (f < heldFrames) { modEnv = modEnvAt(t); modReleaseFrom = modEnv; }
            else modEnv = Math.max(0, modReleaseFrom * (1 - (f - heldFrames) / (modRelease * rate)));
            const vib = t > vibDelay ? triangle((t - vibDelay) * vibFreq) : 0;
            const modLfo = t > modLfoDelay ? triangle((t - modLfoDelay) * modLfoFreq) : 0;
            const bendCents = bend ? bend * 100 * Math.min(1, f / Math.max(1, heldFrames)) : 0;
            step = isPitchModulated || bend
                ? baseStep * Math.pow(2, (modEnv * modEnvToPitch + vib * vibToPitch + modLfo * modLfoToPitch + bendCents) / 1200)
                : baseStep;
            if (useFilter) setFilter(filterFc + modEnv * modEnvToFilter + modLfo * modLfoToFilter);
            lfoVolume = modLfoToVolume ? centibels(-modLfo * modLfoToVolume) : 1;
        }

        // Sample (linear interpolation), looping while the key is held (mode 3) or always (mode 1)
        if (isLooping && (mode === 1 || f < heldFrames)) {
            while (position >= loopEnd) position -= loopEnd - loopStart;
        } else if (position >= sampleEnd - 1) break;
        const i = Math.floor(position), frac = position - i;
        let s = data[i] + (data[i + 1] - data[i]) * frac;
        position += step;

        if (useFilter) {
            const y = b0 * s + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = s; y2 = y1; y1 = y; s = y;
        }
        out[index * 2] += s * level * lfoVolume * left;
        out[index * 2 + 1] += s * level * lfoVolume * right;
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

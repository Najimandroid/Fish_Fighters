// Fish Fighters' music, written as code: one motif, twelve themes and three jingles built from it.
// Writes a Standard MIDI File per piece into audio_sources/midi/, one stem per instrument into
// audio_sources/midi/stems/<piece>/ (MIDI + the notes in seconds for render_stems.js), and audio_sources/midi/loops.json.
//
// Usage: node tools/music/compose_music.js
//
// The motif (1 bar, F major): C F A | G F C' - a rising fourth then a third, like a small fanfare (sol do mi re do sol').
// Underwater colour: tuned percussion (steel drums, marimba, vibraphone, kalimba), harp glissandos, a chorused
// electric piano, fretless bass, conga and shaker grooves, major 7th and lydian chords, synthesized bubbles and sonar
// pings, echoes and muffled pads (the effects are applied by render_stems.js).
//
// Every theme loops like The Battle Cats' stage music: intro, then A (the tune), B (a different middle),
// C (a breakdown, on the longer themes) and A' (the tune again, fuller), back to A.
//   theme_title      "Fish Fighters!" 168 BPM  title screen: ska-punk brass and steel drums, power chords, organ
//   theme_base       "Fish Tank"      112 BPM  the Fish Tank menus: aquarium funk, vibraphone, Rhodes, muted guitar, whistle
//   theme_default    "Reef Rumble"    152 BPM  calypso: steel drums, brass answers, skanking electric piano
//   theme_calm       "Kelp Garden"    100 BPM  underwater bossa: vibraphone, harp, kalimba (easy stages)
//   theme_serious    "Deep Current"   138 BPM  horns over a pizzicato ostinato, a clarinet that plans ahead
//   theme_stressful  "Pressure Alarm" 172 BPM  driving strings and bass, brass, frantic steel drums, sonar alarm
//   theme_boss       "Leviathan"      160 BPM  brass, choir, trombone riff, timpani, the leviathan's voice
//   theme_shanty     "Sunken Galleon" 126 BPM  12/8 sea-shanty jig: fiddle, accordion, tuba, stomps; a music box in the wreck
//   theme_jelly      "Jellyfish Lair" 124 BPM  dark, phrygian: an electric-eel lead, bowed glass, pulses, electric zaps
//   theme_jelly_boss "Jellyfish King" 152 BPM  the lair's tune for its king: church organ, choir, brass, bigger zaps
//   theme_crab       "Crab Cove"      132 BPM  a sideways tiptoe in A minor: bassoon, flamenco strums, castanets and claves
//   theme_crab_boss  "King Crab"      156 BPM  the cove's tune armoured: brass, power chords, taiko, castanets at full speed
//   jingle_battle_start, jingle_victory, jingle_defeat

const fs = require("fs");
const path = require("path");

const OUT = path.join(__dirname, "..", "..", "audio_sources", "midi");
const PPQ = 480;
const RATE = 48000; // the stems' and the game files' sample rate (loop points are in samples)

// ===== Notes =====
const NAMES = { C: 0, D: 2, E: 4, F: 5, G: 7, A: 9, B: 11 };
const mod12 = n => ((n % 12) + 12) % 12;
function pitch(name) {
    const m = /^([A-G])(#|b)?(-?\d)$/.exec(name);
    if (!m) throw new Error("Bad note " + name);
    return 12 * (Number(m[3]) + 1) + NAMES[m[1]] + (m[2] === "#" ? 1 : m[2] === "b" ? -1 : 0);
}

// A length in beats: "1", ".5", or a fraction "1/3" (12/8 songs count dotted quarters: an eighth is 1/3)
function length(text) {
    const [a, b] = text.split("/");
    return b === undefined ? Number(a) : Number(a) / Number(b);
}

// "C5:.5 F5:.5 r:1 [F3,A3,C4]:4 | ..." -> [{ notes, beats }] (r = rest, | = bar line, checked to be 4 beats)
function parse(text, transpose = 0) {
    for (const bar of text.split("|")) {
        const beats = bar.trim().split(/\s+/).filter(Boolean).reduce((sum, t) => sum + length(t.split(":")[1]), 0);
        if (text.includes("|") && Math.abs(beats - 4) > 0.01) throw new Error("Bar of " + beats + " beats: " + bar);
    }
    return text.trim().split(/\s+/).filter(t => t !== "|").map(token => {
        const [what, beats] = token.split(":");
        if (!(length(beats) > 0)) throw new Error("Bad length in " + token);
        const notes = what === "r" ? [] : what.replace(/[\[\]]/g, "").split(",").map(n => pitch(n) + transpose);
        return { notes, beats: length(beats) };
    });
}

// Deterministic "random" in [0, 1) from any values
function hash(...values) {
    let h = 2166136261;
    for (const c of values.join("|")) { h ^= c.charCodeAt(0); h = Math.imul(h, 16777619); }
    return (h >>> 0) / 4294967296;
}

let ORIGIN = 0; // start of the body copy being written (the two copies must be identical for a seamless loop)

class Part {
    constructor(name, program, { channel = 1, volume = 100, group = "main", pan = 0, gainDb = 0, fx = {} } = {}) {
        Object.assign(this, { name, program, channel, volume, group, pan, gainDb, fx, events: [], cursor: 0 });
    }
    // Plays a line from the cursor. gate: part of each note actually held (staccato < 1)
    play(text, { velocity = 90, gate = 0.95, transpose = 0 } = {}) {
        for (const { notes, beats } of parse(text, transpose)) {
            for (const note of notes) this.hit(this.cursor, note, velocity, beats * gate);
            this.cursor += beats;
        }
        return this;
    }
    rest(beats) { this.cursor += beats; return this; }
    at(beat) { this.cursor = beat; return this; }
    hit(beat, note, velocity = 100, length = 0.25, bend = 0) {
        this.events.push({ time: beat, length, note, velocity: Math.max(1, Math.min(127, Math.round(velocity))), bend });
        return this;
    }
}

class Synth {
    constructor(name, kind, { group = "fx", pan = 0, gainDb = 0, fx = {} } = {}) {
        Object.assign(this, { name, kind, group, pan, gainDb, fx, events: [] });
    }
    add(beat, ...values) { this.events.push([beat, ...values]); return this; }
}

class Song {
    // mix: each group's loudness in dB relative to the lead group (0), applied by render_stems.js
    // kind: "menu", "stage" or "jingle" (the editor lists the menu and stage themes for the stages)
    constructor(name, title, tempo, { introBars = 0, bodyBars = 0, mix = {}, kind = "stage" } = {}) {
        Object.assign(this, { name, title, tempo, intro: introBars * 4, length: bodyBars * 4, mix, kind, parts: [] });
    }
    part(name, program, options) { const p = new Part(name, program, options); this.parts.push(p); return p; }
    drums(name, kit, options) { return this.part(name, kit, { ...options, channel: 10 }); }
    synth(name, kind, options) { const s = new Synth(name, kind, options); this.parts.push(s); return s; }
    // Writes the body twice: the game loops the second copy, so the tails of the first one are already there at the loop start
    body(write) {
        for (let copy = 0; copy < 2; copy++) { ORIGIN = this.intro + copy * this.length; write(ORIGIN); }
        ORIGIN = 0;
    }
}

// ===== Chords =====
const QUALITIES = {
    "": [0, 4, 7], m: [0, 3, 7], 7: [0, 4, 7, 10], maj7: [0, 4, 7, 11], m7: [0, 3, 7, 10], 6: [0, 4, 7, 9], m6: [0, 3, 7, 9],
    add9: [0, 4, 7, 14], madd9: [0, 3, 7, 14], sus4: [0, 5, 7], sus2: [0, 2, 7], "7sus4": [0, 5, 7, 10],
    dim: [0, 3, 6], m7b5: [0, 3, 6, 10], 5: [0, 7],
};
function chord(symbol) {
    const [main, slash] = symbol.split("/");
    const m = /^([A-G])(#|b)?(.*)$/.exec(main);
    if (!m || !(m[3] in QUALITIES)) throw new Error("Bad chord " + symbol);
    const pc = (letter, accidental) => mod12(NAMES[letter] + (accidental === "#" ? 1 : accidental === "b" ? -1 : 0));
    const root = pc(m[1], m[2]);
    const s = slash ? /^([A-G])(#|b)?$/.exec(slash) : null;
    const intervals = QUALITIES[m[3]];
    return { root, bass: s ? pc(s[1], s[2]) : root, intervals, pcs: [...new Set(intervals.map(i => mod12(root + i)))] };
}
const above = (pc, base) => base + mod12(pc - base); // lowest note >= base with that pitch class

// Close voicing near a centre note, moving as little as possible from the previous chord
function voice(symbol, center, previous) {
    const { pcs } = chord(symbol);
    let best = null, bestScore = Infinity;
    for (let inversion = 0; inversion < pcs.length; inversion++) {
        const order = [...pcs.slice(inversion), ...pcs.slice(0, inversion)];
        for (let low = center - 9; low <= center + 2; low++) {
            if (mod12(low) !== order[0]) continue;
            const notes = [low];
            for (let k = 1; k < order.length; k++) notes.push(above(order[k], notes[k - 1] + 1));
            const mean = notes.reduce((a, b) => a + b) / notes.length;
            let score = Math.abs(mean - center) * 0.6 + (notes[notes.length - 1] - notes[0] > 14 ? 3 : 0);
            if (previous) score += notes.reduce((sum, n, i) => sum + Math.abs(n - previous[Math.min(i, previous.length - 1)]), 0) / notes.length;
            if (score < bestScore) { bestScore = score; best = notes; }
        }
    }
    return best;
}

// "F | Dm7 | Gm7 C7 | F" from a beat -> [{ symbol, beat, length }] (two chords in a bar = half each)
function progression(text, start) {
    const out = [];
    let beat = start;
    for (const bar of text.split("|").map(b => b.trim()).filter(Boolean)) {
        const names = bar.split(/\s+/);
        for (const name of names) { out.push({ symbol: name, beat, length: 4 / names.length }); beat += 4 / names.length; }
    }
    return out;
}
const progressionEnd = prog => prog[prog.length - 1].beat + prog[prog.length - 1].length;
const chordAt = (prog, beat) => prog.find(e => beat >= e.beat - 1e-6 && beat < e.beat + e.length - 1e-6) || prog[prog.length - 1];
const chordAfter = (prog, entry) => prog[prog.indexOf(entry) + 1] || prog[0];

// A chord tone: R, 3, 5, 7, 8 (octave), 10, 12, -5 (fifth below), app (a half step under the next chord's bass).
// slash: the bass of a slash chord is the anchor (bass lines)
function tone(entry, token, base, { slash = false, next = null } = {}) {
    const c = chord(entry.symbol);
    const anchor = above(slash ? c.bass : c.root, base);
    const third = c.intervals[1], fifth = c.intervals.find(i => i >= 6 && i <= 8) ?? 7;
    const seventh = c.intervals.find(i => i >= 9 && i <= 11);
    const up = (interval, extra = 0) => above(mod12(c.root + interval), anchor) + extra;
    switch (token) {
        case "R": return anchor;
        case "8": return anchor + 12;
        case "3": return up(third);
        case "10": return up(third, 12);
        case "5": return up(fifth);
        case "12": return up(fifth, 12);
        case "-5": return up(fifth) - 12;
        case "7": return seventh !== undefined ? up(seventh) : anchor + 12;
        case "app": { const target = above(chord(next.symbol).bass, base); return target - 1 >= base ? target - 1 : target + 1; }
    }
    throw new Error("Bad tone " + token);
}

// ===== Accompaniment =====

// Held chords, one per chord of the progression
function sustain(part, prog, { center = 60, velocity = 60, gate = 0.98 } = {}) {
    let previous = null;
    for (const entry of prog) {
        const notes = voice(entry.symbol, center, previous);
        previous = notes;
        notes.forEach(n => part.hit(entry.beat, n, velocity, entry.length * gate));
    }
}

// Rhythmic chords: pattern [[offset, length, velocity], ...] repeated every `cycle` beats.
// strum: beats between two strings (a guitar's strum, low to high)
function comp(part, prog, { pattern, cycle = 4, center = 64, strum = 0 }) {
    const start = prog[0].beat, end = progressionEnd(prog);
    let previous = null;
    for (let c = start; c < end - 1e-6; c += cycle) {
        for (const [offset, length, velocity] of pattern) {
            const t = c + offset;
            if (t >= end - 1e-6) continue;
            const entry = chordAt(prog, t);
            const notes = voice(entry.symbol, center, previous);
            previous = notes;
            notes.forEach((n, i) => part.hit(t + i * strum, n, velocity, Math.min(length, entry.beat + entry.length - t)));
        }
    }
}

// Funk chops: one hit per "x" of a 16-step pattern (per bar), cycling through chord tones (like figure's tokens)
function chops(part, prog, { pattern, tones, base = 52, velocity = 70, length = 0.12 }) {
    const steps = pattern.replace(/\s/g, "");
    const start = prog[0].beat, end = progressionEnd(prog);
    let count = 0;
    for (let t = start, step = 0; t < end - 1e-6; t += 0.25, step++) {
        if (steps[step % steps.length] !== "x") continue;
        const entry = chordAt(prog, t);
        part.hit(t, tone(entry, tones[count % tones.length], base), velocity + (step % 4 === 0 ? 10 : 0), length);
        count++;
    }
}

// Power chords (root, fifth, octave) on every eighth, accented on the beats
function powerChords(part, prog, { base = 40, velocity = 84, gate = 0.8, rate = 0.5 } = {}) {
    const start = prog[0].beat, end = progressionEnd(prog);
    for (let t = start, step = 0; t < end - 1e-6; t += rate, step++) {
        const entry = chordAt(prog, t);
        for (const token of ["R", "5", "8"]) part.hit(t, tone(entry, token, base), velocity + (step % 2 === 0 ? 12 : 0), rate * gate);
    }
}

// A few chord hits at given offsets from a beat
function stab(part, prog, beat, offsets, { center = 64, velocity = 96, length = 0.3 } = {}) {
    for (const offset of offsets) voice(chordAt(prog, beat + offset).symbol, center).forEach(n => part.hit(beat + offset, n, velocity, length));
}

// Bass line: pattern [[offset, length, tone, velocity], ...] per bar
function bassline(part, prog, { pattern, cycle = 4, base = 28 }) {
    const start = prog[0].beat, end = progressionEnd(prog);
    for (let c = start; c < end - 1e-6; c += cycle) {
        for (const [offset, length, token, velocity] of pattern) {
            const t = c + offset;
            if (t >= end - 1e-6) continue;
            const entry = chordAt(prog, t);
            part.hit(t, tone(entry, token, base, { slash: true, next: chordAfter(prog, entry) }), velocity, length);
        }
    }
}

// An ostinato of chord tones (tokens like bassline's), one every `rate` beats
function figure(part, prog, { pattern, rate = 0.5, base = 48, velocity = 80, accent = 10, gate = 0.8 }) {
    const start = prog[0].beat, end = progressionEnd(prog);
    for (let step = 0; start + step * rate < end - 1e-6; step++) {
        const t = start + step * rate;
        const entry = chordAt(prog, t);
        part.hit(t, tone(entry, pattern[step % pattern.length], base), velocity + (step % 2 === 0 ? accent : 0), rate * gate);
    }
}

// Broken chords: pattern = indexes into the voiced chord (past its size = an octave up), one every `rate` beats
function arp(part, prog, { pattern, rate = 0.5, center = 72, velocity = 70, accent = 8, gate = 0.9 }) {
    const start = prog[0].beat, end = progressionEnd(prog);
    let previous = null, entry = null, notes = null;
    for (let step = 0; start + step * rate < end - 1e-6; step++) {
        const t = start + step * rate;
        const current = chordAt(prog, t);
        if (current !== entry) { entry = current; notes = voice(entry.symbol, center, previous); previous = notes; }
        const index = pattern[step % pattern.length];
        part.hit(t, notes[index % notes.length] + 12 * Math.floor(index / notes.length), velocity + (step % 2 === 0 ? accent : 0), rate * gate);
    }
}

// A harp glissando through the notes of a scale (pitch classes)
function gliss(part, start, from, to, beats, scale, velocity = 70, length = 2) {
    const notes = [];
    for (let n = from; from <= to ? n <= to : n >= to; n += from <= to ? 1 : -1) if (scale.includes(mod12(n))) notes.push(n);
    notes.forEach((n, i) => part.hit(start + beats * i / notes.length, n, velocity, length));
}

// ===== Drums (General MIDI channel 10) =====
const KICK = 36, SIDE_STICK = 37, SNARE = 38, CLAP = 39, BRUSH_SWIRL = 40, HAT = 42, LOW_TOM = 45, OPEN_HAT = 46, CRASH = 49, RIDE = 51, TAMBOURINE = 54;
const CLAVES = 75, WOOD_HIGH = 76, WOOD_LOW = 77, GUIRO = 74, CASTANETS = 85;
const BONGO_HIGH = 60, BONGO_LOW = 61, CONGA_MUTE = 62, CONGA_OPEN = 63, CONGA_LOW = 64, SHAKER = 82;

// One character per step (16 per bar, or 12 in 12/8): X x m o g = accent to ghost, . = rest.
// Patterns longer than a bar span several bars.
const LEVELS = { X: 122, x: 100, m: 86, o: 70, g: 48 };
function beat(part, start, barsCount, grid, stepsPerBar = 16) {
    for (const [note, text] of Object.entries(grid)) {
        const steps = text.replace(/[\s|]/g, "");
        for (let s = 0; s < barsCount * stepsPerBar; s++) {
            const velocity = LEVELS[steps[s % steps.length]];
            if (velocity) part.hit(start + s * 4 / stepsPerBar, Number(note), velocity, 0.2);
        }
    }
}
// Steps of a pattern: steps(32, [0, 6, 12], "o") -> "o.....o.....o..."
const steps = (length, positions, level = "o") => Array.from({ length }, (_, i) => positions.includes(i) ? level : ".").join("");

function roll(part, start, beats, note = SNARE, from = 50, to = 110, step = 0.25) {
    const count = Math.round(beats / step);
    for (let i = 0; i < count; i++) part.hit(start + i * step, note, from + (to - from) * i / Math.max(1, count - 1), 0.1);
}
function tomFill(part, start, beats = 2, velocity = 100) {
    const toms = [50, 48, 47, 45, 43, 41];
    const count = beats * 4;
    for (let i = 0; i < count; i++) part.hit(start + i / 4, toms[Math.floor(i * toms.length / count)], velocity - (i % 2) * 14, 0.2);
}

// ===== Bubbles =====
// A cluster of bubbles rising in pitch over `spread` beats (the same cluster in both body copies)
function bubbles(synth, start, { count = 6, spread = 1, low = 450, high = 1400, size = [0.04, 0.1], velocity = 80 } = {}) {
    const local = Math.round((start - ORIGIN) * 1000);
    for (let i = 0; i < count; i++) {
        const r = k => hash(synth.name, local, i, k);
        synth.add(start + spread * (i + r(1) * 0.8) / count, low * Math.pow(high / low, (i + r(2)) / count),
            Math.round(velocity * (0.7 + 0.3 * r(3))), size[0] + (size[1] - size[0]) * r(4), (r(5) - 0.5) * 1.2);
    }
}

// ===== Electric zaps (jellyfish) =====
// A burst of crackles, a buzz and a falling "zzap", synthesized by render_stems.js. Same zaps in both body copies.
function zaps(synth, start, { count = 1, spread = 0, velocity = 90, duration = [0.15, 0.35] } = {}) {
    const local = Math.round((start - ORIGIN) * 1000);
    for (let i = 0; i < count; i++) {
        const r = k => hash(synth.name, local, i, k);
        synth.add(start + (count > 1 ? spread * (i + r(1) * 0.6) / count : 0), Math.round(velocity * (0.75 + 0.25 * r(2))),
            duration[0] + (duration[1] - duration[0]) * r(3), Math.floor(r(4) * 1e6), (r(5) - 0.5) * 1.4);
    }
}

// ===== Writing the files =====
function vlq(value) {
    const bytes = [value & 0x7f];
    while ((value >>= 7) > 0) bytes.unshift((value & 0x7f) | 0x80);
    return bytes;
}
function textEvent(type, text) {
    const data = [...Buffer.from(text, "utf8")];
    return [0xff, type, ...vlq(data.length), ...data];
}
function chunk(type, data) {
    const header = Buffer.alloc(8);
    header.write(type, 0, "ascii");
    header.writeUInt32BE(data.length, 4);
    return Buffer.concat([header, Buffer.from(data)]);
}
function trackChunk(timed) {
    timed.sort((a, b) => a.tick - b.tick || a.order - b.order);
    const data = [];
    let last = 0;
    for (const { tick, bytes } of timed) { data.push(...vlq(tick - last), ...bytes); last = tick; }
    data.push(0, 0xff, 0x2f, 0x00);
    return chunk("MTrk", data);
}

function writeMidi(song, parts, file) {
    const tick = beats => Math.round(beats * PPQ);
    const conductor = [
        { tick: 0, order: 0, bytes: textEvent(0x03, song.title) },
        { tick: 0, order: 0, bytes: [0xff, 0x51, 0x03, ...[16, 8, 0].map(s => (Math.round(60000000 / song.tempo) >> s) & 0xff)] },
        { tick: 0, order: 0, bytes: [0xff, 0x58, 0x04, 4, 2, 24, 8] },
    ];
    if (song.length) {
        conductor.push({ tick: tick(song.intro + song.length), order: 0, bytes: textEvent(0x06, "loopStart") });
        conductor.push({ tick: tick(song.intro + 2 * song.length), order: 0, bytes: textEvent(0x06, "loopEnd") });
    }
    const chunks = [trackChunk(conductor)];
    parts.filter(p => p instanceof Part).forEach((part, index) => {
        const ch = part.channel === 10 ? 9 : index % 9;
        const timed = [
            { tick: 0, order: 0, bytes: textEvent(0x03, part.name) },
            { tick: 0, order: 1, bytes: [0xc0 | ch, part.program] },
            { tick: 0, order: 1, bytes: [0xb0 | ch, 7, part.volume] },
        ];
        for (const e of part.events) {
            timed.push({ tick: tick(e.time), order: 3, bytes: [0x90 | ch, e.note, e.velocity] });
            timed.push({ tick: tick(e.time + Math.max(e.length, 0.05)), order: 2, bytes: [0x80 | ch, e.note, 0] });
        }
        chunks.push(trackChunk(timed));
    });
    const header = Buffer.alloc(6);
    header.writeUInt16BE(1, 0);
    header.writeUInt16BE(chunks.length, 2);
    header.writeUInt16BE(PPQ, 4);
    fs.writeFileSync(file, Buffer.concat([chunk("MThd", header), ...chunks]));
}

// The song's MIDI, then one stem per part (stems/<song>/<nn>_<name>.mid + .json in seconds, slightly humanized)
function writeSong(song) {
    writeMidi(song, song.parts, path.join(OUT, song.name + ".mid"));
    const folder = path.join(OUT, "stems", song.name);
    fs.rmSync(folder, { recursive: true, force: true });
    fs.mkdirSync(folder, { recursive: true });
    fs.writeFileSync(path.join(folder, "mix.json"), JSON.stringify(song.mix, null, 4));
    const spb = 60 / song.tempo;
    // Times go on a sample grid where both body copies are exactly `lengthSamples` apart, so the stems' copies are
    // sample-identical and the game's loop points (in samples) join them seamlessly
    const introSamples = Math.round(song.intro * spb * RATE), lengthSamples = Math.round(song.length * spb * RATE);
    const loopSeconds = song.length ? lengthSamples / RATE : null;
    const seconds = t => {
        if (!song.length || t < song.intro) return Math.round(t * spb * RATE) / RATE;
        const copy = Math.floor((t - song.intro) / song.length + 1e-9);
        return (introSamples + copy * lengthSamples + Math.round((t - song.intro - copy * song.length) * spb * RATE)) / RATE;
    };
    // Position inside the body, so both copies get the same humanization
    const local = t => !song.length || t < song.intro ? t : song.intro + (t - song.intro) % song.length;
    song.parts.forEach((part, index) => {
        const name = String(index + 1).padStart(2, "0") + "_" + part.name.toLowerCase().replace(/[^a-z0-9]+/g, "_").replace(/^_|_$/g, "");
        const common = { name: part.name, group: part.group, pan: part.pan, gainDb: part.gainDb, fx: part.fx, tempo: song.tempo, loopSeconds };
        if (part instanceof Synth) {
            const events = part.events.map(([t, ...values]) => [+seconds(t).toFixed(7), ...values]);
            fs.writeFileSync(path.join(folder, name + ".json"), JSON.stringify({ ...common, synth: part.kind, events }));
            return;
        }
        writeMidi(song, [part], path.join(folder, name + ".mid"));
        const notes = part.events.map(e => {
            // Humanize: a little late (never early, or a note would start before the loop point) and a little louder or softer
            const key = local(e.time).toFixed(3) + ":" + e.note;
            const late = Math.round(hash(part.name, key, "t") * (part.channel === 10 ? 0.004 : 0.008) * RATE) / RATE;
            const velocity = Math.max(1, Math.min(127, e.velocity + Math.round((hash(part.name, key, "v") - 0.5) * 10)));
            const note = [+(seconds(e.time) + late).toFixed(7), +(Math.max(e.length, 0.05) * spb).toFixed(5), e.note, velocity];
            if (e.bend) note.push(e.bend);
            return note;
        });
        fs.writeFileSync(path.join(folder, name + ".json"), JSON.stringify({ ...common, channel: part.channel, program: part.program, volume: part.volume, notes }));
    });
}

const songs = [];
const F_MAJOR = [5, 7, 9, 10, 0, 2, 4], EB_MAJOR = [3, 5, 7, 8, 10, 0, 2], EB_LYDIAN = [3, 5, 7, 9, 10, 0, 2];

// =====================================================================================================
// 1. "Reef Rumble" - default stages. Calypso: steel drums sing the tune, the brass answers in its rests.
//    Intro 4 | A 16 (steel drums) | B 8 (brass calls, steel drums answer, one-drop groove) |
//    C 8 (breakdown: marimba alone with the bubbles) | A' 16 (everyone, flute on top) | turnaround 2
// =====================================================================================================
{
    const song = new Song("theme_default", "Fish Fighters - Reef Rumble", 152, { introBars: 4, bodyBars: 50, mix: { lead: 0, brass: -2, keys: -6, bass: -4, drums: -3, percussion: -7, bubbles: -9 } });
    const steel = song.part("Steel drums", 114, { group: "lead", volume: 112, pan: 0.1, fx: { echo: { beats: 0.75, feedback: 0.28, mix: 0.2 } } });
    const flute = song.part("Flute", 73, { group: "lead", volume: 80, pan: 0.3, fx: { echo: { beats: 0.75, feedback: 0.2, mix: 0.12 } } });
    const kalimba = song.part("Kalimba", 108, { group: "lead", volume: 88, pan: -0.25, fx: { echo: { beats: 0.75, feedback: 0.35, mix: 0.3 } } });
    const brass = song.part("Brass", 61, { group: "brass", volume: 100, pan: -0.15 });
    const ep = song.part("Electric piano", 4, { group: "keys", volume: 90, pan: -0.3, fx: { chorus: { mix: 0.5 } } });
    const marimba = song.part("Marimba", 12, { group: "keys", volume: 94, pan: 0.35, fx: { echo: { beats: 0.75, feedback: 0.25, mix: 0.15 } } });
    const bass = song.part("Bass", 33, { group: "bass", volume: 108 });
    const kit = song.drums("Kit", 0, { group: "drums", volume: 92 });
    const perc = song.drums("Percussion", 0, { group: "percussion", volume: 88, pan: 0.2 });
    const bub = song.synth("Bubbles", "bubbles", { group: "bubbles" });

    const A1 = "F | Dm7 | Bbmaj7 | C7 | F | Dm7 | Gm7 C7 | F";
    const A2 = "F | Dm7 | Bbmaj7 | Bbm6 | F/C | D7 | Gm7 C7 | F";
    const B = "Gm7 | C7 | Fmaj7 | Dm7 | Gm7 | C7 | Am7 D7 | Gm7 C7";
    const C = "Dm | Dm/C | Bbmaj7 | A7 | Dm | Dm/C | Bbmaj7 | C7sus4 C7";
    const MEL_A =
        "C5:.5 F5:.5 A5:1 G5:.5 F5:.5 C6:1 | r:.5 A5:.5 r:.5 G5:1 F5:.5 D5:1 | D5:.5 F5:.5 Bb5:1 A5:.5 G5:.5 F5:1 | r:.5 G5:.5 r:.5 E5:1 C5:.5 Bb4:1 | " +
        "C5:.5 F5:.5 A5:1 G5:.5 F5:.5 C6:1 | D6:1.5 C6:.5 A5:1 F5:1 | G5:.5 A5:.5 Bb5:.5 G5:.5 E5:1 C6:1 | F5:2.5 A4:.5 Bb4:.5 B4:.5 | " +
        "C5:.5 F5:.5 A5:1 G5:.5 F5:.5 C6:1 | r:.5 D6:.5 r:.5 C6:1 A5:.5 F5:1 | Bb5:.5 A5:.5 F5:.5 D5:.5 F5:1 Bb5:1 | Db6:1 C6:.5 Bb5:.5 G5:1 F5:1 | " +
        "C5:.5 F5:.5 A5:1 C6:.5 A5:.5 F5:1 | F#5:.5 A5:.5 D6:1 C6:.5 A5:.5 F#5:1 | G5:.5 Bb5:.5 D6:.5 Bb5:.5 C6:.5 Bb5:.5 G5:.5 E5:.5 | F5:1 C5:.5 F5:1.5 r:1";
    const COUNTER = "A4:2 C5:2 | D5:2 A4:2 | Bb4:2 D5:2 | C5:2 E5:2 | C5:2 A4:2 | F4:2 A4:2 | Bb4:2 G4:2 | A4:4 | " +
        "A4:2 C5:2 | D5:2 F5:2 | D5:2 A4:2 | Db5:2 Bb4:2 | A4:2 C5:2 | C5:2 F#4:2 | D5:2 E5:2 | F5:4";
    const B_CALLS = ["G4:.5 Bb4:.5 D5:1 F5:.5 D5:.5 r:1", "A4:.5 C5:.5 F5:1 E5:.5 C5:.5 r:1", "Bb4:.5 D5:.5 G5:1 F5:.5 D5:.5 r:1"];
    const B_ANSWERS = ["r:.5 E5:.5 G5:.5 Bb5:.5 C6:.5 Bb5:.5 G5:1", "r:.5 F5:.5 A5:.5 C6:.5 D6:.5 C6:.5 A5:1", "r:.5 G5:.5 Bb5:.5 C6:.5 E6:.5 C6:.5 Bb5:1"];
    const B_END = "A5:.5 G5:.5 E5:.5 C5:.5 F#5:.5 A5:.5 C6:.5 D6:.5 | Bb5:1 A5:.5 G5:.5 E5:1 C5:1";
    const MEL_C = "A4:.5 D5:.5 F5:1 E5:.5 D5:.5 A5:1 | r:1 G5:.5 F5:.5 E5:1 D5:1 | F5:.5 Bb5:.5 D6:1 C6:.5 Bb5:.5 F5:1 | E5:1 C#5:.5 E5:.5 A5:2 | " +
        "A4:.5 D5:.5 F5:1 E5:.5 D5:.5 A5:1 | C6:1 Bb5:.5 A5:.5 G5:1 F5:1 | D5:.5 F5:.5 A5:.5 Bb5:.5 D6:2 | C6:2 Bb5:1 E5:1";

    const SKANK = [[0.5, 0.3, 72], [1.5, 0.3, 80], [2.5, 0.3, 72], [3.5, 0.3, 80]];
    const CALYPSO = [[0, 1.4, "R", 104], [1.5, 0.45, "R", 80], [2, 0.9, "5", 94], [3, 0.45, "8", 86], [3.5, 0.45, "5", 80]];
    const WALK = [[0, 0.9, "R", 100], [1, 0.9, "5", 86], [2, 0.9, "8", 92], [3, 0.9, "app", 84]];
    const SOCA = { [KICK]: "x...x...x...x...", [SIDE_STICK]: "....x.......x..g", [HAT]: "..o...o...o...o." };
    const SOCA_FULL = { [KICK]: "x...x...x...x...", [SNARE]: "....x.......x..g", [HAT]: "g.g.g.g.g.g.g.g.", [OPEN_HAT]: "..o...o...o...o." };
    const ONE_DROP = { [KICK]: "........x.......", [SIDE_STICK]: "........x.......", [HAT]: "o.g.o.g.o.g.o.g." };
    const PERC_A = { [SHAKER]: "gogogogogogogogo", [CONGA_MUTE]: "o.......o.......", [CONGA_OPEN]: "......x.......x.", [CONGA_LOW]: "..........x.x..." };
    const PERC_B = { [SHAKER]: "gogogogogogogogo", [BONGO_HIGH]: "..o...o...o.o...", [BONGO_LOW]: "o.......o......." };

    // Intro: bubbles rise, the band starts, the steel drums tease the motif
    const pI = progression("F | F | Bbmaj7 | C7", 0);
    bubbles(bub, 0, { count: 12, spread: 3, low: 350, high: 1500, velocity: 90 });
    comp(ep, pI, { pattern: SKANK, center: 65 });
    bassline(bass, pI, { pattern: CALYPSO });
    beat(perc, 0, 4, PERC_A);
    beat(kit, 8, 1, { [HAT]: "..o...o...o...o." });
    beat(kit, 12, 1, { [KICK]: "x...x...........", [HAT]: "..o...o........." });
    tomFill(kit, 14, 2);
    steel.at(8).play("C5:.5 F5:.5 A5:1 G5:.5 F5:.5 C6:1 | r:.5 G5:.5 r:.5 E5:.5 C5:1 r:1", { velocity: 92 });

    song.body(s => {
        const a = s, b = s + 64, c = s + 96, a2 = s + 128, t = s + 192;
        const pA = progression(A1 + " | " + A2, a), pB = progression(B, b), pC = progression(C, c);
        const pA2 = progression(A1 + " | " + A2, a2), pT = progression("Gm7 | C7", t);

        // A: steel drums tune, skanking electric piano, calypso bass; the brass answers where the tune rests
        steel.at(a).play(MEL_A, { velocity: 100, gate: 0.9 });
        comp(ep, pA, { pattern: SKANK, center: 65 });
        bassline(bass, pA, { pattern: CALYPSO });
        beat(kit, a, 16, SOCA);
        kit.hit(a, CRASH, 100, 1);
        beat(perc, a, 16, PERC_A);
        arp(marimba, progression(A2, a + 32), { pattern: [0, 2, 1, 2, 3, 2, 1, 2], rate: 0.5, center: 70, velocity: 58 });
        for (const bar of [1, 3, 9]) stab(brass, pA, a + bar * 4, [0, 1], { center: 67, velocity: 94 });
        stab(brass, pA, a + 60, [3, 3.5], { center: 67, velocity: 100 });
        bubbles(bub, a + 28, { count: 5, spread: 1 });
        bubbles(bub, a + 60, { count: 7, spread: 1.5 });
        tomFill(kit, a + 62, 2);

        // B: brass calls, steel drums answer, one-drop groove, walking bass, marimba in threes against the beat
        kit.hit(b, CRASH, 90, 1);
        brass.at(b);
        steel.at(b);
        for (let i = 0; i < 3; i++) {
            brass.play(B_CALLS[i], { velocity: 100, gate: 0.85 }).rest(4);
            steel.rest(4).play(B_ANSWERS[i], { velocity: 100, gate: 0.9 });
        }
        brass.at(b + 24).play(B_END, { velocity: 100, gate: 0.85, transpose: -12 });
        steel.at(b + 24).play(B_END, { velocity: 104, gate: 0.9 });
        sustain(ep, pB, { center: 64, velocity: 56 });
        arp(marimba, pB, { pattern: [0, 1, 2, 3, 2, 1], rate: 0.5, center: 72, velocity: 60 });
        bassline(bass, pB, { pattern: WALK });
        beat(kit, b, 8, ONE_DROP);
        beat(perc, b, 8, PERC_B);
        bubbles(bub, b + 14, { count: 6, spread: 1.5 });
        tomFill(kit, b + 30, 2);

        // C: breakdown, the motif in D minor on marimba, kalimba joins, lots of bubbles, the drums build back
        marimba.at(c).play(MEL_C, { velocity: 98, gate: 0.9 });
        kalimba.at(c + 16).play(MEL_C.split("|").slice(4).join("|"), { velocity: 72 });
        sustain(ep, pC, { center: 62, velocity: 48 });
        bassline(bass, pC, { pattern: [[0, 1.9, "R", 86], [2, 1.9, "5", 74]] });
        beat(perc, c, 8, { [SHAKER]: "gogogogogogogogo", [CONGA_MUTE]: "o.......o......." });
        beat(kit, c + 24, 1, { [KICK]: "o...o...o...o..." });
        beat(kit, c + 28, 1, { [KICK]: "x...x...x...x..." });
        roll(kit, c + 28, 4, SNARE, 36, 112);
        for (let bar = 0; bar < 8; bar += 2) bubbles(bub, c + bar * 4 + 2, { count: 6, spread: 2, low: 300, high: 1200 });

        // A': the tune on steel drums and brass, the flute's counter-melody on top
        steel.at(a2).play(MEL_A, { velocity: 104, gate: 0.9 });
        brass.at(a2).play(MEL_A, { velocity: 92, gate: 0.85, transpose: -12 });
        flute.at(a2).play(COUNTER, { velocity: 76, transpose: 12 });
        comp(ep, pA2, { pattern: SKANK, center: 65 });
        arp(marimba, pA2, { pattern: [0, 2, 1, 2, 3, 2, 1, 2], rate: 0.5, center: 70, velocity: 62 });
        bassline(bass, pA2, { pattern: CALYPSO });
        beat(kit, a2, 16, SOCA_FULL);
        kit.hit(a2, CRASH, 110, 1).hit(a2 + 32, CRASH, 100, 1);
        beat(perc, a2, 16, PERC_A);

        // Turnaround: a brass run, the steel drums' chromatic pickup back into the tune
        brass.at(t).play("Bb4:.5 C5:.5 D5:.5 E5:.5 F5:.5 G5:.5 A5:.5 Bb5:.5 | C6:.5 r:.5 C5:.5 r:2.5", { velocity: 104, gate: 0.8, transpose: -12 });
        steel.at(t + 4).play("r:2.5 A4:.5 Bb4:.5 B4:.5", { velocity: 96 });
        comp(ep, pT, { pattern: SKANK, center: 65 });
        bassline(bass, pT, { pattern: WALK });
        beat(kit, t, 1, SOCA);
        tomFill(kit, t + 6, 2);
        beat(perc, t, 2, PERC_A);
        bubbles(bub, t + 4, { count: 10, spread: 3, low: 400, high: 1600 });
    });
    songs.push(song);
}

// =====================================================================================================
// 2. "Kelp Garden" - calm and easy stages. An underwater bossa: the I - II/I lydian float,
//    vibraphone and flute, harp ripples, kalimba echoes, brushes.
//    Intro 2 | A 16 (vibraphone) | B 8 (deeper: falling chords, flute, harp) | A' 16 (flute, vibraphone counter-melody)
// =====================================================================================================
{
    const song = new Song("theme_calm", "Fish Fighters - Kelp Garden", 100, { introBars: 2, bodyBars: 40, mix: { lead: 0, kalimba: -5, harp: -5, keys: -5, pad: -9, bass: -4, drums: -7, bubbles: -9 } });
    const vibes = song.part("Vibraphone", 11, { group: "lead", volume: 104, pan: -0.1, fx: { chorus: { mix: 0.35 }, echo: { beats: 0.75, feedback: 0.3, mix: 0.18 } } });
    const flute = song.part("Flute", 73, { group: "lead", volume: 90, pan: 0.15, fx: { echo: { beats: 0.75, feedback: 0.25, mix: 0.15 } } });
    const kalimba = song.part("Kalimba", 108, { group: "kalimba", volume: 92, pan: 0.4, fx: { echo: { beats: 0.75, feedback: 0.4, mix: 0.3 } } });
    const harp = song.part("Harp", 46, { group: "harp", volume: 94, pan: -0.35 });
    const ep = song.part("Electric piano", 4, { group: "keys", volume: 86, pan: -0.15, fx: { chorus: { mix: 0.55 } } });
    const pad = song.part("Pad", 89, { group: "pad", volume: 74, fx: { lowpass: 2200 } });
    const bass = song.part("Fretless bass", 35, { group: "bass", volume: 104 });
    const kit = song.drums("Brushes", 40, { group: "drums", volume: 86 });
    const bub = song.synth("Bubbles", "bubbles", { group: "bubbles" });

    const A1 = "Ebmaj7 | F/Eb | Ebmaj7 | F/Eb | Cm7 | Abmaj7 | Fm7 | Bb7sus4";
    const A2 = "Ebmaj7 | F/Eb | Gm7 | C7 | Fm7 | Abm6 | Eb/Bb Bb7 | Ebmaj7";
    const B = "Abmaj7 | Gm7 | Fm7 | Ebmaj7 | Dbmaj7 | Cm7 | Fm7 | Bb7";
    const MEL_A = "Bb4:1 Eb5:1 G5:2 | F5:1 Eb5:1 A5:2 | G5:1.5 F5:.5 Eb5:1 Bb4:1 | A4:1 C5:1 F5:2 | " +
        "Eb5:1.5 D5:.5 C5:1 G4:1 | Ab4:1 C5:1 Eb5:1 G5:1 | Ab5:1.5 G5:.5 F5:1 C5:1 | Eb5:3 r:1 | " +
        "Bb4:1 Eb5:1 G5:2 | F5:1 Eb5:1 C6:2 | Bb5:1.5 G5:.5 F5:1 D5:1 | E5:1.5 G5:.5 Bb5:2 | " +
        "Ab5:1 G5:.5 F5:.5 C5:2 | Eb5:1 Cb5:1 Ab4:2 | G4:1 Bb4:1 F5:1 D5:1 | Eb5:4";
    const MEL_B = "C6:.5 Bb5:.5 G5:1 Eb5:1 C5:1 | D6:.5 Bb5:.5 F5:1 D5:1 Bb4:1 | C6:.5 Ab5:.5 Eb5:1 C5:1 Ab4:1 | G5:1 Bb5:1 D6:2 | " +
        "F5:.5 Ab5:.5 C6:1 Db6:2 | Bb5:1 G5:1 Eb5:1 C5:1 | Ab5:.5 C6:.5 Eb6:1 C6:1 Ab5:1 | D6:1 C6:.5 Bb5:.5 Ab5:1 F5:1";
    const COUNTER = "G4:4 | A4:4 | Bb4:4 | C5:4 | Eb5:2 D5:2 | C5:4 | Ab4:4 | Ab4:2 Bb4:2 | " +
        "G4:4 | A4:4 | Bb4:4 | Bb4:4 | Ab4:4 | Cb5:4 | Bb4:2 Ab4:2 | G4:4";
    const BOSSA = [[0, 1.2, 62], [1.5, 0.45, 56], [3, 0.8, 60], [4.5, 0.45, 56], [6, 1.2, 62], [7.5, 0.45, 56]];
    const BOSSA_BASS = [[0, 1.4, "R", 92], [1.5, 0.45, "5", 72], [2, 1.4, "5", 84], [3.5, 0.45, "R", 72]];
    const BRUSH = { [KICK]: "o.......o.......", [SNARE]: "....g.......g...", [HAT]: "g.g.g.g.g.g.g.g.", [SIDE_STICK]: steps(32, [0, 6, 12, 20, 26], "o"), [SHAKER]: "g.g.g.g.g.g.g.g." };
    const BRUSH_LIGHT = { [KICK]: "g.......g.......", [BRUSH_SWIRL]: "o...............", [RIDE]: "g.g.g.g.g.g.g.g." };
    const HARP_UP = [0, 1, 2, 3, 4, 5, 6, 7];
    const answers = start => {
        kalimba.at(start + 14).play("C6:.5 A5:.5 F5:.5 C5:.5", { velocity: 70 });
        kalimba.at(start + 29).play("Ab5:.5 F5:.5 Eb5:.5 Bb4:.5 r:1", { velocity: 70 });
        kalimba.at(start + 46).play("Bb5:.5 G5:.5 E5:.5 C5:.5", { velocity: 70 });
        kalimba.at(start + 62).play("G5:.5 Bb5:.5 D6:.5 G6:.5", { velocity: 66 });
    };

    // Intro: a harp glissando into the water, bubbles, the kalimba shimmering over the lydian chords
    const pI = progression("Ebmaj7 | F/Eb", 0);
    gliss(harp, 0, pitch("Eb3"), pitch("Bb5"), 2, EB_LYDIAN, 66, 3);
    sustain(pad, pI, { center: 60, velocity: 56 });
    bassline(bass, pI, { pattern: [[0, 3.8, "R", 80]] });
    kalimba.at(0).play("G5:.5 Bb5:.5 D6:3 | A5:.5 C6:.5 Eb6:3", { velocity: 74 });
    bubbles(bub, 0.5, { count: 10, spread: 5, low: 300, high: 1300, velocity: 80 });

    song.body(s => {
        const a = s, b = s + 64, a2 = s + 96;
        const pA = progression(A1 + " | " + A2, a), pB = progression(B, b), pA2 = progression(A1 + " | " + A2, a2);

        // A: vibraphone tune, bossa electric piano and bass, brushes; the harp ripples in the second half
        vibes.at(a).play(MEL_A, { velocity: 92 });
        comp(ep, pA, { pattern: BOSSA, cycle: 8, center: 62 });
        bassline(bass, pA, { pattern: BOSSA_BASS });
        sustain(pad, pA, { center: 60, velocity: 46 });
        beat(kit, a, 16, BRUSH);
        answers(a);
        arp(harp, progression(A2, a + 32), { pattern: HARP_UP, rate: 0.5, center: 58, velocity: 56, gate: 2 });
        bubbles(bub, a + 30, { count: 6, spread: 2, low: 350, high: 1100 });
        bubbles(bub, a + 61, { count: 8, spread: 3, low: 350, high: 1300 });

        // B: deeper - falling major 7ths, the flute sings, harp and kalimba sparkle, lighter brushes
        gliss(harp, b - 1, pitch("Bb2"), pitch("G5"), 1, EB_MAJOR, 62, 2);
        flute.at(b).play(MEL_B, { velocity: 90 });
        arp(harp, pB, { pattern: HARP_UP, rate: 0.5, center: 55, velocity: 58, gate: 2 });
        arp(kalimba, pB, { pattern: [4, 2, 3, 1], rate: 1, center: 72, velocity: 54 });
        sustain(ep, pB, { center: 62, velocity: 54 });
        sustain(pad, pB, { center: 64, velocity: 52 });
        bassline(bass, pB, { pattern: [[0, 1.9, "R", 84], [2, 1.9, "5", 72]] });
        beat(kit, b, 8, BRUSH_LIGHT);
        bubbles(bub, b + 12, { count: 7, spread: 3, low: 250, high: 900, size: [0.06, 0.13] });

        // A': the flute takes the tune, the vibraphone plays a counter-melody, the harp keeps rippling
        gliss(harp, a2 - 1, pitch("Eb3"), pitch("G5"), 1, EB_MAJOR, 62, 2);
        flute.at(a2).play(MEL_A, { velocity: 94 });
        vibes.at(a2).play(COUNTER, { velocity: 70 });
        comp(ep, pA2, { pattern: BOSSA, cycle: 8, center: 62 });
        bassline(bass, pA2, { pattern: BOSSA_BASS });
        sustain(pad, pA2, { center: 60, velocity: 48 });
        beat(kit, a2, 16, BRUSH);
        arp(harp, pA2, { pattern: HARP_UP, rate: 0.5, center: 58, velocity: 54, gate: 2 });
        answers(a2);
        gliss(harp, a2 + 62, pitch("Eb3"), pitch("Bb5"), 2, EB_LYDIAN, 60, 3);
        bubbles(bub, a2 + 61, { count: 10, spread: 3, low: 300, high: 1300 });
    });
    songs.push(song);
}

// =====================================================================================================
// 3. "Deep Current" - serious stages. Steady and focused: horns state the motif in D minor over a
//    pizzicato ostinato and a muffled marimba; a clarinet plans the next move; the sonar searches.
//    Intro 4 | A 16 (horns) | B 8 (clarinet, brighter) | C 8 (tension: sonar, timpani) | A' 16 (horns and strings)
// =====================================================================================================
{
    const song = new Song("theme_serious", "Fish Fighters - Deep Current", 138, { introBars: 4, bodyBars: 48, mix: { horns: 0, strings: -2, woodwind: 0, ostinato: -4, bass: -4, drums: -3, pad: -9, fx: -7 } });
    const horns = song.part("French horns", 60, { group: "horns", volume: 112, pan: -0.2 });
    const strings = song.part("Strings", 48, { group: "strings", volume: 98, pan: 0.2 });
    const clarinet = song.part("Clarinet", 71, { group: "woodwind", volume: 98, pan: 0.3, fx: { echo: { beats: 0.75, feedback: 0.25, mix: 0.15 } } });
    const pizz = song.part("Pizzicato", 45, { group: "ostinato", volume: 104, pan: -0.3, fx: { echo: { beats: 0.75, feedback: 0.3, mix: 0.18 } } });
    const marimba = song.part("Marimba (muffled)", 12, { group: "ostinato", volume: 86, pan: 0.35, fx: { lowpass: 2500 } });
    const bass = song.part("Bass", 33, { group: "bass", volume: 104 });
    const kit = song.drums("Kit", 8, { group: "drums", volume: 92 });
    const timpani = song.part("Timpani", 47, { group: "drums", volume: 100 });
    const taiko = song.part("Taiko", 116, { group: "drums", volume: 96 });
    const pad = song.part("Sweep pad", 95, { group: "pad", volume: 80, fx: { lowpass: 1800 } });
    const ping = song.synth("Sonar", "ping", { group: "fx", fx: { echo: { beats: 1.5, feedback: 0.4, mix: 0.4 } } });
    const bub = song.synth("Bubbles", "bubbles", { group: "fx" });

    const A1 = "Dm | Dm | Bbmaj7 | C | Dm | Dm | Gm | A7";
    const A2 = "Dm | Dm | Bbmaj7 | C | Gm | Bb | Asus4 | A7";
    const B = "Bbmaj7 | C | Am7 | Dm | Gm7 | C | F | A7";
    const C = "Dm | Eb/D | Dm | Eb/D | Bb | C | Gm/D | A7";
    const MEL_A = "A4:1 D5:1 F5:2 | E5:1 D5:1 A5:2 | Bb5:2 A5:1 F5:1 | G5:2 E5:1 C5:1 | A4:1 D5:1 F5:2 | E5:1 F5:1 A5:2 | Bb5:1.5 A5:.5 G5:1 D5:1 | C#5:1 E5:1 A5:1 G5:1 | " +
        "A4:1 D5:1 F5:2 | E5:1 D5:1 A5:2 | D6:1.5 C6:.5 Bb5:1 F5:1 | E5:1.5 F5:.5 G5:1 C6:1 | Bb5:1 A5:1 G5:1 D5:1 | F5:1 D5:1 Bb4:2 | D5:2 E5:1 D5:1 | C#5:2 E5:1 A4:1";
    const MEL_B = "D5:.5 F5:.5 A5:.5 F5:.5 Bb5:1 A5:1 | G5:.5 E5:.5 C5:.5 E5:.5 G5:2 | C6:.5 A5:.5 E5:.5 G5:.5 A5:1 E5:1 | F5:.5 E5:.5 D5:.5 A4:.5 D5:2 | " +
        "Bb5:.5 A5:.5 G5:.5 D5:.5 F5:1 G5:1 | E5:.5 G5:.5 C6:.5 G5:.5 E5:1 C5:1 | A5:.5 G5:.5 F5:.5 C5:.5 F5:2 | E5:.5 G5:.5 C#6:.5 A5:.5 E5:1 C#5:1";
    const C_STRINGS = "A4:4 | Bb4:4 | A4:4 | G4:2 Bb4:2 | D5:4 | E5:4 | D5:2 Bb4:2 | E5:2 C#5:2";
    const C_HORNS = "A3:1 D4:1 F4:2 | r:4 | A3:1 D4:1 F4:2 | r:4 | Bb3:1 D4:1 F4:2 | r:4 | G3:1 Bb3:1 D4:2 | C#4:1 E4:1 A4:2";
    const OSTINATO = ["R", "5", "8", "5", "R", "5", "10", "5"];
    const PULSE = [[0, 0.45, "R", 96], [0.5, 0.45, "R", 70], [1, 0.45, "R", 84], [1.5, 0.45, "8", 76], [2, 0.45, "R", 92], [2.5, 0.45, "R", 70], [3, 0.45, "5", 84], [3.5, 0.45, "8", 76]];
    const WALK = [[0, 0.9, "R", 96], [1, 0.9, "5", 82], [2, 0.9, "8", 88], [3, 0.9, "app", 80]];
    const HALF = { [KICK]: "x.....x...x.....", [SNARE]: "........x.......", [HAT]: "o.g.o.g.o.g.o.g." };
    const HALF_FULL = { [KICK]: "x.....x...x...g.", [SNARE]: "........x.....g.", [HAT]: "m.g.m.g.m.g.m.g.", [OPEN_HAT]: "..............o." };
    const KIT_B = { [KICK]: "x.......x.......", [SIDE_STICK]: "....x.......x...", [HAT]: "o.g.o.g.o.g.o.g." };
    const timpaniOn = (prog, start, bars, every = 2) => {
        for (let bar = 0; bar < bars; bar += every) timpani.hit(start + bar * 4, above(chord(chordAt(prog, start + bar * 4).symbol).root, 33), 104, 1.5);
    };

    // Intro: the sonar, the ostinato, the bass pedal; the horns announce the motif
    const pI = progression("Dm | Dm | Dm | A7", 0);
    figure(pizz, pI, { pattern: OSTINATO, rate: 0.5, base: 45, velocity: 84 });
    bassline(bass, progression("Dm | Dm | A7", 4), { pattern: PULSE });
    sustain(pad, pI, { center: 57, velocity: 60 });
    ping.add(0, 1180, 104).add(8, 1180, 92);
    taiko.hit(0, 45, 112, 1).hit(8, 45, 112, 1);
    horns.at(8).play("A3:1 D4:1 F4:2 | E4:1 C#4:1 A3:2", { velocity: 98 });
    roll(timpani, 12, 4, 33, 50, 112);
    beat(kit, 8, 2, { [HAT]: "o.g.o.g.o.g.o.g." });
    bubbles(bub, 2, { count: 6, spread: 2, low: 250, high: 900 });

    song.body(s => {
        const a = s, b = s + 64, c = s + 96, a2 = s + 128;
        const pA = progression(A1 + " | " + A2, a), pB = progression(B, b), pC = progression(C, c), pA2 = progression(A1 + " | " + A2, a2);

        // A: horns over the ostinato
        horns.at(a).play(MEL_A, { velocity: 100, transpose: -12 });
        figure(pizz, pA, { pattern: OSTINATO, rate: 0.5, base: 45, velocity: 86 });
        arp(marimba, pA, { pattern: [0, 1, 2, 3, 2, 1], rate: 0.25, center: 69, velocity: 50, gate: 0.8 });
        bassline(bass, pA, { pattern: PULSE });
        sustain(pad, pA, { center: 57, velocity: 50 });
        beat(kit, a, 16, HALF);
        kit.hit(a, CRASH, 100, 1);
        for (let bar = 0; bar < 16; bar += 4) taiko.hit(a + bar * 4, 45, 112, 1);
        timpaniOn(pA, a, 16);
        tomFill(kit, a + 62, 2);

        // B: brighter, the clarinet plans ahead, horns hold the chords, walking bass
        clarinet.at(b).play(MEL_B, { velocity: 94, gate: 0.7 });
        sustain(horns, pB, { center: 55, velocity: 62 });
        figure(pizz, pB, { pattern: ["R", "5", "8", "5"], rate: 0.5, base: 45, velocity: 72 });
        bassline(bass, pB, { pattern: WALK });
        beat(kit, b, 8, KIT_B);
        bubbles(bub, b + 28, { count: 6, spread: 2, low: 300, high: 1000 });

        // C: tension - a D pedal under E flat, the sonar searching, strings and horn fragments, the drums build
        strings.at(c).play(C_STRINGS, { velocity: 84, gate: 1 });
        horns.at(c).play(C_HORNS, { velocity: 92 });
        sustain(pad, pC, { center: 57, velocity: 62 });
        bassline(bass, progression("Dm | Eb/D | Dm | Eb/D", c), { pattern: [[0, 3.9, "R", 84]] });
        bassline(bass, progression("Bb | C | Gm/D | A7", c + 16), { pattern: PULSE });
        [0, 8, 16].forEach(t => ping.add(c + t, 1180, 96));
        for (let bar = 4; bar < 8; bar++) [0, 1.5, 3].forEach(t => taiko.hit(c + bar * 4 + t, 45, 100 + (bar - 4) * 4, 1));
        roll(timpani, c + 28, 4, 33, 50, 116);
        beat(kit, c + 16, 2, { [HAT]: "o.g.o.g.o.g.o.g." });
        beat(kit, c + 24, 2, { [KICK]: "x...x...x...x...", [HAT]: "o.o.o.o.o.o.o.o." });
        roll(kit, c + 28, 4, SNARE, 40, 110);
        bubbles(bub, c + 4, { count: 7, spread: 3, low: 220, high: 800, size: [0.06, 0.14] });
        bubbles(bub, c + 20, { count: 7, spread: 3, low: 220, high: 800, size: [0.06, 0.14] });

        // A': horns and strings together, everything else back
        horns.at(a2).play(MEL_A, { velocity: 106, transpose: -12 });
        strings.at(a2).play(MEL_A, { velocity: 92 });
        figure(pizz, pA2, { pattern: OSTINATO, rate: 0.5, base: 45, velocity: 88 });
        arp(marimba, pA2, { pattern: [0, 1, 2, 3, 2, 1], rate: 0.25, center: 69, velocity: 54, gate: 0.8 });
        bassline(bass, pA2, { pattern: PULSE });
        sustain(pad, pA2, { center: 57, velocity: 50 });
        beat(kit, a2, 16, HALF_FULL);
        kit.hit(a2, CRASH, 112, 1).hit(a2 + 32, CRASH, 100, 1);
        for (let bar = 0; bar < 16; bar += 4) taiko.hit(a2 + bar * 4, 45, 116, 1);
        timpaniOn(pA2, a2, 16, 1);
        tomFill(kit, a2 + 62, 2);
    });
    songs.push(song);
}

// =====================================================================================================
// 4. "Pressure Alarm" - stressful stages. Fast and driving in C minor: a pulsing bass, a repeated-note
//    string ostinato, the motif on brass, a frantic steel drum middle, the sonar alarm in the breakdown.
//    Intro 2 | A 16 (brass) | B 8 (steel drums, half time) | C 8 (alarm, rising) | A' 16 (brass, steel drums, xylophone)
// =====================================================================================================
{
    const song = new Song("theme_stressful", "Fish Fighters - Pressure Alarm", 172, { introBars: 2, bodyBars: 48, mix: { brass: 0, lead: -1, strings: -4, bass: -4, drums: -3, fx: -7 } });
    const brass = song.part("Brass", 61, { group: "brass", volume: 110, pan: -0.1 });
    const steel = song.part("Steel drums", 114, { group: "lead", volume: 108, pan: 0.2, fx: { echo: { beats: 0.5, feedback: 0.25, mix: 0.16 } } });
    const xylo = song.part("Xylophone", 13, { group: "lead", volume: 88, pan: 0.3 });
    const strings = song.part("Strings (ostinato)", 48, { group: "strings", volume: 96, pan: -0.3 });
    const slow = song.part("Strings (long)", 49, { group: "strings", volume: 92, pan: 0.15 });
    const bass = song.part("Synth bass", 38, { group: "bass", volume: 98 });
    const kit = song.drums("Kit", 16, { group: "drums", volume: 94 });
    const ping = song.synth("Alarm sonar", "ping", { group: "fx", fx: { echo: { beats: 1, feedback: 0.35, mix: 0.35 } } });
    const bub = song.synth("Bubbles", "bubbles", { group: "fx" });

    const A1 = "Cm | Db/C | Cm | Db/C | Ab | Bb | Gsus4 | G7";
    const A2 = "Cm | Db/C | Cm | Db/C | Fm | Ab | Dm7b5 G7 | Cm";
    const B = "Abmaj7 | Bb | Gm7 | Cm | Fm7 | Bb | Ebmaj7 | G7";
    const C = "Cm | Cm | Db | Db | Ddim | Ddim | Gsus4 | G7";
    const MOTIF_4 = "G4:.5 C5:.5 Eb5:1 D5:.5 C5:.5 G5:1 | Ab5:1.5 G5:.5 F5:1 Db5:1 | G4:.5 C5:.5 Eb5:1 D5:.5 C5:.5 C6:1 | Db6:1 C6:.5 Ab5:.5 F5:1 Db5:1";
    const MEL_A = MOTIF_4 + " | Eb5:.5 Ab5:.5 C6:1 Bb5:.5 Ab5:.5 Eb5:1 | D5:.5 F5:.5 Bb5:1 Ab5:.5 F5:.5 D5:1 | C5:1 D5:1 G5:1 C6:1 | B5:1 G5:.5 F5:.5 D5:1 B4:1 | " +
        MOTIF_4 + " | C6:.5 Ab5:.5 F5:1 G5:.5 Ab5:.5 C6:1 | Eb6:1 C6:.5 Ab5:.5 Eb5:2 | Ab5:.5 F5:.5 D5:.5 F5:.5 G5:.5 B5:.5 D6:.5 B5:.5 | C6:2 r:2";
    const MEL_B = "C5:.5 Eb5:.5 G5:.5 Eb5:.5 Ab5:1 G5:1 | F5:.5 D5:.5 Bb4:.5 D5:.5 F5:2 | Bb5:.5 G5:.5 D5:.5 F5:.5 G5:1 D5:1 | Eb5:.5 G5:.5 C6:.5 G5:.5 Eb5:2 | " +
        "Ab5:.5 F5:.5 C5:.5 Eb5:.5 F5:1 Ab5:1 | Bb5:.5 F5:.5 D5:.5 F5:.5 Bb5:2 | G5:.5 Bb5:.5 D6:.5 Bb5:.5 G5:1 Eb5:1 | D6:.5 B5:.5 G5:.5 F5:.5 D5:.5 B4:.5 G4:1";
    const C_STRINGS = "C5:4 | D5:4 | Db5:4 | Eb5:4 | D5:4 | F5:4 | G5:4 | B5:4";
    const C_BRASS = "G3:.5 C4:.5 Eb4:1 r:2 | r:4 | Ab3:.5 Db4:.5 F4:1 r:2 | r:4 | Ab3:.5 D4:.5 F4:1 r:2 | r:4 | G3:.5 C4:.5 D4:1 G4:1 r:1 | B3:.5 D4:.5 F4:.5 G4:.5 B4:2";
    const OSTINATO = ["R", "8", "R", "5", "R", "8", "R", "10"];
    const PULSE8 = [[0, 0.45, "R", 100], [0.5, 0.45, "8", 78], [1, 0.45, "R", 92], [1.5, 0.45, "8", 78], [2, 0.45, "R", 96], [2.5, 0.45, "8", 78], [3, 0.45, "R", 92], [3.5, 0.45, "8", 84]];
    const DRIVE = { [KICK]: "x.......x.x.....", [SNARE]: "....x.......x...", [HAT]: "gogogogogogogogo" };
    const DRIVE_FULL = { [KICK]: "x.......x.x...g.", [SNARE]: "....x.......x..g", [HAT]: "g.g.g.g.g.g.g.g.", [OPEN_HAT]: "..o...o...o...o." };
    const HALF = { [KICK]: "x.....x.........", [SNARE]: "........x.......", [HAT]: "o.o.o.o.o.o.o.o." };

    // Intro: the alarm, the bass pulse, the strings join, a snare roll
    ping.add(0, 1480, 110).add(4, 1480, 100);
    bassline(bass, progression("Cm | Cm", 0), { pattern: PULSE8, base: 31 });
    figure(strings, progression("Cm", 4), { pattern: OSTINATO, rate: 0.5, base: 48, velocity: 80, gate: 0.5 });
    beat(kit, 0, 1, { [HAT]: "o.o.o.o.o.o.o.o." });
    roll(kit, 4, 4, SNARE, 40, 115);

    song.body(s => {
        const a = s, b = s + 64, c = s + 96, a2 = s + 128;
        const pA = progression(A1 + " | " + A2, a), pB = progression(B, b), pC = progression(C, c), pA2 = progression(A1 + " | " + A2, a2);

        // A: the motif on brass over the pulse
        brass.at(a).play(MEL_A, { velocity: 104, gate: 0.85, transpose: -12 });
        stab(brass, pA, a + 60, [2.5, 3.5], { center: 60, velocity: 104 });
        figure(strings, pA, { pattern: OSTINATO, rate: 0.5, base: 48, velocity: 82, gate: 0.5 });
        bassline(bass, pA, { pattern: PULSE8, base: 31 });
        beat(kit, a, 16, DRIVE);
        kit.hit(a, CRASH, 106, 1).hit(a + 32, CRASH, 100, 1);
        ping.add(a, 1480, 84).add(a + 32, 1480, 80);
        tomFill(kit, a + 62, 2);

        // B: half time, frantic steel drums, brass stabs, long strings
        steel.at(b).play(MEL_B, { velocity: 104, gate: 0.9 });
        comp(brass, pB, { pattern: [[0, 0.3, 94], [1.5, 0.3, 88], [3, 0.3, 90]], center: 62 });
        sustain(slow, pB, { center: 64, velocity: 70 });
        bassline(bass, pB, { pattern: PULSE8, base: 31 });
        beat(kit, b, 8, HALF);
        bubbles(bub, b + 4, { count: 8, spread: 2, low: 400, high: 1500 });
        tomFill(kit, b + 30, 2);

        // C: the alarm every bar, rising strings, brass fragments climbing, the drums speed up
        for (let bar = 0; bar < 8; bar++) ping.add(c + bar * 4, 1480, 88 + bar * 3);
        slow.at(c).play(C_STRINGS, { velocity: 76, gate: 1 });
        brass.at(c).play(C_BRASS, { velocity: 98 });
        bassline(bass, progression("Cm | Cm | Db | Db", c), { pattern: [[0, 0.9, "R", 96], [1, 0.9, "R", 80], [2, 0.9, "R", 90], [3, 0.9, "R", 80]], base: 31 });
        bassline(bass, progression("Ddim | Ddim | Gsus4 | G7", c + 16), { pattern: PULSE8, base: 31 });
        beat(kit, c, 4, { [KICK]: "x...x...x...x...", [HAT]: "....o.......o..." });
        beat(kit, c + 16, 2, { [KICK]: "x.x.x.x.x.x.x.x." });
        roll(kit, c + 24, 8, SNARE, 36, 118);
        bubbles(bub, c + 28, { count: 14, spread: 4, low: 300, high: 1800 });

        // A': brass, steel drums and xylophone on the tune
        brass.at(a2).play(MEL_A, { velocity: 108, gate: 0.85, transpose: -12 });
        steel.at(a2).play(MEL_A, { velocity: 98, gate: 0.9 });
        xylo.at(a2).play(MEL_A, { velocity: 80, gate: 0.6 });
        stab(brass, pA2, a2 + 60, [2.5, 3.5], { center: 60, velocity: 108 });
        figure(strings, pA2, { pattern: OSTINATO, rate: 0.5, base: 48, velocity: 86, gate: 0.5 });
        bassline(bass, pA2, { pattern: PULSE8, base: 31 });
        beat(kit, a2, 16, DRIVE_FULL);
        kit.hit(a2, CRASH, 112, 1).hit(a2 + 32, CRASH, 104, 1);
        tomFill(kit, a2 + 62, 2);
    });
    songs.push(song);
}

// =====================================================================================================
// 5. "Leviathan" - boss stages. The motif in D minor on brass, a 3-3-2 trombone and tuba riff, string
//    sixteenths, choir, timpani and taiko, the leviathan's sliding voice and the sonar in the dark.
//    Intro 4 | A 16 (brass) | B 8 (half time, choir) | C 8 (the leviathan lurks) | A' 16 (everyone)
// =====================================================================================================
{
    const song = new Song("theme_boss", "Fish Fighters - Leviathan", 160, { introBars: 4, bodyBars: 48, mix: { brass: 0, lowbrass: -3, choir: -4, strings: -5, bells: -6, bass: -4, drums: -2, fx: -7 } });
    const brass = song.part("Brass", 61, { group: "brass", volume: 112, pan: -0.1 });
    const trombone = song.part("Trombones", 57, { group: "lowbrass", volume: 104, pan: 0.15 });
    const tuba = song.part("Tuba", 58, { group: "lowbrass", volume: 100 });
    const choir = song.part("Choir", 52, { group: "choir", volume: 96 });
    const whale = song.part("Leviathan voice", 91, { group: "choir", volume: 104, fx: { lowpass: 1400, echo: { beats: 1.5, feedback: 0.4, mix: 0.35 } } });
    const strings = song.part("Strings", 48, { group: "strings", volume: 94, pan: 0.3 });
    const bells = song.part("Tubular bells", 14, { group: "bells", volume: 90, pan: -0.25 });
    const bass = song.part("Synth bass", 38, { group: "bass", volume: 98 });
    const kit = song.drums("Kit", 16, { group: "drums", volume: 96 });
    const timpani = song.part("Timpani", 47, { group: "drums", volume: 104 });
    const taiko = song.part("Taiko", 116, { group: "drums", volume: 98 });
    const ping = song.synth("Sonar", "ping", { group: "fx", fx: { echo: { beats: 1.5, feedback: 0.45, mix: 0.4 } } });
    const bub = song.synth("Bubbles", "bubbles", { group: "fx" });

    const A1 = "Dm | Dm | Bb | C | Dm | Dm | Eb | A7";
    const A2 = "Dm | Dm | Bb | C | Gm | Bb | Eb | A7";
    const B = "Bb | C | Dm | Dm | Bb | C | Eb | A7";
    const C = "Dm | Eb/D | Dm | Eb/D | Dm | Eb/D | Gm/D | A7";
    const MOTIF = "A4:.5 D5:.5 F5:1 E5:.5 D5:.5 A5:1";
    const MEL_A = MOTIF + " | Bb5:1 A5:1 F5:1 D5:1 | D5:.5 F5:.5 Bb5:1 A5:.5 G5:.5 F5:1 | E5:1.5 F5:.5 G5:1 C5:1 | " +
        MOTIF + " | D6:1.5 C6:.5 A5:1 F5:1 | G5:.5 Bb5:.5 Eb6:1 D6:.5 Bb5:.5 G5:1 | A5:1 C#6:1 E6:1 A5:1 | " +
        MOTIF + " | Bb5:1 A5:1 F5:1 D5:1 | F5:.5 Bb5:.5 D6:1 C6:.5 Bb5:.5 F5:1 | G5:.5 C6:.5 E6:1 D6:.5 C6:.5 G5:1 | " +
        "Bb5:1.5 A5:.5 G5:1 D5:1 | F5:1 D5:1 Bb4:1 D5:1 | Eb5:.5 G5:.5 Bb5:1 Eb6:2 | E6:1 C#6:1 A5:1 E5:1";
    const MEL_B = "F5:2 D5:1 F5:1 | G5:2 E5:1 G5:1 | A5:3 F5:.5 A5:.5 | D6:2 C6:1 A5:1 | Bb5:2 F5:1 Bb5:1 | C6:2 G5:1 C6:1 | Bb5:2 G5:1 Eb5:1 | C#6:2 E6:1 A5:1";
    const C_LOW = "A2:1 D3:1 F3:2 | r:4 | A2:1 D3:1 F3:1 Eb3:1 | r:4 | A2:1 D3:1 F3:2 | r:4 | G2:1 Bb2:1 D3:2 | C#3:1 E3:1 A3:2";
    const RIFF = [[0, 1.2, 104], [1.5, 1.2, 100], [3, 0.8, 104]];   // 3 + 3 + 2 eighths
    const DRIVE8 = [[0, 0.45, "R", 100], [0.5, 0.45, "R", 80], [1, 0.45, "8", 86], [1.5, 0.45, "R", 96], [2, 0.45, "R", 80], [2.5, 0.45, "8", 86], [3, 0.45, "R", 96], [3.5, 0.45, "8", 84]];
    const KIT_A = { [KICK]: "x...x...x...x...", [SNARE]: "....x.......x...", [HAT]: "m.o.m.o.m.o.m.o." };
    const KIT_A_FULL = { [KICK]: "x...x...x...x.x.", [SNARE]: "....x.......x..g", [HAT]: "m.o.m.o.m.o.m.o.", [OPEN_HAT]: "..............o." };
    const KIT_HALF = { [KICK]: "x.....x.........", [SNARE]: "........x.......", [RIDE]: "o.o.o.o.o.o.o.o." };
    const timpaniOn = (prog, start, bars, every = 2) => {
        for (let bar = 0; bar < bars; bar += every) timpani.hit(start + bar * 4, above(chord(chordAt(prog, start + bar * 4).symbol).root, 33), 108, 1.5);
    };
    const riff = (prog, velocityShift = 0) => {
        comp(trombone, prog, { pattern: RIFF.map(([o, l, v]) => [o, l, v + velocityShift]), center: 50 });
        bassline(tuba, prog, { pattern: RIFF.map(([o, l, v]) => [o, l, "R", v - 6 + velocityShift]), base: 31 });
    };

    // Intro: the sonar in the dark, the leviathan's voice, timpani and taiko, the trombones state the motif
    const pI = progression("Dm | Eb/D | Dm | A7", 0);
    ping.add(0, 980, 110);
    whale.hit(0, pitch("A3"), 100, 6, -5).hit(8, pitch("D4"), 94, 6, -7);
    sustain(choir, pI, { center: 55, velocity: 56 });
    bassline(bass, pI, { pattern: [[0, 3.9, "R", 88]], base: 26 });
    roll(timpani, 4, 4, 38, 40, 100);
    [0, 4, 8, 10, 12, 13.5].forEach(t => taiko.hit(t, 45, 110, 1));
    trombone.at(8).play("A2:.5 D3:.5 F3:1 E3:.5 D3:.5 A3:1 | C#3:1 E3:1 A3:1 r:1", { velocity: 104 });
    tuba.at(8).play("D2:2 r:2 | A1:2 r:2", { velocity: 100 });
    beat(kit, 12, 1, { [KICK]: "x.x.x.x.x.x.x.x." });
    tomFill(kit, 14, 2);
    bubbles(bub, 1, { count: 14, spread: 6, low: 200, high: 900, size: [0.06, 0.16] });

    song.body(s => {
        const a = s, b = s + 64, c = s + 96, a2 = s + 128;
        const pA = progression(A1 + " | " + A2, a), pB = progression(B, b), pC = progression(C, c), pA2 = progression(A1 + " | " + A2, a2);

        // A: brass over the riff and the string sixteenths
        brass.at(a).play(MEL_A, { velocity: 106, gate: 0.85, transpose: -12 });
        riff(pA);
        bassline(bass, pA, { pattern: DRIVE8, base: 26 });
        figure(strings, pA, { pattern: ["R", "5", "8", "5"], rate: 0.25, base: 62, velocity: 62, gate: 0.7 });
        sustain(choir, progression(A2, a + 32), { center: 62, velocity: 54 });
        beat(kit, a, 16, KIT_A);
        for (let bar = 0; bar < 16; bar += 4) kit.hit(a + bar * 4, CRASH, 104, 1);
        timpaniOn(pA, a, 16);
        tomFill(kit, a + 62, 2);

        // B: half time, the choir sings, bells on the downbeats
        choir.at(b).play(MEL_B, { velocity: 96, transpose: -12, gate: 1 });
        strings.at(b).play(MEL_B, { velocity: 86, gate: 1 });
        sustain(brass, pB, { center: 60, velocity: 72 });
        for (let bar = 0; bar < 8; bar++) bells.hit(b + bar * 4, above(chord(chordAt(pB, b + bar * 4).symbol).root, 62), 92, 3);
        comp(trombone, pB, { pattern: [[0, 3.8, 84]], center: 50 });
        bassline(tuba, pB, { pattern: [[0, 3.8, "R", 90]], base: 31 });
        bassline(bass, pB, { pattern: [[0, 1.9, "R", 100], [2, 0.45, "R", 80], [2.5, 0.45, "8", 80], [3, 0.45, "R", 84], [3.5, 0.45, "8", 84]], base: 26 });
        beat(kit, b, 8, KIT_HALF);
        kit.hit(b, CRASH, 100, 1);
        tomFill(kit, b + 30, 2);

        // C: the leviathan lurks - its voice, the sonar, a D pedal, taiko, then everything builds
        whale.hit(c, pitch("A3"), 100, 6, -5).hit(c + 16, pitch("D4"), 94, 6, -7);
        [0, 8, 16, 24].forEach(t => ping.add(c + t, 980, 96));
        trombone.at(c).play(C_LOW, { velocity: 100 });
        tuba.at(c).play(C_LOW, { velocity: 92, transpose: -12 });
        bassline(bass, pC, { pattern: [[0, 3.9, "R", 90]], base: 26 });
        sustain(choir, pC, { center: 57, velocity: 62 });
        strings.at(c + 16).play("D5:4 | Eb5:4 | D5:4 | C#5:4", { velocity: 80, gate: 1 });
        for (let bar = 0; bar < 8; bar++) [0, 1.5, 3].forEach(t => taiko.hit(c + bar * 4 + t, 45, 96 + bar * 3, 1));
        roll(timpani, c + 24, 8, 38, 40, 118);
        beat(kit, c + 16, 3, { [KICK]: "x.....x.....x..." });
        roll(kit, c + 28, 4, SNARE, 40, 118);
        bubbles(bub, c + 2, { count: 12, spread: 6, low: 180, high: 800, size: [0.08, 0.18] });
        bubbles(bub, c + 26, { count: 14, spread: 5, low: 250, high: 1400 });

        // A': everyone - brass and strings on the tune, choir, bells, the riff, full drums
        brass.at(a2).play(MEL_A, { velocity: 110, gate: 0.85, transpose: -12 });
        strings.at(a2).play(MEL_A, { velocity: 90, gate: 0.9 });
        riff(pA2, 6);
        bassline(bass, pA2, { pattern: DRIVE8, base: 26 });
        sustain(choir, pA2, { center: 62, velocity: 60 });
        for (let bar = 0; bar < 16; bar += 2) bells.hit(a2 + bar * 4, above(chord(chordAt(pA2, a2 + bar * 4).symbol).root, 62), 86, 3);
        beat(kit, a2, 16, KIT_A_FULL);
        for (let bar = 0; bar < 16; bar += 4) kit.hit(a2 + bar * 4, CRASH, 112, 1);
        for (let bar = 0; bar < 16; bar++) RIFF.forEach(([t]) => taiko.hit(a2 + bar * 4 + t, 45, bar % 4 === 0 ? 112 : 92, 1));
        timpaniOn(pA2, a2, 16, 1);
        tomFill(kit, a2 + 62, 2);
    });
    songs.push(song);
}

// =====================================================================================================
// 6. "Fish Fighters!" - the title screen, made to make you press start. Ska-punk energy: brass and steel
//    drums shout the motif over power chords, a skanking guitar, organ stabs and a punk beat.
//    Intro 4 | A 16 | B 8 (the build) | A' 8 (the big finish) | turnaround 2
// =====================================================================================================
{
    const song = new Song("theme_title", "Fish Fighters - Fish Fighters!", 168, { kind: "menu", introBars: 4, bodyBars: 34,
        mix: { brass: 0, lead: -1, guitar: -6, keys: -8, bass: -4, drums: -2, fx: -9 } });
    const brass = song.part("Brass", 61, { group: "brass", volume: 112, pan: -0.1 });
    const steel = song.part("Steel drums", 114, { group: "lead", volume: 108, pan: 0.15, fx: { echo: { beats: 0.75, feedback: 0.2, mix: 0.12 } } });
    const glock = song.part("Glockenspiel", 9, { group: "lead", volume: 78, pan: 0.3 });
    const power = song.part("Power chords", 29, { group: "guitar", volume: 90, pan: -0.35 });
    const skank = song.part("Skank guitar", 27, { group: "guitar", volume: 86, pan: 0.4 });
    const organ = song.part("Organ", 16, { group: "keys", volume: 84, pan: 0.2 });
    const bass = song.part("Bass", 33, { group: "bass", volume: 108 });
    const kit = song.drums("Kit", 16, { group: "drums", volume: 96 });
    const bub = song.synth("Bubbles", "bubbles", { group: "fx" });

    const A1 = "F | F | Bb | C | Dm | Bb | Gm | C";
    const A2 = "F | F | Bb | C | Dm | Bb | Gm C | F";
    const B = "Bb | C | Dm | Bb | Gm | A7 | Bb | C";
    const HOOK = "C5:.5 F5:.5 A5:1 G5:.5 F5:.5 C6:1 | A5:.5 C6:.5 r:.5 A5:.5 G5:1 F5:1 | D5:.5 F5:.5 Bb5:1 A5:.5 G5:.5 F5:1 | E5:.5 G5:.5 C6:1 E6:1 r:1";
    const MEL_A1 = HOOK + " | D6:.5 C6:.5 A5:1 F5:.5 A5:.5 D6:1 | D6:.5 C6:.5 Bb5:1 F5:.5 G5:.5 A5:1 | Bb5:.5 A5:.5 G5:.5 F5:.5 G5:1 D5:1 | E5:.5 F5:.5 G5:.5 A5:.5 Bb5:.5 C6:.5 r:1";
    const MEL_A2 = HOOK + " | D6:.5 E6:.5 F6:1 E6:.5 D6:.5 A5:1 | D6:1.5 C6:.5 Bb5:1 F5:1 | G5:.5 A5:.5 Bb5:.5 D6:.5 C6:.5 Bb5:.5 G5:.5 E5:.5 | F5:.5 r:.5 F5:.5 r:.5 F5:.5 F5:.5 r:1";
    const MEL_B = "F5:3 D5:1 | G5:3 E5:1 | A5:3 F5:1 | Bb5:2 D6:2 | Bb5:3 G5:1 | C#6:2 E6:2 | D6:2 F6:2 | E6:2 C6:.5 C6:.5 C6:1";
    const SKANK = [[0.5, 0.15, 84], [1.5, 0.15, 90], [2.5, 0.15, 84], [3.5, 0.15, 90]];
    const ORGAN_STABS = [[0, 0.35, 74], [1.5, 0.35, 66], [3, 0.35, 70]];
    const BASS8 = [[0, 0.45, "R", 104], [0.5, 0.45, "R", 84], [1, 0.45, "8", 92], [1.5, 0.45, "R", 84], [2, 0.45, "R", 100], [2.5, 0.45, "5", 84], [3, 0.45, "8", 92], [3.5, 0.45, "5", 86]];
    const PUNK = { [KICK]: "x.....x.x.....x.", [SNARE]: "....x.......x...", [HAT]: "m.o.m.o.m.o.m.o." };
    const PUNK_FULL = { [KICK]: "x.....x.x.....x.", [SNARE]: "....x.......x..g", [HAT]: "m.o.m.o.m.o.m.o.", [OPEN_HAT]: "..............o." };

    // Intro: a splash of bubbles and a big ringing F, then the brass calls the motif over a building beat
    bubbles(bub, 0, { count: 16, spread: 3, low: 300, high: 1800, velocity: 100 });
    for (const token of ["R", "5", "8"]) power.hit(0, tone({ symbol: "F" }, token, 40), 112, 6);
    bass.hit(0, pitch("F1"), 110, 6);
    kit.hit(0, CRASH, 118, 2).hit(0, KICK, 120);
    roll(kit, 6, 2, SNARE, 40, 100);
    const CALL = "C5:.5 F5:.5 A5:1 G5:.5 F5:.5 C6:1 | A5:.5 G5:.5 E5:.5 C5:.5 r:2";
    brass.at(8).play(CALL, { velocity: 108, gate: 0.85, transpose: -12 });
    steel.at(8).play(CALL, { velocity: 100, gate: 0.9 });
    powerChords(power, progression("F | C", 8), { velocity: 84 });
    bassline(bass, progression("F | C", 8), { pattern: BASS8 });
    beat(kit, 8, 1, PUNK);
    roll(kit, 14, 2, SNARE, 60, 120);

    song.body(s => {
        const a = s, b = s + 64, a2 = s + 96, t = s + 128;
        const pA = progression(A1 + " | " + A2, a), pB = progression(B, b), pA2 = progression(A2, a2), pT = progression("Gm7 | C7", t);

        // A: brass and steel drums on the hook, power chords, skank, organ stabs, punk beat
        brass.at(a).play(MEL_A1 + " | " + MEL_A2, { velocity: 104, gate: 0.85, transpose: -12 });
        steel.at(a).play(MEL_A1 + " | " + MEL_A2, { velocity: 100, gate: 0.9 });
        glock.at(a + 32).play(MEL_A2, { velocity: 60, transpose: 12 });
        powerChords(power, pA, { velocity: 80 });
        comp(skank, pA, { pattern: SKANK, center: 67 });
        comp(organ, pA, { pattern: ORGAN_STABS, center: 62 });
        bassline(bass, pA, { pattern: BASS8 });
        beat(kit, a, 16, PUNK);
        kit.hit(a, CRASH, 112, 1).hit(a + 32, CRASH, 108, 1);
        tomFill(kit, a + 62, 2);

        // B: the build - long notes climbing, ringing chords, the drums double up into a roll
        brass.at(b).play(MEL_B, { velocity: 100, gate: 0.95, transpose: -12 });
        steel.at(b).play(MEL_B, { velocity: 84, gate: 0.9 });
        powerChords(power, pB, { rate: 4, gate: 0.95, velocity: 92 });
        sustain(organ, pB, { center: 60, velocity: 70 });
        comp(skank, pB, { pattern: SKANK, center: 67 });
        bassline(bass, pB, { pattern: [[0, 0.9, "R", 100], [1, 0.9, "R", 84], [2, 0.9, "R", 96], [3, 0.9, "R", 84]] });
        beat(kit, b, 4, { [KICK]: "x.......x.......", [SNARE]: "........x.......", [HAT]: "o.o.o.o.o.o.o.o." });
        beat(kit, b + 16, 2, { [KICK]: "x...x...x...x...", [SNARE]: "o.o.m.m.m.x.x.x.", [HAT]: "o.o.o.o.o.o.o.o." });
        beat(kit, b + 24, 2, { [KICK]: "x...x...x...x..." });
        roll(kit, b + 24, 8, SNARE, 50, 124);
        bubbles(bub, b + 26, { count: 14, spread: 5, low: 300, high: 1800 });

        // A': the big finish, everyone on the second half of the hook
        brass.at(a2).play(MEL_A2, { velocity: 112, gate: 0.85, transpose: -12 });
        steel.at(a2).play(MEL_A2, { velocity: 104, gate: 0.9 });
        glock.at(a2).play(MEL_A2, { velocity: 68, transpose: 12 });
        powerChords(power, pA2, { velocity: 88 });
        comp(skank, pA2, { pattern: SKANK, center: 67 });
        comp(organ, pA2, { pattern: ORGAN_STABS, center: 62 });
        bassline(bass, pA2, { pattern: BASS8 });
        beat(kit, a2, 8, PUNK_FULL);
        for (let bar = 0; bar < 8; bar += 2) kit.hit(a2 + bar * 4, CRASH, 112, 1);

        // Turnaround: a brass run, the steel drums' chromatic pickup back into the hook
        brass.at(t).play("Bb4:.5 C5:.5 D5:.5 E5:.5 F5:.5 G5:.5 A5:.5 Bb5:.5 | C6:.5 r:.5 C5:.5 r:2.5", { velocity: 108, gate: 0.8, transpose: -12 });
        steel.at(t + 4).play("r:2.5 A4:.5 Bb4:.5 B4:.5", { velocity: 98 });
        powerChords(power, pT, { velocity: 84 });
        bassline(bass, pT, { pattern: BASS8 });
        beat(kit, t, 1, PUNK);
        tomFill(kit, t + 6, 2);
    });
    songs.push(song);
}

// =====================================================================================================
// 7. "Fish Tank" - the Fish Tank menus (upgrade, equip, encyclopedia, map). Laid-back aquarium funk: the motif
//    on vibraphone over Rhodes and muted-guitar chops, a whistle in the middle, kalimba answers.
//    Intro 2 | A 16 (vibraphone) | B 8 (whistle) | A' 16 (vibraphone and kalimba, the whistle above)
// =====================================================================================================
{
    const song = new Song("theme_base", "Fish Fighters - Fish Tank", 112, { kind: "menu", introBars: 2, bodyBars: 40,
        mix: { lead: 0, kalimba: -5, keys: -5, guitar: -8, bass: -4, drums: -6, bubbles: -9 } });
    const vibes = song.part("Vibraphone", 11, { group: "lead", volume: 104, pan: -0.1, fx: { chorus: { mix: 0.3 }, echo: { beats: 0.75, feedback: 0.25, mix: 0.15 } } });
    const whistle = song.part("Whistle", 78, { group: "lead", volume: 86, pan: 0.15, fx: { echo: { beats: 0.75, feedback: 0.25, mix: 0.18 } } });
    const kalimba = song.part("Kalimba", 108, { group: "kalimba", volume: 92, pan: 0.4, fx: { echo: { beats: 0.75, feedback: 0.35, mix: 0.28 } } });
    const ep = song.part("Electric piano", 4, { group: "keys", volume: 90, pan: -0.2, fx: { chorus: { mix: 0.5 } } });
    const guitar = song.part("Muted guitar", 28, { group: "guitar", volume: 90, pan: 0.35 });
    const bass = song.part("Bass", 33, { group: "bass", volume: 106 });
    const kit = song.drums("Kit", 0, { group: "drums", volume: 86 });
    const bub = song.synth("Bubbles", "bubbles", { group: "bubbles" });

    const A1 = "Bbmaj7 | Gm7 | Cm7 | F7 | Bbmaj7 | Gm7 | Cm7 F7 | Bbmaj7";
    const A2 = "Bbmaj7 | Gm7 | Ebmaj7 | Ebm6 | Dm7 | G7 | Cm7 F7 | Bbmaj7";
    const B = "Ebmaj7 | F/Eb | Dm7 | Gm7 | Cm7 | F7 | Ebmaj7 | F7";
    const MOTIF = "F4:.5 Bb4:.5 D5:.75 C5:.25 r:.5 Bb4:.5 F5:1";
    const MEL_A = MOTIF + " | r:.5 D5:.5 F5:.5 G5:.5 r:.5 F5:.25 D5:.75 r:.5 | Eb5:.5 G5:.5 Bb5:.75 G5:.25 r:.5 Eb5:.5 C5:1 | r:.5 A4:.5 C5:.5 Eb5:.5 r:.5 D5:.25 C5:.75 r:.5 | " +
        MOTIF + " | r:.5 G5:.5 Bb5:.5 A5:.5 r:.5 G5:.25 F5:.75 r:.5 | Eb5:.5 D5:.5 C5:.5 Bb4:.5 A4:.5 C5:.5 Eb5:.5 F5:.5 | D5:1.5 r:2.5 | " +
        MOTIF + " | r:.5 D5:.5 F5:.5 G5:.5 r:.5 Bb5:.25 A5:.75 r:.5 | G5:.5 Bb5:.5 D6:.75 Bb5:.25 r:.5 G5:.5 Eb5:1 | r:.5 Gb5:.5 Bb5:.5 C6:.5 r:.5 Bb5:.25 Gb5:.75 r:.5 | " +
        "F5:.5 A5:.5 C6:.75 A5:.25 r:.5 F5:.5 D5:1 | r:.5 B4:.5 D5:.5 F5:.5 r:.5 E5:.25 D5:.75 r:.5 | Eb5:.5 G5:.5 Bb5:.5 G5:.5 A5:.5 C6:.5 Eb6:.5 C6:.5 | Bb5:1.5 r:2.5";
    const MEL_B = "G5:1 Bb5:1 D6:2 | C6:1 A5:1 F5:2 | F5:1 A5:1 C6:1.5 A5:.5 | Bb5:2 G5:2 | Eb5:1 G5:1 Bb5:1 C6:1 | A5:1 C6:1 Eb6:2 | D6:1.5 Bb5:.5 G5:2 | F5:1 Eb5:1 C5:1 A4:1";
    const COUNTER = "A4:4 | F4:4 | Eb5:4 | Eb5:2 C5:2 | A4:4 | F4:4 | Eb5:2 Eb5:2 | D5:4 | A4:4 | Bb4:4 | G4:4 | Gb4:4 | F4:4 | F4:4 | Eb5:2 Eb5:2 | D5:4";
    const EP_FUNK = [[0, 0.4, 64], [0.75, 0.2, 56], [1.5, 0.4, 60], [2.5, 0.2, 56], [2.75, 0.45, 62], [3.5, 0.2, 56]];
    const CHOPS = "x.xx.x.x.xx.x.x.";
    const BASS_FUNK = [[0, 0.4, "R", 100], [0.75, 0.2, "8", 78], [1, 0.25, "R", 74], [1.5, 0.4, "5", 88], [2.5, 0.2, "R", 78], [2.75, 0.4, "8", 84], [3.5, 0.4, "app", 86]];
    const FUNK = { [KICK]: "x.....x..x......", [SNARE]: "....x..g.g..x..g", [HAT]: "mgogmgogmgogmgog" };
    const FUNK_LIGHT = { [KICK]: "x.......x.......", [SIDE_STICK]: "....x.......x...", [HAT]: "o.g.o.g.o.g.o.g.", [SHAKER]: "g.g.g.g.g.g.g.g." };
    const answers = start => {
        kalimba.at(start + 30).play("D6:.5 C6:.5 Bb5:.5 F5:.5", { velocity: 72 });
        kalimba.at(start + 62).play("F5:.5 A5:.5 C6:.5 D6:.5", { velocity: 72 });
    };

    // Intro: a held Rhodes chord, bubbles, the kalimba twinkling
    const pI = progression("Bbmaj7 | Cm7 F7", 0);
    sustain(ep, pI, { center: 63, velocity: 60 });
    bassline(bass, pI, { pattern: [[0, 1.9, "R", 90], [2, 1.9, "5", 80]] });
    kalimba.at(0).play("F5:.5 Bb5:.5 D6:1 r:2 | C6:.5 A5:.5 F5:1 r:2", { velocity: 72 });
    bubbles(bub, 0.5, { count: 10, spread: 4, low: 350, high: 1300 });
    beat(kit, 4, 1, { [HAT]: "g.g.g.g.g.g.g.g." });

    song.body(s => {
        const a = s, b = s + 64, a2 = s + 96;
        const pA = progression(A1 + " | " + A2, a), pB = progression(B, b), pA2 = progression(A1 + " | " + A2, a2);

        // A: the motif on vibraphone, Rhodes and guitar chops, funk bass and drums
        vibes.at(a).play(MEL_A, { velocity: 94, gate: 0.85 });
        comp(ep, pA, { pattern: EP_FUNK, center: 63 });
        chops(guitar, pA, { pattern: CHOPS, tones: ["8", "10", "12", "10"], base: 50, velocity: 64 });
        bassline(bass, pA, { pattern: BASS_FUNK });
        beat(kit, a, 16, FUNK);
        answers(a);
        bubbles(bub, a + 29, { count: 5, spread: 1.5 });
        bubbles(bub, a + 61, { count: 7, spread: 2 });

        // B: the whistle sings over held chords, the kalimba sparkles, lighter drums
        whistle.at(b).play(MEL_B, { velocity: 90 });
        sustain(ep, pB, { center: 62, velocity: 56 });
        arp(kalimba, pB, { pattern: [0, 2, 1, 3], rate: 0.5, center: 74, velocity: 52 });
        chops(guitar, pB, { pattern: "x...x...x...x...", tones: ["8"], base: 50, velocity: 56 });
        bassline(bass, pB, { pattern: [[0, 1.4, "R", 92], [1.5, 0.4, "5", 76], [2, 1.9, "8", 84]] });
        beat(kit, b, 8, FUNK_LIGHT);
        bubbles(bub, b + 14, { count: 6, spread: 2, low: 300, high: 1000 });

        // A': vibraphone, the kalimba doubling the second half, the whistle's counter-melody above
        vibes.at(a2).play(MEL_A, { velocity: 96, gate: 0.85 });
        kalimba.at(a2 + 32).play(MEL_A.split("|").slice(8).join("|"), { velocity: 62 });
        whistle.at(a2).play(COUNTER, { velocity: 66, transpose: 12 });
        comp(ep, pA2, { pattern: EP_FUNK, center: 63 });
        chops(guitar, pA2, { pattern: CHOPS, tones: ["8", "10", "12", "10"], base: 50, velocity: 66 });
        bassline(bass, pA2, { pattern: BASS_FUNK });
        beat(kit, a2, 16, FUNK);
        kit.hit(a2, CRASH, 80, 1);
        kalimba.at(a2 + 30).play("D6:.5 C6:.5 Bb5:.5 F5:.5", { velocity: 72 });
        bubbles(bub, a2 + 61, { count: 9, spread: 3, low: 350, high: 1400 });
    });
    songs.push(song);
}

// =====================================================================================================
// 8. "Sunken Galleon" - shipwreck stages. A sea-shanty jig in 12/8 (a beat is a dotted quarter, an eighth 1/3):
//    fiddle and flute over accordion, strummed guitar and an oom-pah tuba, stomps, claps and tambourine.
//    In the middle we sink into the wreck, where a music box plays the motif in the dark.
//    Intro 2 | A 16 (fiddle) | B 8 (heave-ho: whistle, stomps and claps) | C 8 (the wreck) | A' 16 (fiddle and flute)
// =====================================================================================================
{
    const song = new Song("theme_shanty", "Fish Fighters - Sunken Galleon", 126, { introBars: 2, bodyBars: 48,
        mix: { lead: 0, keys: -1, accordion: -5, guitar: -7, bass: -4, drums: -4, fx: -8 } });
    const fiddle = song.part("Fiddle", 40, { group: "lead", volume: 104, pan: -0.1, fx: { echo: { beats: 1, feedback: 0.2, mix: 0.1 } } });
    const flute = song.part("Flute", 73, { group: "lead", volume: 86, pan: 0.2 });
    const whistle = song.part("Whistle", 78, { group: "lead", volume: 88, pan: 0.15, fx: { echo: { beats: 1, feedback: 0.25, mix: 0.15 } } });
    const musicBox = song.part("Music box", 10, { group: "keys", volume: 104, fx: { echo: { beats: 1, feedback: 0.4, mix: 0.3 } } });
    const accordion = song.part("Accordion", 21, { group: "accordion", volume: 92, pan: 0.25 });
    const guitar = song.part("Guitar", 25, { group: "guitar", volume: 90, pan: -0.35 });
    const tuba = song.part("Tuba", 58, { group: "bass", volume: 104 });
    const kit = song.drums("Kit", 0, { group: "drums", volume: 92 });
    const ghost = song.part("Sunken voice", 91, { group: "fx", volume: 96, fx: { lowpass: 1500, echo: { beats: 2, feedback: 0.4, mix: 0.35 } } });
    const bub = song.synth("Bubbles", "bubbles", { group: "fx" });

    const A1 = "Dm | C | Dm | Am | Dm | C | Bb A | Dm";
    const A2 = "F | C | Dm | Am | Bb | C | Bb A | Dm";
    const B = "F | C | Dm | A | F | C | Gm | A";
    const C = "Dm | Dm | Bb | C | Dm | Dm | Bb | A";
    const JIG = "A4:1/3 D5:1/3 F5:1/3 E5:2/3 D5:1/3 A5:2/3 G5:1/3 F5:2/3 D5:1/3";
    const MEL_A = JIG + " | E5:2/3 C5:1/3 G5:2/3 E5:1/3 C6:2/3 B5:1/3 G5:2/3 E5:1/3 | F5:2/3 D5:1/3 A5:2/3 F5:1/3 D6:2/3 C6:1/3 A5:2/3 F5:1/3 | E5:2/3 A4:1/3 C5:2/3 E5:1/3 A5:1 G5:2/3 E5:1/3 | " +
        JIG + " | G5:2/3 E5:1/3 C5:2/3 E5:1/3 G5:1 C6:1 | D6:2/3 C6:1/3 Bb5:2/3 F5:1/3 E5:2/3 C#5:1/3 A4:1 | D5:1 F5:1/3 E5:1/3 D5:1/3 D5:1 r:1 | " +
        "C5:1/3 F5:1/3 A5:1/3 G5:2/3 F5:1/3 C6:1 A5:2/3 F5:1/3 | G5:2/3 E5:1/3 G5:2/3 C6:1/3 E6:2/3 D6:1/3 C6:2/3 G5:1/3 | A5:2/3 F5:1/3 D5:2/3 F5:1/3 A5:2/3 D6:1/3 C6:2/3 A5:1/3 | C6:2/3 A5:1/3 E5:2/3 A5:1/3 C6:1 B5:2/3 G5:1/3 | " +
        "Bb5:2/3 F5:1/3 D5:2/3 F5:1/3 Bb5:2/3 C6:1/3 D6:1 | E6:2/3 D6:1/3 C6:2/3 G5:1/3 E5:2/3 G5:1/3 C6:1 | D6:2/3 C6:1/3 Bb5:2/3 G5:1/3 A5:2/3 G5:1/3 E5:2/3 C#5:1/3 | D5:1 A4:1/3 D5:1/3 F5:1/3 D5:1 r:1";
    const MEL_B = "C6:2 A5:1 F5:1 | G5:2 E5:1 C5:1 | D5:1 F5:1 A5:2 | E5:1 A5:1 C#6:2 | A5:1 C6:1 A5:2 | G5:1 E5:1 G5:2 | Bb5:1 A5:1 G5:1 D5:1 | C#5:1 E5:1 A5:1 r:1";
    const MEL_C = "A5:1 D6:1 F6:2 | E6:1 D6:1 A5:2 | Bb5:1 D6:1 F6:1 D6:1 | E6:2 C6:2 | A5:1 D6:1 F6:2 | E6:1 F6:1 A6:2 | F6:1 D6:1 Bb5:2 | r:4";
    const RUN = "E4:1/3 A4:1/3 C#5:1/3 E5:1/3 A5:1/3 C#6:1/3 E5:1/3 A5:1/3 C#6:1/3 E6:1/3 C#6:1/3 A5:1/3";
    const TUBA = [[0, 0.9, "R", 100], [1, 0.9, "5", 84], [2, 0.9, "R", 96], [3, 0.9, "-5", 84]];
    const STRUM = [0, 1, 2, 3].flatMap(b => [[b, 0.6, 76], [b + 2 / 3, 0.3, 60]]);
    const PAH = [0, 1, 2, 3].map(b => [b + 2 / 3, 0.3, b % 2 ? 66 : 72]);
    const JIG_KIT = { [KICK]: "x.....x.....", [SNARE]: "...m.....m..", [TAMBOURINE]: "mggoggmggogg", [LOW_TOM]: "..g..g..g..g" };
    const STOMP = { [KICK]: "X.....X.....", [CLAP]: "...x.....x..", [TAMBOURINE]: "g.gg.gg.gg.g" };

    // Intro: a held accordion chord over stomps and claps, then the fiddle runs up into the jig
    voice("Dm", 62).forEach(n => accordion.hit(0, n, 80, 3.9));
    beat(kit, 0, 2, STOMP, 12);
    tuba.hit(0, pitch("D2"), 100, 1.8).hit(2, pitch("A1"), 92, 1.8).hit(4, pitch("A1"), 96, 3.8);
    fiddle.at(4).play(RUN, { velocity: 96, gate: 0.9 });
    bubbles(bub, 0.5, { count: 8, spread: 3, low: 300, high: 1100 });

    song.body(s => {
        const a = s, b = s + 64, c = s + 96, a2 = s + 128;
        const pA = progression(A1 + " | " + A2, a), pB = progression(B, b), pC = progression(C, c), pA2 = progression(A1 + " | " + A2, a2);

        // A: the fiddle's jig, accordion on the off-beats, strummed guitar, oom-pah tuba
        fiddle.at(a).play(MEL_A, { velocity: 100, gate: 0.85 });
        comp(accordion, pA, { pattern: PAH, center: 62 });
        comp(guitar, pA, { pattern: STRUM, center: 57, strum: 0.03 });
        bassline(tuba, pA, { pattern: TUBA });
        beat(kit, a, 16, JIG_KIT, 12);
        bubbles(bub, a + 30, { count: 5, spread: 1.5 });
        bubbles(bub, a + 62, { count: 6, spread: 1.5 });

        // B: heave-ho - the whistle over held accordion chords, stomps and claps
        whistle.at(b).play(MEL_B, { velocity: 94 });
        sustain(accordion, pB, { center: 62, velocity: 74 });
        comp(guitar, pB, { pattern: [[0, 1.8, 72], [2, 1.8, 66]], center: 57, strum: 0.04 });
        bassline(tuba, pB, { pattern: [[0, 1.9, "R", 96], [2, 1.9, "5", 86]] });
        beat(kit, b, 8, STOMP, 12);

        // C: the wreck - a music box plays the motif in the dark, a sunken voice, bubbles; the fiddle runs back up
        musicBox.at(c).play(MEL_C, { velocity: 96 });
        ghost.hit(c, pitch("A3"), 90, 6, -3).hit(c + 16, pitch("D4"), 86, 6, -5);
        sustain(accordion, pC, { center: 60, velocity: 44 });
        bassline(tuba, pC, { pattern: [[0, 3.8, "R", 76]] });
        beat(kit, c, 7, { [TAMBOURINE]: "g.....g....." }, 12);
        roll(kit, c + 28, 4, TAMBOURINE, 40, 110, 1 / 3);
        fiddle.at(c + 28).play(RUN, { velocity: 100, gate: 0.9 });
        for (let bar = 0; bar < 8; bar += 2) bubbles(bub, c + bar * 4 + 1, { count: 7, spread: 3, low: 220, high: 900, size: [0.06, 0.15] });

        // A': fiddle and flute together, the claps join in
        fiddle.at(a2).play(MEL_A, { velocity: 104, gate: 0.85 });
        flute.at(a2).play(MEL_A, { velocity: 82, gate: 0.85 });
        comp(accordion, pA2, { pattern: PAH, center: 62 });
        comp(guitar, pA2, { pattern: STRUM, center: 57, strum: 0.03 });
        bassline(tuba, pA2, { pattern: TUBA });
        beat(kit, a2, 16, { ...JIG_KIT, [CLAP]: "...x.....x.." }, 12);
    });
    songs.push(song);
}

// The jellyfish lair and its king share their tune (E minor, with a phrygian F major 7 over the E)
const JELLY = {
    A1: "Em | Fmaj7/E | Em | Fmaj7/E | Am | C | B7 | B7",
    A2: "Em | Fmaj7/E | Em | Fmaj7/E | C | Am | B7 | Em",
    B: "Cmaj7 | B7 | Em | Em/D | Cmaj7 | Am | B7sus4 | B7",
    C: "Em | Em | F/E | F/E | Em | Em | B7 | B7",
    MEL_A: "B4:.5 E5:.5 G5:1 F#5:.5 E5:.5 B5:1 | C6:1.5 B5:.5 A5:1 F5:1 | B4:.5 E5:.5 G5:1 F#5:.5 E5:.5 E6:1 | F5:1 E5:.5 C5:.5 A4:2 | " +
        "A4:.5 C5:.5 E5:1 D5:.5 C5:.5 A5:1 | G5:1.5 E5:.5 C5:1 G4:1 | F#5:.5 A5:.5 B5:1 D#5:1 F#5:1 | B5:2 r:2 | " +
        "B4:.5 E5:.5 G5:1 F#5:.5 E5:.5 B5:1 | C6:1 B5:1 A5:1 E5:1 | G5:.5 F#5:.5 E5:.5 B4:.5 E5:1 G5:1 | A5:1.5 G5:.5 F5:1 E5:1 | " +
        "E5:.5 G5:.5 C6:1 B5:.5 G5:.5 E5:1 | A5:1 C6:1 E6:1 C6:1 | D#6:1 B5:1 A5:1 F#5:1 | E5:3 r:1",
    MEL_B: "E5:4 | D#5:4 | B4:4 | B4:2 A4:2 | G4:4 | A4:2 C5:2 | B4:4 | B4:2 D#5:2",
    FRAGMENTS: "B3:.5 E4:.5 G4:1 r:2 | r:4 | C4:.5 F4:.5 A4:1 r:2 | r:4 | B3:.5 E4:.5 G4:1 F#4:1 r:1 | r:4 | D#4:1 F#4:1 A4:1 B4:1 | B4:2 r:2",
};

// =====================================================================================================
// 9. "Jellyfish Lair" - jellyfish stages. Dark and sinister: a filtered saw lead (an electric eel) with celesta
//    glints, bowed glass and tremolo strings, deep synth pulses, a slow electronic beat, bubbles and electric zaps.
//    Intro 4 | A 16 (the eel) | B 8 (the lair's heart: a choir) | C 8 (electric storm) | A' 16
// =====================================================================================================
{
    const song = new Song("theme_jelly", "Fish Fighters - Jellyfish Lair", 124, { introBars: 4, bodyBars: 48,
        mix: { lead: 0, choir: -4, pad: -7, arp: -8, bass: -3, drums: -4, fx: -6 } });
    const eel = song.part("Eel lead", 81, { group: "lead", volume: 100, pan: -0.1, fx: { lowpass: 2600, echo: { beats: 0.75, feedback: 0.3, mix: 0.2 } } });
    const celesta = song.part("Celesta", 8, { group: "lead", volume: 84, pan: 0.3, fx: { echo: { beats: 0.75, feedback: 0.35, mix: 0.25 } } });
    const glass = song.part("Bowed glass", 92, { group: "pad", volume: 86, fx: { lowpass: 3000 } });
    const tremolo = song.part("Tremolo strings", 44, { group: "pad", volume: 84, pan: 0.2 });
    const choir = song.part("Choir", 91, { group: "choir", volume: 96, fx: { lowpass: 2200, echo: { beats: 1.5, feedback: 0.3, mix: 0.25 } } });
    const crystal = song.part("Crystal", 98, { group: "arp", volume: 84, pan: 0.35, fx: { echo: { beats: 0.75, feedback: 0.4, mix: 0.3 } } });
    const pulse = song.part("Pulse bass", 39, { group: "bass", volume: 100 });
    const kit = song.drums("Kit", 24, { group: "drums", volume: 92 });
    const zap = song.synth("Zaps", "zaps", { group: "fx", fx: { echo: { beats: 0.75, feedback: 0.3, mix: 0.25 } } });
    const bub = song.synth("Bubbles", "bubbles", { group: "fx" });

    const PULSE = [[0, 0.4, "R", 100], [0.5, 0.4, "R", 70], [0.75, 0.2, "R", 60], [1, 0.4, "8", 84], [1.5, 0.4, "R", 72],
        [2, 0.4, "R", 96], [2.5, 0.4, "R", 70], [2.75, 0.2, "R", 60], [3, 0.4, "8", 84], [3.5, 0.4, "5", 76]];
    const LAIR = { [KICK]: "x.....x...x.....", [SNARE]: "........x.......", [HAT]: "g.g.o.g.g.g.o.g." };
    const LAIR_FULL = { [KICK]: "x.....x...x...g.", [SNARE]: "........x.....g.", [CLAP]: "........x.......", [HAT]: "g.g.o.g.g.g.o.g." };
    const DROPS = { pattern: [0, 2, 1, 3, 2, 4], rate: 0.75, center: 76, velocity: 48 };

    // Intro: glass and a low choir, the pulse wakes up, a celesta glint of the motif, crackles in the dark
    const pI = progression("Em | Fmaj7/E | Em | B7", 0);
    sustain(glass, pI, { center: 64, velocity: 60 });
    sustain(choir, progression("Em | Fmaj7/E", 0), { center: 52, velocity: 62 });
    bassline(pulse, progression("Fmaj7/E | Em | B7", 4), { pattern: PULSE });
    celesta.at(8).play("B5:.5 E6:.5 G6:1 F#6:.5 E6:.5 B6:1 | r:4", { velocity: 70 });
    zaps(zap, 2, { count: 2, spread: 1 });
    zaps(zap, 12, { count: 3, spread: 2, velocity: 100 });
    bubbles(bub, 0, { count: 10, spread: 6, low: 220, high: 900, size: [0.06, 0.14] });
    beat(kit, 8, 1, { [HAT]: "g.g.g.g.g.g.g.g." });
    beat(kit, 12, 1, { [KICK]: "x...x...x...x...", [HAT]: "g.g.g.g.g.g.g.g." });

    song.body(s => {
        const a = s, b = s + 64, c = s + 96, a2 = s + 128;
        const pA = progression(JELLY.A1 + " | " + JELLY.A2, a), pB = progression(JELLY.B, b), pC = progression(JELLY.C, c);
        const pA2 = progression(JELLY.A1 + " | " + JELLY.A2, a2);

        // A: the eel lead over glass, pulses and a slow electronic beat; zaps at the phrase ends
        eel.at(a).play(JELLY.MEL_A, { velocity: 100, gate: 0.9, transpose: -12 });
        celesta.at(a + 32).play(JELLY.MEL_A.split("|").slice(8).join("|"), { velocity: 58 });
        sustain(glass, pA, { center: 64, velocity: 54 });
        bassline(pulse, pA, { pattern: PULSE });
        arp(crystal, pA, DROPS);
        beat(kit, a, 16, LAIR);
        for (const bar of [3, 7, 11, 15]) zaps(zap, a + bar * 4 + 2, { count: 2, spread: 1 });
        bubbles(bub, a + 30, { count: 6, spread: 2, low: 250, high: 1000 });
        bubbles(bub, a + 62, { count: 6, spread: 2, low: 250, high: 1000 });

        // B: the lair's heart - a choir over tremolo strings, half time, crystal drops
        choir.at(b).play(JELLY.MEL_B, { velocity: 92, gate: 1, transpose: -12 });
        sustain(tremolo, pB, { center: 55, velocity: 70 });
        sustain(glass, pB, { center: 66, velocity: 50 });
        arp(crystal, pB, { pattern: [4, 2, 3, 1], rate: 1, center: 76, velocity: 54 });
        bassline(pulse, pB, { pattern: [[0, 1.9, "R", 96], [2, 0.45, "R", 76], [2.5, 0.45, "8", 76], [3, 0.9, "R", 84]] });
        beat(kit, b, 8, { [KICK]: "x.........x.....", [SNARE]: "........x.......", [HAT]: "g...g...g...g..." });
        zaps(zap, b + 12, { count: 2, spread: 1 });
        zaps(zap, b + 28, { count: 3, spread: 2 });

        // C: electric storm - zaps everywhere, the pulse alone, fragments of the motif, building back
        eel.at(c).play(JELLY.FRAGMENTS, { velocity: 104, gate: 0.9 });
        bassline(pulse, pC, { pattern: PULSE });
        sustain(glass, pC, { center: 62, velocity: 46 });
        for (let bar = 0; bar < 8; bar++) zaps(zap, c + bar * 4 + (bar % 2 ? 2.5 : 0.5), { count: bar < 4 ? 2 : 4, spread: 1.5, velocity: 90 + bar * 3 });
        beat(kit, c, 4, { [HAT]: "g.g.g.g.g.g.g.g." });
        beat(kit, c + 16, 2, { [KICK]: "x...x...x...x...", [HAT]: "g.g.g.g.g.g.g.g." });
        beat(kit, c + 24, 2, { [KICK]: "x.x.x.x.x.x.x.x.", [HAT]: "gggggggggggggggg" });
        roll(kit, c + 28, 4, SNARE, 40, 115);
        bubbles(bub, c + 8, { count: 10, spread: 6, low: 220, high: 1200 });

        // A': everything - the celesta doubles the eel, the choir holds the chords
        eel.at(a2).play(JELLY.MEL_A, { velocity: 104, gate: 0.9, transpose: -12 });
        celesta.at(a2).play(JELLY.MEL_A, { velocity: 64 });
        sustain(choir, pA2, { center: 55, velocity: 56 });
        sustain(tremolo, pA2, { center: 52, velocity: 56 });
        bassline(pulse, pA2, { pattern: PULSE });
        arp(crystal, pA2, DROPS);
        beat(kit, a2, 16, LAIR_FULL);
        kit.hit(a2, CRASH, 100, 1);
        for (const bar of [3, 7, 11, 15]) zaps(zap, a2 + bar * 4 + 2, { count: 3, spread: 1.5, velocity: 100 });
        bubbles(bub, a2 + 62, { count: 8, spread: 2, low: 250, high: 1200 });
    });
    songs.push(song);
}

// =====================================================================================================
// 10. "Jellyfish King" - the jellyfish king's boss stages: the lair's tune, crowned. A church organ for the king,
//     a choir, brass stabs on a 3-3-2, the eel lead, a driving synth bass, timpani, power drums, bigger zaps.
//     Intro 4 | A 16 | B 8 (the king's chant) | C 8 (electric storm) | A' 16
// =====================================================================================================
{
    const song = new Song("theme_jelly_boss", "Fish Fighters - Jellyfish King", 152, { introBars: 4, bodyBars: 48,
        mix: { brass: 0, lead: -1, organ: -3, choir: -4, bass: -4, drums: -2, fx: -6 } });
    const brass = song.part("Brass", 61, { group: "brass", volume: 110, pan: -0.15 });
    const eel = song.part("Eel lead", 81, { group: "lead", volume: 100, pan: 0.15, fx: { lowpass: 3000, echo: { beats: 0.5, feedback: 0.25, mix: 0.15 } } });
    const celesta = song.part("Celesta", 8, { group: "lead", volume: 84, pan: 0.3 });
    const organ = song.part("Church organ", 19, { group: "organ", volume: 96 });
    const choir = song.part("Choir", 52, { group: "choir", volume: 98 });
    const bass = song.part("Synth bass", 38, { group: "bass", volume: 100 });
    const kit = song.drums("Kit", 16, { group: "drums", volume: 96 });
    const timpani = song.part("Timpani", 47, { group: "drums", volume: 104 });
    const zap = song.synth("Zaps", "zaps", { group: "fx", fx: { echo: { beats: 0.5, feedback: 0.3, mix: 0.2 } } });
    const bub = song.synth("Bubbles", "bubbles", { group: "fx" });

    const STABS = [[0, 0.3, 102], [1.5, 0.3, 94], [3, 0.3, 98]];
    const DRIVE8 = [[0, 0.45, "R", 100], [0.5, 0.45, "R", 80], [1, 0.45, "8", 86], [1.5, 0.45, "R", 96], [2, 0.45, "R", 80], [2.5, 0.45, "8", 86], [3, 0.45, "R", 96], [3.5, 0.45, "8", 84]];
    const KING = { [KICK]: "x...x...x...x.x.", [SNARE]: "....x.......x...", [HAT]: "m.o.m.o.m.o.m.o." };
    const KING_FULL = { [KICK]: "x...x...x...x.x.", [SNARE]: "....x.......x..g", [HAT]: "m.o.m.o.m.o.m.o.", [OPEN_HAT]: "..............o." };
    const HALF = { [KICK]: "x.....x.........", [SNARE]: "........x.......", [RIDE]: "o.o.o.o.o.o.o.o." };
    const organChords = (prog, velocity) => {
        sustain(organ, prog, { center: 57, velocity });
        bassline(organ, prog, { pattern: [[0, 3.9, "R", velocity + 10]] });   // the pedal
    };
    const timpaniOn = (prog, start, bars, every = 2) => {
        for (let bar = 0; bar < bars; bar += every) timpani.hit(start + bar * 4, above(chord(chordAt(prog, start + bar * 4).symbol).root, 33), 108, 1.5);
    };

    // Intro: the king's organ chord, a timpani roll and big zaps, then the brass announces the motif
    const pI = progression("Em | Em | Em | B7", 0);
    organChords(progression("Em", 0), 84);
    roll(timpani, 0, 8, 40, 40, 110);
    zaps(zap, 0, { count: 3, spread: 1, velocity: 110 });
    zaps(zap, 6, { count: 2, spread: 1, velocity: 100 });
    sustain(choir, progression("Em | Em", 0), { center: 55, velocity: 70 });
    brass.at(8).play("B4:.5 E5:.5 G5:1 F#5:.5 E5:.5 B5:1 | D#5:1 F#5:1 B5:2", { velocity: 108, gate: 0.85, transpose: -12 });
    organChords(progression("Em | B7", 8), 84);
    bassline(bass, progression("Em | B7", 8), { pattern: DRIVE8 });
    beat(kit, 8, 1, { [KICK]: "x...x...x...x..." });
    tomFill(kit, 14, 2);
    bubbles(bub, 1, { count: 12, spread: 6, low: 200, high: 900, size: [0.06, 0.16] });

    song.body(s => {
        const a = s, b = s + 64, c = s + 96, a2 = s + 128;
        const pA = progression(JELLY.A1 + " | " + JELLY.A2, a), pB = progression(JELLY.B, b), pC = progression(JELLY.C, c);
        const pA2 = progression(JELLY.A1 + " | " + JELLY.A2, a2);

        // A: the eel sings the lair's tune, brass stabs, the king's organ, driving bass
        eel.at(a).play(JELLY.MEL_A, { velocity: 104, gate: 0.85, transpose: -12 });
        comp(brass, pA, { pattern: STABS, center: 60 });
        organChords(pA, 70);
        sustain(choir, progression(JELLY.A2, a + 32), { center: 60, velocity: 56 });
        bassline(bass, pA, { pattern: DRIVE8 });
        beat(kit, a, 16, KING);
        for (let bar = 0; bar < 16; bar += 4) kit.hit(a + bar * 4, CRASH, 106, 1);
        timpaniOn(pA, a, 16);
        for (const bar of [3, 7, 11, 15]) zaps(zap, a + bar * 4 + 2, { count: 3, spread: 1.5, velocity: 104 });
        tomFill(kit, a + 62, 2);

        // B: the king's chant - the choir over the full organ, brass on every bar, half time, timpani
        choir.at(b).play(JELLY.MEL_B, { velocity: 100, gate: 1 });
        organChords(pB, 80);
        comp(brass, pB, { pattern: [[0, 1.8, 92]], center: 58 });
        bassline(bass, pB, { pattern: [[0, 1.9, "R", 100], [2, 0.45, "R", 80], [2.5, 0.45, "8", 80], [3, 0.45, "R", 84], [3.5, 0.45, "8", 84]] });
        beat(kit, b, 8, HALF);
        kit.hit(b, CRASH, 100, 1);
        for (let bar = 0; bar < 8; bar++) [0, 2].forEach(t => timpani.hit(b + bar * 4 + t, above(chord(chordAt(pB, b + bar * 4).symbol).root, 33), 96, 1.5));
        zaps(zap, b + 14, { count: 3, spread: 1.5 });
        zaps(zap, b + 30, { count: 4, spread: 2 });
        tomFill(kit, b + 30, 2);

        // C: electric storm - the organ's pedal, zaps on every bar, the eel's fragments, the drums build
        eel.at(c).play(JELLY.FRAGMENTS, { velocity: 106, gate: 0.9 });
        bassline(organ, pC, { pattern: [[0, 3.9, "R", 90]] });
        bassline(bass, pC, { pattern: DRIVE8 });
        sustain(choir, pC, { center: 52, velocity: 58 });
        for (let bar = 0; bar < 8; bar++) zaps(zap, c + bar * 4 + (bar % 2 ? 2 : 0), { count: bar < 4 ? 3 : 5, spread: 2, velocity: 96 + bar * 3 });
        beat(kit, c, 4, { [KICK]: "x.......x.......", [HAT]: "o.o.o.o.o.o.o.o." });
        beat(kit, c + 16, 2, { [KICK]: "x...x...x...x...", [SNARE]: "....x.......x..." });
        beat(kit, c + 24, 2, { [KICK]: "x.x.x.x.x.x.x.x." });
        roll(kit, c + 28, 4, SNARE, 40, 120);
        roll(timpani, c + 24, 8, 40, 40, 118);
        bubbles(bub, c + 4, { count: 12, spread: 6, low: 200, high: 1200, size: [0.06, 0.16] });

        // A': the crowned tune - brass and eel together, celesta on top, choir, organ, full drums
        brass.at(a2).play(JELLY.MEL_A, { velocity: 110, gate: 0.85, transpose: -12 });
        eel.at(a2).play(JELLY.MEL_A, { velocity: 100, gate: 0.85, transpose: -12 });
        celesta.at(a2).play(JELLY.MEL_A, { velocity: 70 });
        organChords(pA2, 74);
        sustain(choir, pA2, { center: 60, velocity: 62 });
        bassline(bass, pA2, { pattern: DRIVE8 });
        beat(kit, a2, 16, KING_FULL);
        for (let bar = 0; bar < 16; bar += 4) kit.hit(a2 + bar * 4, CRASH, 112, 1);
        timpaniOn(pA2, a2, 16, 1);
        for (const bar of [3, 7, 11, 15]) zaps(zap, a2 + bar * 4 + 2, { count: 4, spread: 1.5, velocity: 110 });
        tomFill(kit, a2 + 62, 2);
    });
    songs.push(song);
}

// The crab coves and their king share their tune: A minor with the Spanish (Andalusian) cadence Am G F E,
// the motif tiptoeing sideways in staccato like a crab, castanets and claves clicking like claws
const CRAB = {
    A1: "Am | G | F | E | Am | G | F E | Am",
    A2: "Am | G | F | E | Dm | Am | F E | Am",
    B: "Dm | Am | E | Am | Dm | Am | B7 | E7",
    C: "Am | G | F | E | Am | G | F | E",
    MOTIF: "E4:.5 A4:.5 C5:.5 r:.5 B4:.5 A4:.5 E5:.5 r:.5",
    get MEL_A() {
        return this.MOTIF + " | D5:.5 B4:.5 G4:.5 r:.5 D5:.25 E5:.25 D5:.5 B4:1 | C5:.5 A4:.5 F4:.5 r:.5 A4:.5 C5:.5 F5:.5 r:.5 | E5:.5 D5:.5 C5:.5 B4:.5 G#4:1 E4:1 | " +
            this.MOTIF + " | D5:.5 E5:.5 F5:.5 r:.5 E5:.5 D5:.5 B4:1 | A4:.5 C5:.5 F5:.5 E5:.5 G#4:.5 B4:.5 E5:1 | A4:1 r:.5 E4:.5 A4:1 r:1 | " +
            this.MOTIF + " | D5:.5 B4:.5 G4:.5 r:.5 D5:.25 E5:.25 D5:.5 B4:1 | C5:.5 A4:.5 F4:.5 A4:.5 C5:.5 D5:.5 C5:1 | B4:.5 C5:.5 B4:.5 A4:.5 G#4:.5 F4:.5 E4:1 | " +
            "D5:.5 F5:.5 A4:.5 r:.5 D5:.5 E5:.5 F5:1 | E5:.5 C5:.5 A4:.5 r:.5 C5:.5 B4:.5 A4:1 | F4:.5 A4:.5 C5:.5 D5:.5 E5:.5 D5:.5 C5:.5 B4:.5 | A4:1 r:.5 E4:.5 A4:1 r:1";
    },
    MEL_B: "A4:1 D5:1 F5:1.5 E5:.5 | E5:1 C5:1 A4:2 | G#4:.5 A4:.5 B4:1 E5:1.5 D5:.5 | C5:1 B4:.5 A4:.5 E4:2 | " +
        "F5:1 E5:.5 D5:.5 A5:2 | G5:.5 F5:.5 E5:.5 D5:.5 C5:2 | D#5:1 F#5:1 A5:1 F#5:1 | G#5:1 E5:1 D5:1 B4:1",
    FRAGMENTS: "E4:.5 A4:.5 C5:.5 r:2.5 | r:4 | F4:.5 A4:.5 C5:.5 r:2.5 | r:2 G#4:.5 B4:.5 E5:1 | " +
        "E4:.5 A4:.5 C5:.5 r:2.5 | r:4 | F4:.5 A4:.5 C5:.5 r:2.5 | G#4:.5 B4:.5 D5:.5 E5:.5 G#4:1 r:1",
    // A flamenco strum (rasgueado): accents on the beats, a fast flourish at the end of the bar
    RASGUEADO: [[0, 0.4, 92], [0.5, 0.2, 70], [1, 0.4, 84], [1.5, 0.2, 66], [2, 0.4, 88], [2.5, 0.2, 70], [3, 0.25, 86], [3.25, 0.25, 72], [3.5, 0.25, 80], [3.75, 0.25, 72]],
    CLAWS: { [CASTANETS]: "x.mxm.x.x.mxm.x.", [CLAVES]: "x..x..x...x.x...", [WOOD_LOW]: "....o.......o...", [KICK]: "o.......o......." },
    CLAWS_PALMAS: { [CASTANETS]: "x.mxm.x.x.mxm.x.", [CLAVES]: "x..x..x...x.x...", [CLAP]: "..o...x...o...x.", [KICK]: "x.......x.....o.", [GUIRO]: "........o......." },
};

// =====================================================================================================
// 11. "Crab Cove" - crustacean stages. A sneaky, sideways tiptoe: a staccato bassoon over flamenco guitar strums,
//     pizzicato and an oom-pah tuba, castanets and claves clicking like claws, palmas (hand claps) in the middle.
//     Intro 2 | A 16 (bassoon) | B 8 (muted trumpet, palmas) | C 8 (the claws: castanets, strums, fragments) | A' 16
// =====================================================================================================
{
    const song = new Song("theme_crab", "Fish Fighters - Crab Cove", 132, { introBars: 2, bodyBars: 48,
        mix: { lead: 0, guitar: -4, strings: -6, bass: -4, percussion: -3, fx: -9 } });
    const bassoon = song.part("Bassoon", 70, { group: "lead", volume: 106, pan: -0.1, fx: { echo: { beats: 0.75, feedback: 0.2, mix: 0.1 } } });
    const xylo = song.part("Xylophone", 13, { group: "lead", volume: 86, pan: 0.3 });
    const trumpet = song.part("Muted trumpet", 59, { group: "lead", volume: 100, pan: 0.15, fx: { echo: { beats: 0.75, feedback: 0.2, mix: 0.12 } } });
    const guitar = song.part("Nylon guitar", 24, { group: "guitar", volume: 96, pan: -0.35 });
    const pizz = song.part("Pizzicato", 45, { group: "strings", volume: 94, pan: 0.35 });
    const tuba = song.part("Tuba", 58, { group: "bass", volume: 104 });
    const kit = song.drums("Claws", 0, { group: "percussion", volume: 94 });
    const bub = song.synth("Bubbles", "bubbles", { group: "fx" });
    const TUBA = [[0, 0.9, "R", 100], [1, 0.4, "5", 80], [2, 0.9, "R", 96], [3, 0.4, "-5", 80]];

    // Intro: castanets clicking in, two strums, the bassoon tiptoes up to the tune
    const pI = progression("Am | E", 0);
    beat(kit, 0, 2, { [CASTANETS]: "....x.mxm.x.mxmx", [CLAVES]: "x..x..x...x.x..." });
    comp(guitar, pI, { pattern: [[0, 1.5, 96], [2, 1.5, 90]], center: 57, strum: 0.03 });
    tuba.hit(0, pitch("A1"), 96, 1.8).hit(4, pitch("E2"), 96, 1.8);
    bassoon.at(6).play("G#4:.5 B4:.5", { velocity: 90, gate: 0.6 });
    bubbles(bub, 0.5, { count: 8, spread: 3, low: 300, high: 1100 });

    song.body(s => {
        const a = s, b = s + 64, c = s + 96, a2 = s + 128;
        const pA = progression(CRAB.A1 + " | " + CRAB.A2, a), pB = progression(CRAB.B, b), pC = progression(CRAB.C, c);
        const pA2 = progression(CRAB.A1 + " | " + CRAB.A2, a2);

        // A: the bassoon tiptoes, guitar strums, pizzicato and tuba, castanets and claves
        bassoon.at(a).play(CRAB.MEL_A, { velocity: 100, gate: 0.55 });
        comp(guitar, pA, { pattern: CRAB.RASGUEADO, center: 57, strum: 0.02 });
        figure(pizz, pA, { pattern: ["R", "5", "8", "5"], rate: 0.5, base: 45, velocity: 70 });
        bassline(tuba, pA, { pattern: TUBA });
        beat(kit, a, 16, CRAB.CLAWS);
        bubbles(bub, a + 30, { count: 5, spread: 1.5 });
        bubbles(bub, a + 62, { count: 6, spread: 1.5 });

        // B: the muted trumpet sings, palmas, a fuller strum, walking tuba
        trumpet.at(b).play(CRAB.MEL_B, { velocity: 98, gate: 0.85 });
        comp(guitar, pB, { pattern: CRAB.RASGUEADO, center: 57, strum: 0.02 });
        figure(pizz, pB, { pattern: ["R", "8", "5", "8"], rate: 0.5, base: 45, velocity: 74 });
        bassline(tuba, pB, { pattern: [[0, 0.9, "R", 96], [1, 0.9, "5", 84], [2, 0.9, "8", 90], [3, 0.9, "app", 84]] });
        beat(kit, b, 8, CRAB.CLAWS_PALMAS);

        // C: the claws - castanets and strums alone, the bassoon's fragments, then everyone comes back
        bassoon.at(c).play(CRAB.FRAGMENTS, { velocity: 100, gate: 0.6 });
        comp(guitar, pC, { pattern: [[0, 0.3, 96], [0.25, 0.3, 80], [0.5, 0.3, 84], [0.75, 0.3, 76], [2, 1.5, 88]], center: 57, strum: 0.015 });
        bassline(tuba, pC, { pattern: [[0, 1.9, "R", 84]] });
        beat(kit, c, 6, { [CASTANETS]: "x.mxm.x.x.mxmxmx", [CLAP]: "..o...o...o...x." });
        beat(kit, c + 24, 2, { [CASTANETS]: "xmxmxmxmxmxmxmxm", [CLAP]: "x.x.x.x.x.x.x.x.", [KICK]: "x...x...x...x..." });
        for (let bar = 0; bar < 8; bar += 2) bubbles(bub, c + bar * 4 + 2, { count: 5, spread: 2, low: 250, high: 900 });

        // A': bassoon and xylophone together, the trumpet joins the second half
        bassoon.at(a2).play(CRAB.MEL_A, { velocity: 104, gate: 0.55 });
        xylo.at(a2).play(CRAB.MEL_A, { velocity: 80, gate: 0.5, transpose: 12 });
        trumpet.at(a2 + 32).play(CRAB.MEL_A.split("|").slice(8).join("|"), { velocity: 84, gate: 0.6 });
        comp(guitar, pA2, { pattern: CRAB.RASGUEADO, center: 57, strum: 0.02 });
        figure(pizz, pA2, { pattern: ["R", "5", "8", "5"], rate: 0.5, base: 45, velocity: 74 });
        bassline(tuba, pA2, { pattern: TUBA });
        beat(kit, a2, 16, CRAB.CLAWS_PALMAS);
        bubbles(bub, a2 + 62, { count: 8, spread: 2 });
    });
    songs.push(song);
}

// =====================================================================================================
// 12. "King Crab" - the crustaceans' boss stages: the cove's tune, armoured. Brass on the tune over power chords
//     and fast flamenco strums, a driving bass, taiko and timpani, castanets at full speed, a choir in the middle.
//     Intro 4 | A 16 (brass) | B 8 (the king's march: trumpet and choir) | C 8 (claws and drums) | A' 16
// =====================================================================================================
{
    const song = new Song("theme_crab_boss", "Fish Fighters - King Crab", 156, { introBars: 4, bodyBars: 48,
        mix: { brass: 0, lead: -1, guitar: -5, choir: -5, bass: -4, drums: -2, fx: -9 } });
    const brass = song.part("Brass", 61, { group: "brass", volume: 112, pan: -0.1 });
    const xylo = song.part("Xylophone", 13, { group: "lead", volume: 88, pan: 0.3 });
    const trumpet = song.part("Trumpet", 56, { group: "lead", volume: 100, pan: 0.15 });
    const power = song.part("Power chords", 29, { group: "guitar", volume: 92, pan: -0.35 });
    const nylon = song.part("Nylon guitar", 24, { group: "guitar", volume: 90, pan: 0.4 });
    const choir = song.part("Choir", 52, { group: "choir", volume: 96 });
    const bass = song.part("Synth bass", 38, { group: "bass", volume: 98 });
    const kit = song.drums("Kit", 16, { group: "drums", volume: 96 });
    const timpani = song.part("Timpani", 47, { group: "drums", volume: 104 });
    const taiko = song.part("Taiko", 116, { group: "drums", volume: 98 });
    const bub = song.synth("Bubbles", "bubbles", { group: "fx" });
    const DRIVE8 = [[0, 0.45, "R", 100], [0.5, 0.45, "R", 80], [1, 0.45, "8", 86], [1.5, 0.45, "R", 96], [2, 0.45, "R", 80], [2.5, 0.45, "8", 86], [3, 0.45, "R", 96], [3.5, 0.45, "8", 84]];
    const KING = { [KICK]: "x...x...x...x.x.", [SNARE]: "....x.......x...", [HAT]: "m.o.m.o.m.o.m.o.", [CASTANETS]: "xmxmxmxmxmxmxmxm" };
    const HALF = { [KICK]: "x.....x.........", [SNARE]: "........x.......", [CASTANETS]: "x.mxm.x.x.mxm.x.", [CLAP]: "..o...x...o...x." };
    const timpaniOn = (prog, start, bars, every = 2) => {
        for (let bar = 0; bar < bars; bar += every) timpani.hit(start + bar * 4, above(chord(chordAt(prog, start + bar * 4).symbol).root, 33), 108, 1.5);
    };

    // Intro: a timpani roll and the castanets, strums, then the brass announces the tune
    roll(timpani, 0, 8, 33, 40, 112);
    beat(kit, 0, 2, { [CASTANETS]: "x.mxm.x.x.mxmxmx" });
    comp(nylon, progression("Am | E", 0), { pattern: [[0, 1.5, 100], [2, 1.5, 92]], center: 57, strum: 0.03 });
    [0, 4, 8, 10, 12, 13.5].forEach(t => taiko.hit(t, 45, 110, 1));
    brass.at(8).play(CRAB.MOTIF + " | E5:.5 D5:.5 C5:.5 B4:.5 G#4:1 E4:1", { velocity: 108, gate: 0.6, transpose: -12 });
    powerChords(power, progression("Am | E", 8), { velocity: 86 });
    bassline(bass, progression("Am | E", 8), { pattern: DRIVE8, base: 26 });
    beat(kit, 8, 1, { [KICK]: "x...x...x...x...", [CASTANETS]: "xmxmxmxmxmxmxmxm" });
    tomFill(kit, 14, 2);
    bubbles(bub, 1, { count: 10, spread: 5, low: 200, high: 900, size: [0.06, 0.14] });

    song.body(s => {
        const a = s, b = s + 64, c = s + 96, a2 = s + 128;
        const pA = progression(CRAB.A1 + " | " + CRAB.A2, a), pB = progression(CRAB.B, b), pC = progression(CRAB.C, c);
        const pA2 = progression(CRAB.A1 + " | " + CRAB.A2, a2);

        // A: the brass on the tune, power chords and fast strums, the driving bass, castanets in sixteenths
        brass.at(a).play(CRAB.MEL_A, { velocity: 106, gate: 0.6 });
        powerChords(power, pA, { velocity: 82 });
        comp(nylon, pA, { pattern: CRAB.RASGUEADO, center: 57, strum: 0.015 });
        bassline(bass, pA, { pattern: DRIVE8, base: 26 });
        beat(kit, a, 16, KING);
        for (let bar = 0; bar < 16; bar += 4) kit.hit(a + bar * 4, CRASH, 104, 1);
        timpaniOn(pA, a, 16);
        tomFill(kit, a + 62, 2);

        // B: the king's march - trumpet and choir, power chords held, half time
        trumpet.at(b).play(CRAB.MEL_B, { velocity: 104, gate: 0.85 });
        choir.at(b).play(CRAB.MEL_B, { velocity: 88, gate: 1, transpose: -12 });
        powerChords(power, pB, { rate: 4, gate: 0.95, velocity: 90 });
        comp(nylon, pB, { pattern: CRAB.RASGUEADO, center: 57, strum: 0.015 });
        bassline(bass, pB, { pattern: [[0, 1.9, "R", 100], [2, 0.45, "R", 80], [2.5, 0.45, "8", 80], [3, 0.45, "R", 84], [3.5, 0.45, "8", 84]], base: 26 });
        beat(kit, b, 8, HALF);
        kit.hit(b, CRASH, 100, 1);
        timpaniOn(pB, b, 8, 1);
        tomFill(kit, b + 30, 2);

        // C: the claws and the drums - castanets, palmas, taiko, the brass fragments, building back
        brass.at(c).play(CRAB.FRAGMENTS, { velocity: 104, gate: 0.6 });
        comp(nylon, pC, { pattern: [[0, 0.3, 100], [0.25, 0.3, 84], [0.5, 0.3, 88], [0.75, 0.3, 80], [2, 1.5, 92]], center: 57, strum: 0.015 });
        bassline(bass, pC, { pattern: [[0, 1.9, "R", 92], [2, 1.9, "R", 84]], base: 26 });
        beat(kit, c, 6, { [CASTANETS]: "x.mxm.x.x.mxmxmx", [CLAP]: "..o...x...o...x." });
        for (let bar = 0; bar < 8; bar++) [0, 1.5, 3].forEach(t => taiko.hit(c + bar * 4 + t, 45, 96 + bar * 3, 1));
        beat(kit, c + 24, 2, { [CASTANETS]: "xmxmxmxmxmxmxmxm", [KICK]: "x.x.x.x.x.x.x.x." });
        roll(kit, c + 28, 4, SNARE, 40, 120);
        roll(timpani, c + 24, 8, 33, 40, 118);
        bubbles(bub, c + 4, { count: 10, spread: 6, low: 220, high: 1100 });

        // A': everyone - brass and xylophone on the tune, the choir holds the chords
        brass.at(a2).play(CRAB.MEL_A, { velocity: 110, gate: 0.6 });
        xylo.at(a2).play(CRAB.MEL_A, { velocity: 84, gate: 0.5, transpose: 12 });
        sustain(choir, pA2, { center: 60, velocity: 58 });
        powerChords(power, pA2, { velocity: 88 });
        comp(nylon, pA2, { pattern: CRAB.RASGUEADO, center: 57, strum: 0.015 });
        bassline(bass, pA2, { pattern: DRIVE8, base: 26 });
        beat(kit, a2, 16, KING);
        for (let bar = 0; bar < 16; bar += 4) kit.hit(a2 + bar * 4, CRASH, 112, 1);
        timpaniOn(pA2, a2, 16, 1);
        tomFill(kit, a2 + 62, 2);
    });
    songs.push(song);
}

// =====================================================================================================
// 13. Battle start: the submarine dives (bubbles, a snare and timpani roll), then the motif's call to arms in two
//     rising "da-da-DUM"s, a turn on C7 and a big F major
// =====================================================================================================
{
    const song = new Song("jingle_battle_start", "Fish Fighters - Battle Start", 160, { kind: "jingle",
        mix: { lead: 0, brass: -1, strings: -6, bass: -5, drums: -3, bubbles: -6 } });
    const steel = song.part("Steel drums", 114, { group: "lead", volume: 112, fx: { echo: { beats: 0.5, feedback: 0.2, mix: 0.12 } } });
    const bells = song.part("Glockenspiel", 9, { group: "lead", volume: 80, pan: 0.3 });
    const brass = song.part("Brass", 61, { group: "brass", volume: 110, pan: -0.15 });
    const strings = song.part("Strings", 48, { group: "strings", volume: 96 });
    const tuba = song.part("Tuba", 58, { group: "bass", volume: 104 });
    const kit = song.drums("Kit", 0, { group: "drums", volume: 98 });
    const timpani = song.part("Timpani", 47, { group: "drums", volume: 104 });
    const bub = song.synth("Bubbles", "bubbles", { group: "bubbles" });

    const CALL = "r:1 C5:.25 C5:.25 F5:.5 A5:.25 A5:.25 C6:.5 D6:.25 C6:.25 A5:.25 C6:.25 F6:2";
    steel.play(CALL, { velocity: 108, gate: 0.9 });
    bells.play(CALL, { velocity: 72 });
    brass.play(CALL, { velocity: 110, gate: 0.85, transpose: -12 });
    let previous = null;
    for (const [symbol, at, length, velocity] of [["F", 1, 0.45, 96], ["F", 1.5, 0.45, 90], ["F", 2, 0.45, 96], ["F", 2.5, 0.45, 90], ["C7", 3, 0.9, 98], ["F", 4, 2, 110]]) {
        previous = voice(symbol, 57, previous);
        previous.forEach(n => brass.hit(at, n, velocity - 10, length));
    }
    voice("C7", 62).forEach(n => strings.hit(1, n, 70, 3));
    voice("Fadd9", 65).forEach(n => strings.hit(4, n, 96, 2.5));
    tuba.hit(1, pitch("F2"), 100, 0.4).hit(2, pitch("A2"), 96, 0.4).hit(3, pitch("C2"), 100, 0.9).hit(4, pitch("F1"), 114, 2.5);
    roll(kit, 0, 1, SNARE, 40, 110);
    roll(timpani, 0, 1, 41, 40, 104);
    kit.hit(1, KICK, 110).hit(1, SNARE, 100).hit(2, KICK, 110).hit(2, SNARE, 104);
    roll(kit, 3, 1, SNARE, 70, 112);
    kit.hit(4, CRASH, 122, 2).hit(4, KICK, 122);
    timpani.hit(4, pitch("F2"), 116, 2);
    bubbles(bub, 0, { count: 14, spread: 1, low: 250, high: 1400, velocity: 95 });
    bubbles(bub, 4, { count: 16, spread: 2, low: 400, high: 2000, velocity: 90 });
    songs.push(song);
}

// =====================================================================================================
// 14. Victory: the motif as a fanfare on steel drums and brass, a big F major, bubbles fizzing up
// =====================================================================================================
{
    const song = new Song("jingle_victory", "Fish Fighters - Victory", 150, { kind: "jingle", mix: { lead: 0, brass: -1, strings: -5, bass: -4, drums: -4, bubbles: -7 } });
    const steel = song.part("Steel drums", 114, { group: "lead", volume: 112, fx: { echo: { beats: 0.5, feedback: 0.25, mix: 0.15 } } });
    const bells = song.part("Glockenspiel", 9, { group: "lead", volume: 84, pan: 0.3 });
    const brass = song.part("Brass", 61, { group: "brass", volume: 108, pan: -0.15 });
    const strings = song.part("Strings", 48, { group: "strings", volume: 96 });
    const bass = song.part("Bass", 33, { group: "bass", volume: 104 });
    const kit = song.drums("Kit", 0, { group: "drums", volume: 96 });
    const perc = song.drums("Congas", 0, { group: "drums", volume: 90, pan: 0.25 });
    const bub = song.synth("Bubbles", "bubbles", { group: "bubbles" });
    const FANFARE = "C5:.333 F5:.333 A5:.334 C6:.75 A5:.25 C6:1 F6:3";
    steel.play(FANFARE, { velocity: 108 });
    bells.play(FANFARE, { velocity: 74 });
    brass.play(FANFARE, { velocity: 108, transpose: -12, gate: 0.9 });
    voice("F", 57).forEach(n => brass.hit(3, n, 100, 3));
    voice("C7", 60).forEach(n => strings.hit(1, n, 84, 2));
    voice("Fadd9", 64).forEach(n => strings.hit(3, n, 96, 3));
    bass.hit(0, pitch("C2"), 100, 0.9).hit(1, pitch("C2"), 100, 1.9).hit(3, pitch("F1"), 112, 3);
    roll(kit, 0, 1, SNARE, 70, 100);
    kit.hit(2, SNARE, 100).hit(2.5, SNARE, 106).hit(3, CRASH, 120, 3).hit(3, KICK, 120);
    perc.hit(2, CONGA_OPEN, 96).hit(2.25, CONGA_OPEN, 84).hit(2.5, CONGA_LOW, 100).hit(3, CONGA_LOW, 104);
    bubbles(bub, 3, { count: 16, spread: 2.5, low: 400, high: 2000, velocity: 90 });
    songs.push(song);
}

// =====================================================================================================
// 15. Defeat: the motif sinking - muted trumpet over a chromatic tuba, the last note droops,
//    big bubbles go "blub" as we sink, a soft F minor on the vibraphone
// =====================================================================================================
{
    const song = new Song("jingle_defeat", "Fish Fighters - Defeat", 96, { kind: "jingle", mix: { lead: 0, bass: -3, strings: -6, keys: -5, bubbles: -4 } });
    const trumpet = song.part("Muted trumpet", 59, { group: "lead", volume: 104 });
    const tuba = song.part("Tuba", 58, { group: "bass", volume: 100 });
    const strings = song.part("Strings", 49, { group: "strings", volume: 84 });
    const vibes = song.part("Vibraphone", 11, { group: "keys", volume: 90, fx: { chorus: { mix: 0.4 }, echo: { beats: 0.75, feedback: 0.35, mix: 0.25 } } });
    const bub = song.synth("Blub", "bubbles", { group: "bubbles" });
    trumpet.play("C5:.75 Ab4:.25 G4:1 Gb4:1", { velocity: 92 });
    trumpet.hit(3, pitch("F4"), 90, 2.8, -0.6);
    tuba.play("F2:1 E2:1 Eb2:1 Db2:1.5 C2:.5", { velocity: 92, gate: 0.9 });
    tuba.hit(5, pitch("F1"), 96, 2.5);
    let previous = null;
    for (const [symbol, at, length] of [["Fm", 0, 1], ["C/E", 1, 1], ["Ebm", 2, 1], ["Db", 3, 1.5], ["C7", 4.5, 0.5], ["Fm", 5, 3]]) {
        previous = voice(symbol, 58, previous);
        previous.forEach(n => strings.hit(at, n, 64, length));
    }
    voice("Fmadd9", 66).forEach(n => vibes.hit(5, n, 70, 3));
    bub.add(3.2, 320, 100, 0.17).add(3.7, 270, 96, 0.18).add(4.2, 225, 92, 0.2).add(4.8, 180, 90, 0.24);
    songs.push(song);
}

// ===== Write =====
fs.mkdirSync(OUT, { recursive: true });
for (const f of fs.readdirSync(OUT)) if (f.endsWith(".mid")) fs.rmSync(path.join(OUT, f));
fs.rmSync(path.join(OUT, "stems"), { recursive: true, force: true });
const loops = {};
for (const song of songs) {
    writeSong(song);
    const spb = 60 / song.tempo;
    const end = Math.max(...song.parts.flatMap(p => p instanceof Part ? p.events.map(e => e.time + e.length) : p.events.map(e => e[0])));
    // The game loops the second body copy: [loopStartSample, loopStartSample + loopLengthSample) at 48 kHz
    const introSamples = Math.round(song.intro * spb * RATE), lengthSamples = Math.round(song.length * spb * RATE);
    loops[song.name] = {
        title: song.title.replace("Fish Fighters - ", ""),
        kind: song.kind,
        tempo: song.tempo,
        sampleRate: RATE,
        lengthSeconds: +(end * spb).toFixed(3),
        loopStartSample: song.length ? introSamples + lengthSamples : null,
        loopLengthSample: song.length ? lengthSamples : null,
    };
    const l = loops[song.name];
    console.log(song.name.padEnd(17), (l.lengthSeconds.toFixed(1) + " s").padStart(8),
        song.length ? "loop from " + (l.loopStartSample / RATE).toFixed(3) + " s, " + (l.loopLengthSample / RATE).toFixed(3) + " s long" : "");
}
fs.writeFileSync(path.join(OUT, "loops.json"), JSON.stringify(loops, null, 4));

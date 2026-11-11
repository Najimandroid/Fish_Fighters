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
//   theme_boss       "Leviathan's Waltz" 3/4  a doom waltz: brass, choir, trombone riff, timpani, a music-box lullaby
//   theme_shanty     "Sunken Galleon" 126 BPM  12/8 sea-shanty jig: fiddle, accordion, tuba, stomps; a music box in the wreck
//   theme_jelly      "Jellyfish Lair" 124 BPM  dark, phrygian: an electric-eel lead, bowed glass, pulses, electric zaps
//   theme_jelly_boss "High Voltage Majesty" 138 BPM  dark electro: the eel crowned, 808/909, synth bass, a choir in the back
//   theme_crab       "Crab Cove"      132 BPM  a sideways tiptoe in A minor: bassoon, flamenco strums, castanets and claves
//   theme_crab_boss  "King Crab's Sideways Stomp" 120 BPM  heavy and jumpy: chugs, freezes, scuttles, taiko; the cove's pinches
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

// Beats in a bar of the song being written (3 for a waltz); set by each Song
let BAR = 4;

// "C5:.5 F5:.5 r:1 [F3,A3,C4]:4 | ..." -> [{ notes, beats }] (r = rest, | = bar line, checked to be BAR beats;
// lines without bar lines aren't checked, so a hemiola can cross them)
function parse(text, transpose = 0) {
    for (const bar of text.split("|")) {
        const beats = bar.trim().split(/\s+/).filter(Boolean).reduce((sum, t) => sum + length(t.split(":")[1]), 0);
        if (text.includes("|") && Math.abs(beats - BAR) > 0.01) throw new Error("Bar of " + beats + " beats: " + bar);
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
    // vibrato: { cents, hz, delay (seconds), minBeats } on the held notes of a solo line (rendered by sf2_render.js)
    constructor(name, program, { channel = 1, volume = 100, group = "main", pan = 0, gainDb = 0, fx = {}, vibrato = null } = {}) {
        Object.assign(this, { name, program, channel, volume, group, pan, gainDb, fx, vibrato, events: [], cursor: 0 });
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
    // barBeats: beats per bar (3 for a waltz); key: its tonic (the battle start jingle and the boss stinger match it)
    constructor(name, title, tempo, { introBars = 0, bodyBars = 0, mix = {}, kind = "stage", barBeats = 4, key = null } = {}) {
        Object.assign(this, { name, title, tempo, barBeats, key, intro: introBars * barBeats, length: bodyBars * barBeats, mix, kind, parts: [] });
        BAR = barBeats;
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
        for (const name of names) { out.push({ symbol: name, beat, length: BAR / names.length }); beat += BAR / names.length; }
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
const CHINA = 52, CRASH2 = 57, HIGH_TOM = 50, HI_MID_TOM = 48, LOW_MID_TOM = 47;

// One character per step (16 per bar, or 12 in 12/8): X x m o g = accent to ghost, . = rest.
// Patterns longer than a bar span several bars.
const LEVELS = { X: 122, x: 100, m: 86, o: 70, g: 48 };
function beat(part, start, barsCount, grid, stepsPerBar = 16) {
    for (const [note, text] of Object.entries(grid)) {
        const steps = text.replace(/[\s|]/g, "");
        for (let s = 0; s < barsCount * stepsPerBar; s++) {
            const velocity = LEVELS[steps[s % steps.length]];
            if (velocity) part.hit(start + s * BAR / stepsPerBar, Number(note), velocity, 0.2);
        }
    }
}
// Steps of a pattern: steps(32, [0, 6, 12], "o") -> "o.....o.....o..."
const steps = (length, positions, level = "o") => Array.from({ length }, (_, i) => positions.includes(i) ? level : ".").join("");

function roll(part, start, beats, note = SNARE, from = 50, to = 110, step = 0.25) {
    const count = Math.round(beats / step);
    for (let i = 0; i < count; i++) part.hit(start + i * step, note, from + (to - from) * i / Math.max(1, count - 1), 0.1);
}
// Repeats each written note as 16ths (tremolo picking), the first one accented
function tremolo(part, text, { velocity = 90, gate = 0.45, rate = 0.25, accent = 10, transpose = 0 } = {}) {
    for (const { notes, beats } of parse(text, transpose)) {
        for (let t = 0, i = 0; t < beats - 1e-6; t += rate, i++)
            for (const n of notes)
                part.hit(part.cursor + t, n, velocity + (i === 0 ? accent : -4 + Math.round((hash(part.name, (part.cursor + t - ORIGIN).toFixed(3), n) - 0.5) * 12)), rate * gate);
        part.cursor += beats;
    }
    return part;
}
// Tango "arrastre": a chromatic 16th-note drag up into a target note on a downbeat
function drag(part, beat, target, { count = 4, step = 0.25, from = 70, to = 100 } = {}) {
    for (let i = 0; i < count; i++) part.hit(beat - (count - i) * step, target - (count - i), from + (to - from) * i / (count - 1), step * 0.9);
}
// Phrase shaping: the highest note of [start, end) gets +peak, the last note +ending
function shape(part, start, end, { peak = 10, ending = -8 } = {}) {
    const events = part.events.filter(e => e.time >= start - 1e-6 && e.time < end - 1e-6);
    if (!events.length) return;
    const top = events.reduce((a, b) => (b.note > a.note ? b : a));
    top.velocity = Math.min(127, top.velocity + peak);
    const last = events.reduce((a, b) => (b.time > a.time ? b : a));
    last.velocity = Math.max(1, last.velocity + ending);
}
// Stop-time gag: silences the parts in [from, to) (notes crossing into it are cut at from)
function stopTime(parts, from, to) {
    for (const p of parts) {
        if (p instanceof Synth) { p.events = p.events.filter(e => e[0] < from - 1e-6 || e[0] >= to - 1e-6); continue; }
        p.events = p.events.filter(e => e.time < from - 1e-6 || e.time >= to - 1e-6);
        for (const e of p.events) if (e.time < from && e.time + e.length > from) e.length = from - e.time;
    }
}
// Bend an already-written note (droops, slides): the notes starting at that beat
const bendAt = (part, beat, semitones) => part.events.filter(e => Math.abs(e.time - beat) < 1e-6).forEach(e => (e.bend = semitones));
// Timpani notes kept in their range (D2 to D3)
const foldTimp = n => { while (n > 50) n -= 12; while (n < 38) n += 12; return n; };
// The theremin (a render_stems.js synth): a line in the note format, gliding between legato notes;
// swoopFirst: the phrase's first note swoops up from that many semitones below
function theremin(synth, start, text, { velocity = 96, glide = 0.07, swoopFirst = 0 } = {}) {
    let t = start, first = true;
    for (const { notes, beats } of parse(text)) {
        for (const n of notes) synth.add(t, +(440 * Math.pow(2, (n - 69) / 12)).toFixed(3), beats, velocity, glide, first ? swoopFirst : 0);
        if (notes.length) first = false;
        t += beats;
    }
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
        { tick: 0, order: 0, bytes: [0xff, 0x58, 0x04, song.barBeats, 2, 24, 8] },
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
        if (part.vibrato) common.vibrato = part.vibrato;
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
    const song = new Song("theme_default", "Fish Fighters - Reef Rumble", 152, { key: "F", introBars: 4, bodyBars: 50, mix: { lead: 0, brass: -2, keys: -6, bass: -4, drums: -3, percussion: -7, bubbles: -9 } });
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
    const song = new Song("theme_calm", "Fish Fighters - Kelp Garden", 100, { key: "Eb", introBars: 2, bodyBars: 40, mix: { lead: 0, kalimba: -5, harp: -5, keys: -5, pad: -9, bass: -4, drums: -7, bubbles: -9 } });
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
    const song = new Song("theme_serious", "Fish Fighters - Deep Current", 138, { key: "D", introBars: 4, bodyBars: 48, mix: { horns: 0, strings: -2, woodwind: 0, ostinato: -4, bass: -4, drums: -3, pad: -9, fx: -7 } });
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
    const song = new Song("theme_stressful", "Fish Fighters - Pressure Alarm", 172, { key: "C", introBars: 2, bodyBars: 48, mix: { brass: 0, lead: -1, strings: -4, bass: -4, drums: -3, fx: -7 } });
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
// 5. "Leviathan's Waltz" - boss stages. The sea monster has to dance a waltz: a heavy one-in-a-bar 3/4 in D minor
//    (tuba, low piano, concert bass drum, timpani on every downbeat), the motif as a bassoon riff with a hiccup,
//    a hemiola "fight" where the waltz breaks into big two-beat strides, and a music-box lullaby in D major
//    sung to the monster, cut by silence, a hiccup and the slam.
//    Intro 8 | A 24 | B 16 (hemiola fight) | C 8 (lullaby) | A' 24  (3/4 bars)
// =====================================================================================================
{
    const song = new Song("theme_boss", "Fish Fighters - Leviathan's Waltz", 162, { barBeats: 3, introBars: 8, bodyBars: 72, key: "D",
        mix: { lead: 0, brass: -1, bass: -2, strings: -7, keys: -6, drums: -2, fx: -8 } });
    const bassoon = song.part("Bassoon", 70, { group: "lead", volume: 110, pan: -0.15 });
    const trumpet = song.part("Trumpet", 56, { group: "lead", volume: 104, pan: 0.1, vibrato: { cents: 18, hz: 5.5, delay: 0.25, minBeats: 1 } });
    const glock = song.part("Glockenspiel", 9, { group: "lead", volume: 80, pan: 0.35 });
    const brass = song.part("Brass", 61, { group: "brass", volume: 110, pan: -0.1 });
    const trombones = song.part("Trombones", 57, { group: "brass", volume: 106, pan: 0.2 });
    const horns = song.part("Horns", 60, { group: "brass", volume: 100, pan: -0.3 });
    const tuba = song.part("Tuba", 58, { group: "bass", volume: 106 });
    const piano = song.part("Low piano", 0, { group: "bass", volume: 100 });
    const pizz = song.part("Pizzicato", 45, { group: "strings", volume: 92, pan: 0.4 });
    const trem = song.part("Tremolo strings", 44, { group: "strings", volume: 92, pan: -0.35 });
    const musicBox = song.part("Music box", 10, { group: "keys", volume: 96, fx: { echo: { beats: 1, feedback: 0.4, mix: 0.3 } } });
    const kit = song.drums("Orchestral kit", 48, { group: "drums", volume: 100 });
    const timpani = song.part("Timpani", 47, { group: "drums", volume: 106 });
    const taiko = song.part("Taiko", 116, { group: "drums", volume: 100 });
    const whale = song.part("Leviathan voice", 91, { group: "fx", volume: 104, fx: { lowpass: 1400, echo: { beats: 1.5, feedback: 0.4, mix: 0.35 } } });
    const ping = song.synth("Sonar", "ping", { group: "fx", fx: { echo: { beats: 1.5, feedback: 0.45, mix: 0.4 } } });
    const bub = song.synth("Bubbles", "bubbles", { group: "fx" });

    const A = "Dm | Dm | Bb7 | A7 | Dm | Dm | Bb7 | A7 | Dm | Dm | Gm | C7 | F | A7 | Eb | Dm | Dm | Dm | C7 | F | A7 | Dm | Gm | A7";
    const B = "Bb | F/A | Gm | Gm | Eb | Eb | A7sus4 | A7 | Bb | F/A | Gm | Eb | Edim | Edim | A7sus4 | A7";
    const C = "D | D | G | D | A7 | D | Em A7 | D";
    const bars = (text, from, to) => text.split("|").slice(from, to).join("|");

    const RIFF = "A2:1 D3:1 F3:1 | E3:.5 D3:.5 A3:2 | Bb3:1 A3:1 Ab3:1 | G3:1 r:.5 C#3:.5 D3:1";
    const TUNE_A1 = "A4:1 D5:1 F5:1 | E5:1.5 D5:.5 A5:1 | Bb5:2 A5:1 | G5:1 F5:1 E5:1 | " +
                    "F5:1 D5:1 A4:1 | C#5:1.5 D5:.5 E5:1 | F5:1 G5:1 Eb5:1 | D5:3";
    const TUNE_A2 = "A4:1 D5:1 F5:1 | E5:.5 D5:.5 A5:2 | C6:2 Bb5:1 | A5:1 G5:1 F5:1 | " +
                    "G5:1 E5:1 C#5:1 | D5:1 F5:1 A5:1 | Bb5:1 A5:1 G5:1 | A5:3";
    // Hemiola half notes: no bar lines, they cross the 3/4 bars
    const B_MEL = "F5:2 D5:2 C5:2 Bb4:2 D5:2 G5:2 G5:2 Bb5:2 G5:2 A5:2 G5:1 E5:3 " +
                  "F5:2 D5:2 C5:2 D5:2 G5:2 Bb5:2 Bb5:2 G5:2 E5:2 D5:2 E5:1 C#5:3";
    const LULLABY = "A5:1 D6:1 F#6:1 | E6:.5 D6:.5 A6:2 | B6:2 A6:1 | F#6:3 | " +
                    "G6:1 F#6:1 E6:1 | D6:1 F#6:1 A5:1 | B5:1.5 C#6:.5 E6:1 | D6:3";

    const OOM = [[0, 0.9, "R", 104], [3, 0.9, "-5", 96]];          // tuba, every 2 bars: the root, then the fifth below
    const LOWP = [[0, 2.5, "R", 96], [0, 2.5, "8", 88]];           // piano octaves, every 2 bars
    const PAH = [[0.93, 0.35, 80], [2.04, 0.35, 74]];              // Viennese: beat 2 early, beat 3 late
    const PIZZ = [[0.93, 0.25, 70], [2.04, 0.25, 66]];
    const DOOM = { [KICK]: "X...........", [SNARE]: "....m...o...", [RIDE]: "o...g...g..." };
    const DOOM_FULL = { [KICK]: "X.......o...", [SNARE]: "....x...m..g", [RIDE]: "m.g.o.g.o.g." };
    const HEMIOLA = { [KICK]: "X.......x.......x.......", [SNARE]: "....m.......m.......m...", [RIDE]: "m.o.m.o.m.o.m.o.m.o.m.o." };
    const timpaniRoot = (prog, beat) => foldTimp(above(chord(chordAt(prog, beat).symbol).root, 38));

    // The riff with its expression: the third bar swells, the C# is a short loud hiccup
    const playRiff = (part, start, { velocity, gate = 0.8, transpose = 0, times = 1 }) => {
        for (let k = 0; k < times; k++) {
            const from = start + k * 12;
            part.at(from).play(RIFF, { velocity, gate, transpose });
            part.events.filter(e => e.time >= from + 6 - 1e-6 && e.time < from + 9 - 1e-6)
                .forEach(e => { e.velocity = Math.min(127, e.velocity + Math.round((e.time - from - 6) * 4)); });
            part.events.filter(e => Math.abs(e.time - (from + 10.5)) < 1e-6)
                .forEach(e => { e.velocity = Math.min(127, e.velocity + 14); e.length = 0.15; });
        }
    };

    // Intro: the sonar in the dark, the leviathan's voice, a ghost of the motif on the music box, then the riff
    ping.add(0, 980, 110);
    whale.hit(0, pitch("A3"), 100, 9, -5).hit(12, pitch("D4"), 94, 9, -7);
    for (let bar = 0; bar < 4; bar++) tuba.hit(bar * 3, pitch("D2"), 80, 0.9);
    timpani.hit(0, pitch("D2"), 100, 1.5).hit(6, pitch("D2"), 96, 1.5);
    musicBox.at(6).play("A5:1 D6:1 F6:1 | E6:.5 D6:.5 A6:2", { velocity: 58, gate: 1 });
    playRiff(bassoon, 12, { velocity: 92 });
    bassoon.events.filter(e => e.time >= 12).forEach(e => { e.velocity = Math.min(127, e.velocity + Math.round((e.time - 12))); });
    playRiff(piano, 12, { velocity: 88, transpose: -12 });
    beat(kit, 12, 4, DOOM, 12);
    comp(trombones, progression("Bb7 | A7", 18), { pattern: PAH, cycle: 3, center: 53 });
    roll(kit, 21, 3, SNARE, 40, 112);
    bubbles(bub, 1, { count: 14, spread: 8, low: 200, high: 900, size: [0.06, 0.16] });

    song.body(s => {
        const a = s, b = s + 72, c = s + 120, a2 = s + 144;
        const pA = progression(A, a), pB = progression(B, b), pC = progression(C, c), pA2 = progression(A, a2);

        // A: the riff twice, then the trumpet's tune, then the brass; oom-pah-pah all along
        playRiff(bassoon, a, { velocity: 100, times: 2 });
        playRiff(piano, a, { velocity: 92, transpose: -12, times: 2 });
        bassline(piano, progression(bars(A, 8, 24), a + 24), { pattern: LOWP, cycle: 6, base: 26 });
        bassline(tuba, pA, { pattern: OOM, cycle: 6, base: 31 });
        comp(pizz, pA, { pattern: PIZZ, cycle: 3, center: 60 });
        comp(trombones, progression(bars(A, 4, 24), a + 12), { pattern: PAH, cycle: 3, center: 53 });
        stab(brass, pA, a + 18, [0], { velocity: 108, length: 0.35, center: 60 });
        stab(brass, pA, a + 21, [0], { velocity: 108, length: 0.35, center: 60 });
        trombones.hit(a + 21, 45, 96, 2, -2);                                          // wah
        trumpet.at(a + 24).play(TUNE_A1, { velocity: 96, gate: 0.9 });
        horns.at(a + 24).play(TUNE_A1, { velocity: 72, gate: 0.95, transpose: -12 });
        glock.at(a + 36).play(bars(TUNE_A1, 4, 8), { velocity: 52, gate: 0.6, transpose: 24 });
        brass.at(a + 48).play(TUNE_A2, { velocity: 100, gate: 0.85 });
        trumpet.at(a + 48).play(TUNE_A2, { velocity: 92, gate: 0.9 });
        trombones.hit(a + 69, 45, 100, 2, -2);                                         // wah
        for (const start of [a + 24, a + 36, a + 48, a + 60]) shape(trumpet, start, start + 12, { peak: 12, ending: -10 });
        for (const start of [a + 48, a + 60]) shape(brass, start, start + 12, { peak: 12, ending: -10 });
        beat(kit, a, 24, DOOM, 12);
        [0, 24, 48].forEach(t => kit.hit(a + t, CRASH, 104, 1));
        [0, 4, 8, 12, 16, 20].forEach(bar => taiko.hit(a + bar * 3, 45, 110, 1));
        for (let bar = 0; bar < 24; bar += 2) timpani.hit(a + bar * 3, timpaniRoot(pA, a + bar * 3), 100, 1.2);

        // B: the fight - the waltz breaks into two-beat strides under the trumpet and horns
        trumpet.at(b).play(B_MEL, { velocity: 100, gate: 0.95 });
        horns.at(b).play(B_MEL, { velocity: 88, gate: 1, transpose: -12 });
        pB.forEach((entry, bar) => voice(entry.symbol, 64).forEach(n => trem.hit(entry.beat, n, 64 + 2 * bar, entry.length * 0.98)));
        bassline(tuba, pB, { pattern: [[0, 1.8, "R", 104], [2, 1.8, "R", 96], [4, 1.8, "-5", 100]], cycle: 6, base: 31 });
        comp(brass, pB, { pattern: [[0, 0.4, 110], [2, 0.4, 100], [4, 0.4, 104]], cycle: 6, center: 60 });
        beat(kit, b, 16, HEMIOLA, 12);
        for (let group = 0; group < 8; group++) [[0, 116], [2, 100], [4, 106]].forEach(([t, v]) => taiko.hit(b + group * 6 + t, 45, v, 1));
        roll(timpani, b + 42, 3, foldTimp(pitch("A2")), 50, 118);
        stab(brass, pB, b + 45, [0], { velocity: 120, length: 2.5, center: 60 });
        trombones.hit(b + 45, pitch("A2"), 116, 2.5);
        tuba.hit(b + 45, pitch("A1"), 116, 2.5);
        kit.hit(b + 45, CRASH, 118, 2);
        timpani.hit(b + 45, foldTimp(pitch("A2")), 120, 2);

        // C: a lullaby for the monster (the motif in D major), its voice, a timpani heartbeat; silence, a hiccup...
        musicBox.at(c).play(LULLABY, { velocity: 84, gate: 1 });
        tuba.hit(c, pitch("D2"), 52, 20);
        whale.hit(c, pitch("A3"), 100, 9, -5).hit(c + 12, pitch("D4"), 94, 9, -7);
        ping.add(c, 980, 100).add(c + 12, 980, 92);
        for (let bar = 0; bar < 7; bar++) timpani.hit(c + bar * 3, pitch("D2"), 66, 1);
        bubbles(bub, c + 1, { count: 10, spread: 18, low: 180, high: 700, size: [0.1, 0.2] });
        bassoon.hit(c + 22.5, pitch("C#3"), 104, 0.12);

        // A': ...the slam. Everyone on the riff, then the tunes; the last bar stops dead and hiccups into the loop
        kit.hit(a2, CRASH, 124, 2).hit(a2, CHINA, 116, 2).hit(a2, KICK, 124);
        timpani.hit(a2, pitch("D2"), 124, 2);
        taiko.hit(a2, 45, 124, 1);
        playRiff(bassoon, a2, { velocity: 104, times: 2 });
        playRiff(trombones, a2, { velocity: 104, times: 2 });
        playRiff(tuba, a2, { velocity: 100, gate: 0.85, transpose: -12, times: 2 });
        playRiff(piano, a2, { velocity: 96, transpose: -12, times: 2 });
        [2, 3, 6, 7].forEach(bar => stab(brass, pA2, a2 + bar * 3, [0], { velocity: 108, length: 0.35, center: 60 }));
        brass.at(a2 + 24).play(TUNE_A1, { velocity: 108, gate: 0.85 });
        trumpet.at(a2 + 24).play(TUNE_A1, { velocity: 100, gate: 0.9 });
        glock.at(a2 + 24).play(TUNE_A1, { velocity: 52, gate: 0.6, transpose: 24 });
        sustain(trem, progression(bars(A, 8, 16), a2 + 24), { center: 64, velocity: 76 });
        brass.at(a2 + 48).play(TUNE_A2, { velocity: 108, gate: 0.85 });
        trumpet.at(a2 + 48).play(TUNE_A2, { velocity: 100, gate: 0.9 });
        for (const start of [a2 + 24, a2 + 36, a2 + 48, a2 + 60]) { shape(trumpet, start, start + 12, { peak: 12, ending: -10 }); shape(brass, start, start + 12, { peak: 12, ending: -10 }); }
        const pLate = progression(bars(A, 8, 24), a2 + 24);
        bassline(tuba, pLate, { pattern: OOM, cycle: 6, base: 31 });
        bassline(piano, pLate, { pattern: LOWP, cycle: 6, base: 26 });
        comp(trombones, pLate, { pattern: PAH, cycle: 3, center: 53 });
        comp(pizz, pLate, { pattern: PIZZ, cycle: 3, center: 60 });
        beat(kit, a2, 24, DOOM_FULL, 12);
        [0, 24, 48].forEach(t => kit.hit(a2 + t, CRASH, 112, 1));
        for (let bar = 16; bar < 23; bar++) { taiko.hit(a2 + bar * 3, 45, 112, 1); timpani.hit(a2 + bar * 3, timpaniRoot(pA2, a2 + bar * 3), 104, 1.2); }
        stab(brass, pA2, a2 + 69, [0], { velocity: 120, length: 0.5, center: 60 });
        stopTime([bassoon, trumpet, glock, brass, trombones, horns, tuba, piano, pizz, trem, musicBox, kit, timpani, taiko], a2 + 70, a2 + 72);
        bassoon.at(a2 + 70.5).play("C#3:.25 D3:.25", { velocity: 110, gate: 0.6 });
    });
    songs.push(song);
}

// =====================================================================================================
// 6. "Fish Fighters!" - the title screen, made to make you press start. Ska-punk energy: brass and steel
//    drums shout the motif over power chords, a skanking guitar, organ stabs and a punk beat.
//    Intro 4 | A 16 | B 8 (the build) | A' 8 (the big finish) | turnaround 2
// =====================================================================================================
{
    const song = new Song("theme_title", "Fish Fighters - Fish Fighters!", 168, { key: "F", kind: "menu", introBars: 4, bodyBars: 34,
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
    const song = new Song("theme_base", "Fish Fighters - Fish Tank", 112, { key: "Bb", kind: "menu", introBars: 2, bodyBars: 40,
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
    const song = new Song("theme_shanty", "Fish Fighters - Sunken Galleon", 126, { key: "D", introBars: 2, bodyBars: 48,
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
    const song = new Song("theme_jelly", "Fish Fighters - Jellyfish Lair", 124, { key: "E", introBars: 4, bodyBars: 48,
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
// 10. "High Voltage Majesty" - the jellyfish king's boss stages. Dark electro, nothing of a band: the lair's eel
//     (a saw lead) crowned, over a pumping synth bass, an 808/909 kit and 16th arpeggios; a concert choir always
//     in the back, holding the dark chords like a court in a cathedral, and singing the king's anthem in B.
//     Synth brass for the royal weight, electric zaps everywhere, one blackout gag (the power cuts, a giant zap).
//     E phrygian (the F a half step over the tonic), a tritone in the cadences.
//     Intro 4 | A 16 (the eel's anthem) | B 8 (the court: the choir sings) | C 8 (blackout and power surge) | A' 16
// =====================================================================================================
{
    const song = new Song("theme_jelly_boss", "Fish Fighters - High Voltage Majesty", 138, { introBars: 4, bodyBars: 48, key: "E",
        mix: { lead: 0, choir: -3, arp: -9, pad: -10, brass: -5, bass: -2, drums: -2, fx: -6 } });
    const eel = song.part("Eel lead", 81, { group: "lead", volume: 104, pan: -0.05, fx: { lowpass: 3400, echo: { beats: 0.75, feedback: 0.25, mix: 0.18 } },
        vibrato: { cents: 18, hz: 5.5, delay: 0.25, minBeats: 1 } });
    const square = song.part("Square lead", 80, { group: "lead", volume: 82, pan: 0.25, fx: { lowpass: 2600 } });
    const choir = song.part("Concert choir", 52, { group: "choir", volume: 100 });
    const voices = song.part("Space voices", 91, { group: "choir", volume: 86, pan: -0.25, fx: { lowpass: 2400, echo: { beats: 1.5, feedback: 0.35, mix: 0.3 } } });
    const arpSynth = song.part("Arpeggio", 90, { group: "arp", volume: 84, pan: 0.35, fx: { lowpass: 2800, echo: { beats: 0.75, feedback: 0.35, mix: 0.25 } } });
    const sweep = song.part("Sweep pad", 95, { group: "pad", volume: 84 });
    const brass = song.part("Synth brass", 62, { group: "brass", volume: 104, pan: -0.15 });
    const bass = song.part("Synth bass", 39, { group: "bass", volume: 110 });
    const kit = song.drums("808/909", 25, { group: "drums", volume: 100 });
    const zap = song.synth("Zaps", "zaps", { group: "fx", fx: { echo: { beats: 0.75, feedback: 0.3, mix: 0.22 } } });
    const bub = song.synth("Bubbles", "bubbles", { group: "fx" });

    const A = "Em | Fmaj7/E | Em | Dm/F | Em | Am | B7sus4 B7 | Em | Em | Fmaj7/E | Em | C | Am | Dm/F B7 | Em B7 | Em";
    const B = "Cmaj7 | B7 | Em | Em/D | Cmaj7 | Am | B7sus4 | B7";
    const C = "Em | Em | F/E | F/E | Em | Em | B7 | B7";

    // The eel's anthem: the lair's "B E G F# E" made royal, phrygian F, a tritone (A#) before each cadence
    const ANTHEM = "E5:.75 E5:.25 G5:.5 F5:1 E5:.5 B4:1 | C5:.5 B4:.5 A4:.5 B4:.5 E4:2 | E5:.75 E5:.25 G5:.5 A5:1 G5:.5 F5:1 | G5:.5 F5:.5 E5:.5 D5:.5 F5:2 | " +
                   "E5:.75 E5:.25 G5:.5 F5:1 E5:.5 B5:1 | C6:1.5 B5:.5 A5:.5 G5:.5 F5:1 | E5:.5 F5:.5 G5:.5 B5:.5 A#5:1 B5:1 | E5:3 r:1 | " +
                   "B5:.75 B5:.25 D6:.5 C6:1 B5:.5 F5:1 | G5:.5 F5:.5 E5:.5 F5:.5 B4:2 | E5:.75 E5:.25 G5:.5 B5:1 C6:.5 D6:1 | E6:1.5 D6:.5 C6:1 B5:1 | " +
                   "C6:.5 B5:.5 A5:.5 G5:.5 F5:1 E5:1 | F5:1 G5:.5 F5:.5 E5:1 D#5:1 | E5:.5 G5:.5 B5:.5 E6:.5 F6:1 D#6:1 | E6:2 r:2";
    // The court: the choir sings it slow, the low voices of a cathedral
    const COURT = "E5:2 G5:2 | F#5:2 D#5:2 | E5:3 B4:1 | D5:4 | E5:2 G5:2 | A5:2 C6:2 | B5:4 | B5:2 A#5:2";
    const FRAGMENTS = "E5:.75 E5:.25 G5:.5 F5:.5 r:2 | r:4 | F5:.75 F5:.25 A5:.5 G5:.5 r:2 | r:4 | " +
                      "E5:.75 E5:.25 G5:.5 F5:1 E5:.5 B4:1 | r:4 | B4:.5 D#5:.5 F#5:.5 A5:.5 A#5:1 B5:1 | r:4";

    // A galloping 16th bass (the octave pumps), the 808 kick under its accents
    const PUMP = [[0, 0.2, "R", 112], [0.25, 0.2, "R", 70], [0.5, 0.2, "8", 92], [0.75, 0.2, "R", 74],
                  [1, 0.2, "R", 104], [1.25, 0.2, "R", 70], [1.5, 0.2, "8", 90], [1.75, 0.2, "5", 78],
                  [2, 0.2, "R", 110], [2.25, 0.2, "R", 70], [2.5, 0.2, "8", 92], [2.75, 0.2, "R", 74],
                  [3, 0.2, "R", 102], [3.25, 0.2, "8", 84], [3.5, 0.2, "5", 88], [3.75, 0.2, "R", 80]];
    const HALF = [[0, 1.9, "R", 104], [2, 0.45, "R", 80], [2.5, 0.45, "8", 84], [3, 0.9, "R", 92]];
    const ELECTRO = { [KICK]: "X..x..X...x..x..", [CLAP]: "....X.......X...", [HAT]: "g.o.g.o.g.o.g.og" };
    const FOUR = { [KICK]: "X...X...X...X...", [CLAP]: "....X.......X...", [HAT]: "g.g.g.g.g.g.g.g.", [OPEN_HAT]: "..o...o...o...o." };
    const COURT_BEAT = { [KICK]: "X.........x.....", [SNARE]: "........X.......", [HAT]: "g...g...g...g..." };
    const ARP16 = { pattern: [0, 1, 2, 1, 3, 2, 1, 2], rate: 0.25, center: 64, velocity: 60, accent: 10, gate: 0.7 };

    const royalStabs = (prog, start, bars, offsets, velocity = 100) => {
        for (const bar of bars) for (const o of offsets) {
            const t = start + bar * 4 + o;
            voice(chordAt(prog, t).symbol, 60).forEach(n => brass.hit(t, n, velocity, 0.35));
        }
    };

    // Intro: the choir rises out of the dark, the bass wakes up in 8ths, zaps crackle, the brass crowns the king
    const pI = progression("Em | Fmaj7/E | Em | B7", 0);
    sustain(choir, pI, { center: 52, velocity: 70 });
    sustain(sweep, pI, { center: 60, velocity: 56 });
    bassline(bass, progression("Fmaj7/E | Em | B7", 4), { pattern: [[0, 0.4, "R", 96], [0.5, 0.4, "R", 72], [1, 0.4, "R", 90], [1.5, 0.4, "R", 72],
                                                                    [2, 0.4, "R", 94], [2.5, 0.4, "R", 72], [3, 0.4, "8", 90], [3.5, 0.4, "R", 76]] });
    zaps(zap, 1, { count: 2, spread: 1.5, velocity: 100 });
    zaps(zap, 6, { count: 3, spread: 2, velocity: 104 });
    bubbles(bub, 0, { count: 10, spread: 6, low: 200, high: 900, size: [0.06, 0.14] });
    brass.at(8).play("E4:.75 E4:.25 F4:1 E4:2 | r:4", { velocity: 104, gate: 0.85 });
    [12, 13, 14].forEach(t => voice("B7", 60).forEach(n => brass.hit(t, n, 100, 0.3)));
    beat(kit, 8, 1, { [KICK]: "X.......X.......", [HAT]: "g.g.g.g.g.g.g.g." });
    beat(kit, 12, 1, { [KICK]: "X...X...X...X.X.", [CLAP]: "............XXXX", [HAT]: "gggggggggggggggg" });

    song.body(s => {
        const a = s, b = s + 64, c = s + 96, a2 = s + 128;
        const pA = progression(A, a), pB = progression(B, b), pC = progression(C, c), pA2 = progression(A, a2);

        // A: the eel's anthem over the pump and the electro beat; the choir in the back, synth brass on the cadences
        eel.at(a).play(ANTHEM, { velocity: 100, gate: 0.88, transpose: -12 });
        for (let p = 0; p < 4; p++) shape(eel, a + p * 16, a + p * 16 + 16, { peak: 10, ending: -6 });
        sustain(choir, pA, { center: 55, velocity: 60 });
        bassline(bass, pA, { pattern: PUMP });
        arp(arpSynth, pA, ARP16);
        beat(kit, a, 16, ELECTRO);
        kit.hit(a, CRASH, 104, 1).hit(a + 32, CRASH, 100, 1);
        royalStabs(pA, a, [6, 14], [0, 0.75, 1.5]);
        royalStabs(pA, a, [7, 15], [0]);
        for (const bar of [3, 7, 11, 15]) zaps(zap, a + bar * 4 + 2.5, { count: 2, spread: 1, velocity: 100 });

        // B: the court - half time, the choir sings the anthem slowly, synth brass marches, voices echo far away
        choir.at(b).play(COURT, { velocity: 100, gate: 0.98, transpose: -12 });
        sustain(voices, pB, { center: 64, velocity: 66 });
        sustain(sweep, pB, { center: 57, velocity: 54 });
        comp(brass, pB, { pattern: [[0, 0.5, 96], [1.5, 0.3, 84], [2, 0.5, 92], [3.5, 0.3, 84]], center: 60 });
        bassline(bass, pB, { pattern: HALF });
        arp(arpSynth, pB, { ...ARP16, rate: 0.5, velocity: 54 });
        beat(kit, b, 8, COURT_BEAT);
        kit.hit(b, CRASH, 100, 1);
        zaps(zap, b + 14, { count: 2, spread: 1, velocity: 96 });
        zaps(zap, b + 30, { count: 3, spread: 1.5, velocity: 100 });

        // C: blackout and power surge - the bass alone with the kick, the eel's fragments, zaps growing;
        // the power cuts (a beat of silence and one giant zap), then everything charges back up
        eel.at(c).play(FRAGMENTS, { velocity: 104, gate: 0.85, transpose: -12 });
        bassline(bass, pC, { pattern: PUMP });
        sustain(voices, pC, { center: 57, velocity: 58 });
        beat(kit, c, 4, { [KICK]: "X...X...X...X..." });
        beat(kit, c + 16, 2, { [KICK]: "X...X...X...X...", [HAT]: "g.g.g.g.g.g.g.g." });
        beat(kit, c + 24, 2, { [KICK]: "X.X.X.X.X.X.X.X.", [HAT]: "gggggggggggggggg" });
        roll(kit, c + 28, 4, CLAP, 40, 118);
        for (let bar = 0; bar < 8; bar++) zaps(zap, c + bar * 4 + (bar % 2 ? 2.5 : 0.5), { count: bar < 4 ? 2 : 4, spread: 1.5, velocity: 92 + 3 * bar });
        arp(arpSynth, progression("Em | Em | B7 | B7", c + 16), { ...ARP16, velocity: 50 });
        bubbles(bub, c + 4, { count: 10, spread: 6, low: 200, high: 1100 });
        stopTime([eel, bass, kit, arpSynth, voices], c + 12, c + 13);
        zaps(zap, c + 12, { count: 1, velocity: 120, duration: [0.6, 0.7] });

        // A': everyone - the square lead doubles the eel an octave up, the choir and the voices hold the chords,
        // four on the floor, the brass on every cadence
        eel.at(a2).play(ANTHEM, { velocity: 104, gate: 0.88, transpose: -12 });
        square.at(a2).play(ANTHEM, { velocity: 80, gate: 0.6 });
        for (let p = 0; p < 4; p++) shape(eel, a2 + p * 16, a2 + p * 16 + 16, { peak: 10, ending: -6 });
        sustain(choir, pA2, { center: 55, velocity: 68 });
        sustain(voices, pA2, { center: 67, velocity: 54 });
        bassline(bass, pA2, { pattern: PUMP });
        arp(arpSynth, pA2, ARP16);
        beat(kit, a2, 16, FOUR);
        kit.hit(a2, CRASH, 108, 1).hit(a2 + 32, CRASH, 104, 1);
        royalStabs(pA2, a2, [2, 6, 10, 14], [0, 0.75, 1.5], 104);
        royalStabs(pA2, a2, [3, 7, 11, 15], [0]);
        for (const bar of [3, 7, 11, 15]) zaps(zap, a2 + bar * 4 + 2.5, { count: 3, spread: 1.2, velocity: 104 });
        bubbles(bub, a2 + 62, { count: 8, spread: 2, low: 250, high: 1200 });
    });
    songs.push(song);
}

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
    const song = new Song("theme_crab", "Fish Fighters - Crab Cove", 132, { key: "A", introBars: 2, bodyBars: 48,
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
// 12. "King Crab's Sideways Stomp" - the crustaceans' boss stages. Heavy and jumpy, like a crab walks: never
//     smooth. Distorted power-chord chugs in 3+3+2, then a freeze (everything stops), then a chromatic scuttle
//     sideways into the next chord; taiko stomps, a heavy kit that holds its breath with the riff, low brass on
//     the cove's tune. The cove's pinchy instruments are all there in the gaps: castanets and claves clicking,
//     staccato bassoon, xylophone and pizzicato "snip-snip-snap"s.
//     A minor. Intro 4 | A 16 (the stomp) | B 8 (the claw-off, half time) | C 8 (scuttle and freeze) | A' 16
// =====================================================================================================
{
    const song = new Song("theme_crab_boss", "Fish Fighters - King Crab's Sideways Stomp", 120, { introBars: 4, bodyBars: 48, key: "A",
        mix: { lead: 0, guitar: -3, bass: -4, pinch: -5, drums: -1, percussion: -6, fx: -10 } });
    const brass = song.part("Brass", 61, { group: "lead", volume: 110, pan: -0.1 });
    const trombones = song.part("Trombones", 57, { group: "lead", volume: 106, pan: 0.2 });
    const guitar = song.part("Distortion guitar", 30, { group: "guitar", volume: 100, pan: -0.3 });
    const guitar2 = song.part("Overdrive guitar", 29, { group: "guitar", volume: 92, pan: 0.3 });
    const tuba = song.part("Tuba", 58, { group: "bass", volume: 108 });
    const bassoon = song.part("Bassoon", 70, { group: "pinch", volume: 104, pan: -0.25 });
    const xylo = song.part("Xylophone", 13, { group: "pinch", volume: 96, pan: 0.4 });
    const pizz = song.part("Pizzicato", 45, { group: "pinch", volume: 96, pan: -0.4 });
    const kit = song.drums("Power kit", 16, { group: "drums", volume: 104 });
    const taiko = song.part("Taiko", 116, { group: "drums", volume: 110 });
    const claws = song.drums("Claws", 0, { group: "percussion", volume: 96, pan: 0.25 });
    const bub = song.synth("Bubbles", "bubbles", { group: "fx" });

    const A = "Am | Am | Dm | E | Am | F | Dm E | Am | Am | E | Dm | Am | F | E | Am E | Am";
    const B = CRAB.B;
    const C = CRAB.C;

    // The stomp: [offset, length, semitones from the bar's root]; "next" = from the next bar's root (the scuttle into it)
    const STOMP = [[0, 0.75, 0], [0.75, 0.75, 0], [1.5, 0.5, 0], [2.5, 0.25, 3], [2.75, 0.25, 2], [3, 0.5, 0]];
    const STOMP_TURN = [[0, 0.75, 0], [0.75, 0.75, 0], [1.5, 0.5, 1], [3, 0.25, "next-4"], [3.25, 0.25, "next-3"], [3.5, 0.25, "next-2"], [3.75, 0.25, "next-1"]];
    const rootOf = (prog, t) => chord(chordAt(prog, t).symbol).root;
    // A bar of the riff: power chords on the guitars (root, fifth, octave), the tuba on the roots an octave down
    const riffBar = (prog, t, pattern, velocity = 104) => {
        const root = rootOf(prog, t), next = rootOf(prog, t + 4);
        for (const [o, l, step] of pattern) {
            const isScuttle = typeof step === "string";
            const n = isScuttle ? above(next, 40) - Number(step.slice(5)) : above(root, 40) + step;
            const v = velocity - (isScuttle ? 14 : o === 0 ? 0 : 8);
            [0, 7, 12].forEach(i => { guitar.hit(t + o, n + i, v, l * 0.7); guitar2.hit(t + o, n + i, v - 8, l * 0.7); });
            tuba.hit(t + o, n - 12, v, l * 0.8);
        }
    };
    const riff = (prog, start, bars, velocity) => {
        for (let bar = 0; bar < bars; bar++) riffBar(prog, start + bar * 4, bar % 2 ? STOMP_TURN : STOMP, velocity);
    };
    // A pinch in a gap: snip-snip-snap on the chord (xylophone, pizzicato, a castanet click)
    const pinch = (prog, t, velocity = 96) => {
        const entry = chordAt(prog, t);
        [[0, "8"], [0.5, "8"], [1, "12"]].forEach(([o, token]) => {
            xylo.hit(t + o, tone(entry, token, 69), velocity, 0.15);
            pizz.hit(t + o, tone(entry, token, 57), velocity - 8, 0.2);
        });
        claws.hit(t + 1.5, CASTANETS, 112, 0.1).hit(t + 1.75, CASTANETS, 96, 0.1);
    };

    // The heavy kit holds its breath with the riff (steps 7-11: the freeze); taiko on the stomps
    const HEAVY = { [KICK]: "X..X..X.....X.X.", [SNARE]: "....X.......X...", [HAT]: "x.x.x.x.....x.x." };
    const HEAVY_FULL = { [KICK]: "X..X..X.....X.XX", [SNARE]: "....X.......X..g", [CRASH]: "X...............", [HAT]: "x.x.x.x.....x.x." };
    const TAIKO = [[0, 120], [0.75, 100], [1.5, 110]];
    const PINCHY = { [CASTANETS]: "x.mxm.x.....xmx.", [CLAVES]: "x..x..x.........", [WOOD_LOW]: "............o..." };
    const CLAW_OFF = { [KICK]: "X.......X.......", [SNARE]: "........X.......", [RIDE]: "o...o...o...o..." };
    const taikoBars = (start, bars) => { for (let bar = 0; bar < bars; bar++) for (const [o, v] of TAIKO) taiko.hit(start + bar * 4 + o, pitch("D2"), v, 0.5); };

    // The king's tune: the cove's motif in heavy brass, every phrase stopping dead (the pinches answer in the gaps)
    const KING = "E4:.5 A4:.5 C5:.5 r:.5 B4:.5 A4:.5 E5:.5 r:.5 | E5:.75 D5:.75 C5:.5 r:2 | D5:.5 F5:.5 A5:.5 r:.5 G5:.5 F5:.5 D5:.5 r:.5 | E5:.75 F5:.75 E5:.5 r:2 | " +
                 "E4:.5 A4:.5 C5:.5 r:.5 B4:.5 A4:.5 E5:.5 r:.5 | A5:.75 G5:.75 F5:.5 r:1 E5:.5 r:.5 | F5:.5 E5:.5 D5:.5 C5:.5 B4:.75 G#4:.75 E4:.5 | A4:1.5 r:2.5 | " +
                 "C5:.5 E5:.5 A5:.5 r:.5 G5:.5 E5:.5 C6:.5 r:.5 | B5:.75 A5:.75 G#5:.5 r:2 | A5:.5 F5:.5 D5:.5 r:.5 F5:.5 A5:.5 D6:.5 r:.5 | C6:.75 B5:.75 A5:.5 r:2 | " +
                 "F5:.75 F5:.75 E5:.5 r:.5 D5:.25 E5:.25 F5:.5 r:.5 | G#5:.75 G#5:.75 A5:.5 r:.5 B5:.25 C6:.25 B5:.5 r:.5 | A5:.5 E5:.5 C5:.5 A4:.5 G#4:.5 B4:.5 E5:.5 G#5:.5 | A5:1 r:3";
    const GAP_BARS = [1, 3, 7, 9, 11, 15];   // Bars of KING that stop at beat 2: a pinch there
    const CLAW_LINE = "D3:2 F3:2 | E3:2 C3:2 | B2:2 G#2:2 | A2:3 r:1 | D3:2 F3:2 | A3:2 E3:2 | D#3:2 F#3:2 | G#3:2 E3:1 r:1";

    // Intro: the cove's castanets sneak in alone... then the king lands: STOMP, freeze, scuttle
    beat(claws, 0, 2, { [CASTANETS]: "....x.mxm.x.mxmx", [CLAVES]: "x..x..x...x.x..." });
    bubbles(bub, 1, { count: 8, spread: 3, low: 300, high: 1100 });
    const pI = progression("Am | E", 8);
    riffBar(pI, 8, STOMP, 112);
    riffBar(pI, 12, STOMP_TURN, 112);
    taikoBars(8, 2);
    beat(kit, 8, 2, HEAVY);
    kit.hit(8, CRASH, 116, 1);
    voice("Am", 60).forEach(n => brass.hit(8, n, 110, 1.4));
    voice("E", 59).forEach(n => brass.hit(12, n, 106, 1.4));

    song.body(s => {
        const a = s, b = s + 64, c = s + 96, a2 = s + 128;
        const pA = progression(A, a), pB = progression(B, b), pC = progression(C, c), pA2 = progression(A, a2);

        // A: the stomp riff, the brass on the king's tune, pinches in every gap, the claws clicking
        brass.at(a).play(KING, { velocity: 104, gate: 0.7 });
        for (let p = 0; p < 4; p++) shape(brass, a + p * 16, a + p * 16 + 16, { peak: 10, ending: -4 });
        riff(pA, a, 16, 102);
        GAP_BARS.forEach(bar => pinch(pA, a + bar * 4 + 2));
        bassoon.at(a + 20).play("r:2 E4:.25 r:.25 A4:.25 r:.25 C5:.25 r:.75", { velocity: 92, gate: 0.6 });
        taikoBars(a, 16);
        beat(kit, a, 16, HEAVY);
        kit.hit(a, CRASH, 112, 1).hit(a + 32, CRASH, 110, 1);
        beat(claws, a, 16, PINCHY);
        bubbles(bub, a + 30, { count: 5, spread: 1.5 });

        // B: the claw-off - half time and heavy: trombones low, the cove's pinchy tune on bassoon and xylophone,
        // power chords held, taiko on 1 and 3
        trombones.at(b).play(CLAW_LINE, { velocity: 104, gate: 0.85 });
        bassoon.at(b).play(CRAB.MEL_B, { velocity: 100, gate: 0.45 });
        xylo.at(b).play(CRAB.MEL_B, { velocity: 84, gate: 0.3, transpose: 12 });
        for (let bar = 0; bar < 8; bar++) {
            const t = b + bar * 4, root = above(rootOf(pB, t), 40);
            [0, 2].forEach(o => [0, 7, 12].forEach(i => { guitar.hit(t + o, root + i, 100 - o * 4, 1.6); guitar2.hit(t + o, root + i, 90, 1.6); }));
            tuba.hit(t, root - 12, 104, 1.8).hit(t + 2, root - 12, 96, 1.8);
            taiko.hit(t, pitch("D2"), 118, 0.5).hit(t + 2, pitch("D2"), 108, 0.5);
        }
        figure(pizz, pB, { pattern: ["R", "8", "5", "8"], rate: 0.5, base: 57, velocity: 74, gate: 0.4 });
        beat(kit, b, 8, CLAW_OFF);
        beat(claws, b, 8, { [CASTANETS]: "x.mxm.x.x.mxm.x.", [CLAP]: "....x.......x..." });
        kit.hit(b, CRASH, 108, 1);

        // C: scuttle and freeze - the riff alone with long freezes, the bassoon's fragments, castanets in the holes;
        // then the crabs scuttle up (16ths on pizzicato and xylophone) into a double stomp
        bassoon.at(c).play(CRAB.FRAGMENTS, { velocity: 102, gate: 0.5 });
        for (let bar = 0; bar < 6; bar++) {
            const t = c + bar * 4;
            riffBar(pC, t, bar % 2 ? [[0, 0.75, 0], [0.75, 0.75, 0], [1.5, 0.5, 1]] : [[0, 0.75, 0], [0.75, 0.75, 0], [1.5, 0.5, 0]], 100);
            taiko.hit(t, pitch("D2"), 116, 0.5);
            beat(claws, t, 1, { [CASTANETS]: "........xmxm.x.x", [CLAVES]: "........x..x...." });
        }
        for (let i = 0; i < 32; i++) {
            const n = pitch("E4") + i, t = c + 24 + i * 0.25;
            pizz.hit(t, n, 70 + i, 0.2);
            if (i % 2 === 0) xylo.hit(t, n + 12, 70 + i, 0.15);
        }
        beat(kit, c + 24, 2, { [KICK]: "X...X...X...X...", [SNARE]: "..............xx" });
        roll(kit, c + 28, 4, SNARE, 50, 118);
        [c + 30, c + 31].forEach(t => [0, 7, 12].forEach(i => guitar.hit(t, pitch("E2") + i, 116, 0.5)));
        taiko.hit(c + 30, pitch("D2"), 124, 0.5).hit(c + 31, pitch("D2"), 124, 0.5);
        bubbles(bub, c + 6, { count: 8, spread: 4, low: 250, high: 900 });

        // A': everyone - brass and trombones (an octave down) on the tune, the xylophone on its first notes,
        // the full heavy kit, pinches and claws
        brass.at(a2).play(KING, { velocity: 108, gate: 0.7 });
        trombones.at(a2).play(KING, { velocity: 100, gate: 0.7, transpose: -12 });
        for (let p = 0; p < 4; p++) shape(brass, a2 + p * 16, a2 + p * 16 + 16, { peak: 10, ending: -4 });
        xylo.at(a2).play(KING.split("|").slice(0, 2).join("|"), { velocity: 80, gate: 0.4, transpose: 12 });
        riff(pA2, a2, 16, 106);
        GAP_BARS.forEach(bar => pinch(pA2, a2 + bar * 4 + 2, 100));
        taikoBars(a2, 16);
        beat(kit, a2, 16, HEAVY_FULL);
        beat(claws, a2, 16, { ...PINCHY, [CLAP]: "....x.......x..." });
        bubbles(bub, a2 + 62, { count: 8, spread: 2 });
    });
    songs.push(song);
}

// =====================================================================================================
// 12b. "It Came From The Trench!" - the most stressful and dangerous stages (swarms, something huge down there):
//     a 1960s B-movie creature feature played by a surf-noir band. Tremolo-picked surf guitar, a cheesy combo
//     organ, a theremin, picked bass and surf drums in G minor; a chromatic riff, D7 stabs, a "lights out"
//     heartbeat, the creature's lunge; an anglerfish lure blinking in the wrong key, "gulp" stop-times.
//     Intro 4 | A 16 | B 8 (the chase, half time) | C 8 (lights out) | A' 12
// =====================================================================================================
{
    const song = new Song("theme_danger", "Fish Fighters - It Came From The Trench!", 156, { introBars: 4, bodyBars: 44, key: "G",
        mix: { lead: 0, guitar: -2, keys: -5, bass: -3, brass: -4, drums: -2, fx: -9 } });
    const ther = song.synth("Theremin", "theremin", { group: "lead", fx: { echo: { beats: 0.75, feedback: 0.3, mix: 0.25 } } });
    const guitar = song.part("Surf guitar", 27, { group: "guitar", volume: 104, pan: -0.3, fx: { echo: { beats: 0.25, feedback: 0.2, mix: 0.2 } } });
    const muted = song.part("Muted guitar", 28, { group: "guitar", volume: 92, pan: 0.35 });
    const organ = song.part("Combo organ", 17, { group: "keys", volume: 96, pan: 0.25 });
    const celeste = song.part("Lure", 8, { group: "keys", volume: 90, pan: -0.45, fx: { echo: { beats: 0.75, feedback: 0.4, mix: 0.3 } } });
    const bass = song.part("Pick bass", 34, { group: "bass", volume: 106 });
    const brass = song.part("Brass", 61, { group: "brass", volume: 108, pan: 0.1 });
    const kit = song.drums("Room kit", 8, { group: "drums", volume: 100 });
    const ping = song.synth("Sonar", "ping", { group: "fx", fx: { echo: { beats: 1.5, feedback: 0.45, mix: 0.4 } } });
    const bub = song.synth("Bubbles", "bubbles", { group: "fx" });

    const A = "Gm | Gm | Gm | Gm | Eb | Eb | D7 | D7 | Gm | Gm | Gm | Gm | Cm | Eb | D7 | D7sus4";
    const B = "Cm | Cm | Ab | Ab | F | F | D7 | D7";
    const C = "Gm | Gm | Gm | Gm | Gm | Gm | Gm | D7";
    const A2 = "Gm | Gm | Gm | Gm | Eb | Eb | D7 | D7 | Cm | Eb | D7 | D7sus4";
    const bars = (text, from, to) => text.split("|").slice(from, to).join("|");

    const BASS8 = [[0, 0.45, "R", 104], [0.5, 0.45, "R", 80], [1, 0.45, "8", 88], [1.5, 0.45, "R", 80],
                   [2, 0.45, "R", 100], [2.5, 0.45, "R", 80], [3, 0.45, "5", 88], [3.5, 0.45, "app", 86]];
    const CHOP = [[0.5, 0.2, 72], [1.5, 0.2, 66], [2.5, 0.2, 72], [3.5, 0.2, 66]];
    const SURF_A = { [KICK]: "x.....x...x.....", [SNARE]: "....x.......x..g", [HAT]: "m.o.m.o.m.o.m.o.", [LOW_TOM]: "...........o...." };
    const SURF_FULL = { [KICK]: "x.....x.x.x.....", [SNARE]: "....x..g....x..g", [HAT]: "m.o.m.o.m.o.m.o.", [OPEN_HAT]: "..............o.", [LOW_TOM]: "..........o.o..." };
    const HALF = { [KICK]: "x.........x.....", [SNARE]: "........x.......", [RIDE]: "m.o.m.o.m.o.m.o." };
    const HEART = { [KICK]: "x..x............" };
    const HEART_FAST = { [KICK]: "x..x....x..x...." };
    const GULP = { [KICK]: "x.....x.........", [SNARE]: "....x...........", [HAT]: "m.o.m.o.m.o....." };

    const RIFF = "D3:.5 G3:.5 Bb3:.5 A3:.5 G3:.5 D4:.5 C#4:.5 D4:.5 | Eb4:.5 D4:.5 Bb3:.5 G3:.5 A3:.5 F#3:.5 G3:1";
    const TUNE_A = "D5:2 G5:2 | Bb5:3 A5:1 | G5:2 D6:2 | C#6:1 D6:1 Eb6:1 D6:1 | Bb5:2 G5:2 | Eb5:3 F5:1 | F#5:2 A5:2 | C6:4 | " +
                   "D5:2 G5:2 | Bb5:3 C6:1 | D6:2 F6:2 | Eb6:1.5 D6:.5 C6:1 Bb5:1 | C6:2 Eb6:2 | D6:2 Bb5:1 G5:1 | A5:2 C6:1 F#5:1 | G5:4";
    const TUNE_B = "G5:.5 G5:.5 r:.5 G5:.5 Ab5:.5 G5:.5 F5:.5 Eb5:.5 | C5:3 r:1 | " +
                   "Ab5:.5 Ab5:.5 r:.5 Ab5:.5 Bb5:.5 Ab5:.5 G5:.5 Eb5:.5 | C5:3 r:1 | " +
                   "A5:.5 A5:.5 r:.5 A5:.5 Bb5:.5 A5:.5 G5:.5 F5:.5 | C6:3 r:1 | " +
                   "F#5:.5 A5:.5 C6:.5 Eb6:.5 D6:.5 C6:.5 A5:.5 F#5:.5 | D5:1 r:1 D5:.5 D5:.5 D5:1";
    const TUNE_C = "G4:4 | D5:4 | Eb5:4 | D5:2 C#5:2 | G5:4 | F#5:4 | r:4 | r:4";
    const LURE = "F#5:.25 A#5:.25 C#6:.25 F#6:.25";   // F# major, a half step above the key: the fake light
    const phrases = text => text.split("|").reduce((list, bar, i) => { if (i % 4 === 0) list.push([]); list[list.length - 1].push(bar); return list; }, []).map(p => p.join("|"));

    const chug = (prog, velocity = 72) => figure(muted, prog, { pattern: ["R"], rate: 0.5, base: 43, velocity, accent: 12, gate: 0.45 });
    const gulp = (beat, parts) => {
        stopTime(parts, beat, beat + 1);
        bubbles(bub, beat, { count: 1, spread: 0.01, low: 240, high: 241, size: [0.14, 0.14], velocity: 110 });
    };
    const band = [guitar, muted, organ, celeste, bass, brass];
    const surfRiff = (start, { withOrgan = false } = {}) => {
        tremolo(guitar.at(start), RIFF + " | " + RIFF, { velocity: 92 });
        bass.at(start).play(RIFF + " | " + RIFF, { velocity: 100, gate: 0.6, transpose: -12 });
        if (withOrgan) organ.at(start).play(RIFF + " | " + RIFF, { velocity: 80, gate: 0.4, transpose: 12 });
    };
    const tremChords = prog => figure(guitar, prog, { pattern: ["R", "5", "8", "5"], rate: 0.25, base: 55, velocity: 84, accent: 8, gate: 0.45 });
    const theremA = (start, text) => phrases(text).forEach((p, i) => theremin(ther, start + i * 16, p, { velocity: 96, glide: 0.07, swoopFirst: 7 }));

    // Intro: the sonar, a heartbeat in the dark, the lure blinking; the chug creeps up; the guitar dives
    ping.add(0, 880, 100);
    beat(kit, 0, 2, HEART);
    [0, 0.75, 4, 4.75].forEach(t => bass.hit(t, pitch("G1"), 100, 0.4));
    celeste.at(2).play(LURE, { velocity: 50 });
    celeste.at(6).play(LURE, { velocity: 50 });
    chug(progression("Gm | D7", 8));
    muted.events.filter(e => e.time >= 8 && e.time < 16).forEach(e => { e.velocity = Math.min(127, e.velocity + Math.floor(e.time - 8) * 4); });
    roll(kit, 12, 4, SNARE, 40, 112);
    guitar.hit(15, 55, 104, 1, -12);

    song.body(s => {
        const a = s, b = s + 64, c = s + 96, a2 = s + 128;
        const pA = progression(A, a), pB = progression(B, b), pC = progression(C, c), pA2 = progression(A2, a2);

        // A: the riff in tremolo, the theremin's tune (the motif, slowed down and wobbling), stabs, two gulps
        surfRiff(a);
        surfRiff(a + 32);
        tremChords(progression(bars(A, 4, 8), a + 16));
        tremChords(progression(bars(A, 12, 16), a + 48));
        bassline(bass, progression(bars(A, 4, 8), a + 16), { pattern: BASS8, base: 31 });
        bassline(bass, progression(bars(A, 12, 16), a + 48), { pattern: BASS8, base: 31 });
        chug(pA);
        comp(organ, pA, { pattern: CHOP, center: 67 });
        theremA(a, TUNE_A);
        [[a + 16], [a + 24], [a + 48], [a + 56], [a + 57.5]].forEach(([t]) => stab(brass, pA, t, [0], { velocity: 106, length: 0.3, center: 60 }));
        beat(kit, a, 7, SURF_A);
        beat(kit, a + 28, 1, GULP);
        beat(kit, a + 32, 7, SURF_A);
        beat(kit, a + 60, 1, GULP);
        kit.hit(a, CRASH, 108, 1).hit(a + 32, CRASH, 104, 1);
        gulp(a + 31, band);
        gulp(a + 63, band);

        // B: the chase - half time, the combo organ in octaves, the guitar under it, brass, "stab stab STAB"
        organ.at(b).play(TUNE_B, { velocity: 96, gate: 0.6 });
        organ.at(b).play(TUNE_B, { velocity: 88, gate: 0.6, transpose: -12 });
        tremolo(guitar.at(b), TUNE_B, { velocity: 84, transpose: -12 });
        bassline(bass, pB, { pattern: [[0, 0.9, "R", 104], [1, 0.45, "R", 80], [2.5, 0.45, "8", 88], [3, 0.9, "app", 90]], base: 31 });
        beat(kit, b, 7, HALF);
        kit.hit(b, CRASH, 106, 1);
        [b, b + 8, b + 16].forEach(t => stab(brass, pB, t, [0], { velocity: 110, length: 0.3, center: 60 }));
        brass.at(b + 24).play("F#4:.5 A4:.5 C5:.5 Eb5:.5 D5:.5 C5:.5 A4:.5 F#4:.5", { velocity: 100, gate: 0.7 });
        for (const t of [b + 28, b + 30, b + 30.5, b + 31]) {
            [pitch("D3"), pitch("A3"), pitch("D4")].forEach(n => { brass.hit(t, n, 112, 0.35); organ.hit(t, n + 12, 104, 0.3); guitar.hit(t, n, 108, 0.3); });
            kit.hit(t, KICK, 116);
        }

        // C: lights out - a heartbeat, the sonar, the lure blinking, the theremin moaning; then the lunge
        beat(kit, c, 4, HEART);
        beat(kit, c + 16, 2, HEART_FAST);
        for (let bar = 0; bar < 6; bar++) (bar < 4 ? [0, 0.75] : [0, 0.75, 2, 2.75]).forEach(t => bass.hit(c + bar * 4 + t, pitch("G1"), 98, 0.4));
        [0, 8, 16].forEach(t => ping.add(c + t, 880, 96));
        [6, 14, 22].forEach(t => celeste.at(c + t).play(LURE, { velocity: 50 }));
        theremin(ther, c, TUNE_C, { velocity: 88, glide: 0.4 });
        bubbles(bub, c + 2, { count: 12, spread: 20, low: 160, high: 600, size: [0.1, 0.2] });
        roll(kit, c + 24, 4, LOW_TOM, 40, 104);
        chug(progression("Gm", c + 24));
        muted.events.filter(e => e.time >= c + 24 - 1e-6 && e.time < c + 28 - 1e-6).forEach(e => { e.velocity = Math.min(127, 60 + Math.round((e.time - c - 24) * 12)); });
        [50, 51, 57, 62].forEach(n => brass.hit(c + 28, n, 120, 2, -2));                   // the lunge: a cluster falling
        guitar.hit(c + 28, 55, 110, 2, -12);                                              // the dive
        kit.hit(c + 28, CRASH, 120, 2);
        tomFill(kit, c + 30, 2);

        // A': the riff with the organ on top, the theremin's tune shortened, brass on every change, one last gulp
        surfRiff(a2, { withOrgan: true });
        surfRiff(a2 + 32);
        tremChords(progression(bars(A2, 4, 8), a2 + 16));
        bassline(bass, progression(bars(A2, 4, 8), a2 + 16), { pattern: BASS8, base: 31 });
        chug(pA2);
        comp(organ, progression(bars(A2, 4, 12), a2 + 16), { pattern: CHOP, center: 67 });
        const tune2 = bars(TUNE_A, 0, 8) + "|" + bars(TUNE_A, 12, 16);
        theremA(a2, tune2);
        organ.at(a2).play(tune2, { velocity: 74, gate: 0.9, transpose: -12 });
        for (const entry of pA2) if (entry.beat === a2 || entry.symbol !== chordAt(pA2, entry.beat - 0.01).symbol)
            stab(brass, pA2, entry.beat, [0], { velocity: 104, length: 0.3, center: 60 });
        beat(kit, a2, 7, SURF_FULL);
        tomFill(kit, a2 + 30, 2);
        beat(kit, a2 + 32, 3, SURF_FULL);
        beat(kit, a2 + 44, 1, GULP);
        kit.hit(a2, CRASH, 112, 1).hit(a2 + 32, CRASH, 108, 1);
        gulp(a2 + 47, band);
    });
    songs.push(song);
}

// =====================================================================================================
// 13. "Battle Stations!" - battle start: the submarine's crew drums to battle stations. A fast drum fill, a low
//     brass call "da-dada-dada" on the motif's frame (sol, do, then a chromatic climb to sol'), one big held power
//     chord: no third, no tune, no steel drums (that's the victory's). Rendered in the 7 keys of the stage themes,
//     so the hit is always on the tonic of the theme that follows it (jingle_battle_start = C, the fallback).
// =====================================================================================================
for (const [keyName, T] of [["C", 0], ["D", 2], ["Eb", 3], ["E", 4], ["F", 5], ["G", -5], ["A", -3]]) {
    const name = keyName === "C" ? "jingle_battle_start" : "jingle_battle_start_" + keyName;
    const song = new Song(name, "Fish Fighters - Battle Stations! (" + keyName + ")", 160, { kind: "jingle", key: keyName,
        mix: { brass: 0, bass: -4, drums: -1, fx: -10 } });
    song.endBeat = 6;   // The theme starts here (the crash rings under it)
    const brass = song.part("Brass", 61, { group: "brass", volume: 112, pan: -0.1 });
    const trumpet = song.part("Trumpet", 56, { group: "brass", volume: 106, pan: 0.25 });
    const trombones = song.part("Trombones", 57, { group: "brass", volume: 110, pan: 0.1 });
    const tuba = song.part("Tuba", 58, { group: "bass", volume: 108 });
    const kit = song.drums("Kit", 0, { group: "drums", volume: 100 });
    const taiko = song.part("Taiko", 116, { group: "drums", volume: 104 });
    const timpani = song.part("Timpani", 47, { group: "drums", volume: 106 });
    const ping = song.synth("Sonar", "ping", { group: "fx", fx: { echo: { beats: 1, feedback: 0.35, mix: 0.35 } } });
    const bub = song.synth("Bubbles", "bubbles", { group: "fx" });

    // 0-2: the sonar and a 32nd-note snare roll, then the toms run down, the timpani swelling underneath
    ping.add(0, 1180, 104);
    bubbles(bub, 0, { count: 12, spread: 1.6, low: 300, high: 1500, velocity: 84 });
    taiko.hit(0, 45, 116, 0.8).hit(1, 45, 104, 0.8);
    roll(kit, 0, 1, SNARE, 46, 100, 0.125);
    tomFill(kit, 1, 1, 110);
    roll(timpani, 0, 2, foldTimp(pitch("C2") + T), 40, 96, 0.125);

    // 2-4.5: the call "Da-dada-dada-da-da" (sol, do, then a chromatic climb to sol'), doubled by toms and kick
    trombones.at(2).play("G3:.25 r:.25 G3:.25 C4:.25 r:.25 G3:.25 C4:.25 r:.25 F4:.25 F#4:.25", { velocity: 110, gate: 0.7, transpose: T });
    brass.at(4).play("F4:.25 F#4:.25", { velocity: 104, gate: 0.8, transpose: T });
    [[2, 45], [2.5, 45], [2.75, 48], [3.25, 45], [3.5, 48], [4, 50], [4.25, 50]].forEach(([b, n]) => { kit.hit(b, n, 108, 0.2); kit.hit(b, KICK, 96); });
    roll(kit, 2, 2.5, SNARE, 44, 72, 0.25);
    timpani.hit(2, foldTimp(pitch("G2") + T), 100, 0.3).hit(2.75, foldTimp(pitch("C3") + T), 104, 0.3).hit(3.5, foldTimp(pitch("C3") + T), 108, 0.3);

    // 4.5-6: THE HIT - a power chord on the key's tonic, held
    ["C3", "G3", "C4", "G4"].forEach(n => brass.hit(4.5, pitch(n) + T, 118, 1.5));
    trumpet.hit(4.5, pitch("C5") + T, 112, 1.5);
    trombones.hit(4.5, pitch("C3") + T, 114, 1.5).hit(4.5, pitch("G3") + T, 110, 1.5);
    tuba.hit(4.5, pitch("C2") + T, 118, 1.5);
    timpani.hit(4.5, foldTimp(pitch("C2") + T), 124, 1.5);
    taiko.hit(4.5, 45, 124, 1);
    kit.hit(4.5, KICK, 124).hit(4.5, CRASH, 124, 2).hit(4.5, CRASH2, 100, 2);
    bubbles(bub, 4.5, { count: 14, spread: 1.2, low: 500, high: 2200, velocity: 86 });
    bubbles(bub, 6.2, { count: 1, spread: 0.01, low: 260, high: 261, size: [0.12, 0.12], velocity: 100 });   // one low "bloop" after the shout
    songs.push(song);
}

// =====================================================================================================
// 14. Victory: the motif as a fanfare on steel drums and brass, a big F major, bubbles fizzing up
// =====================================================================================================
{
    const song = new Song("jingle_victory", "Fish Fighters - Victory", 150, { key: "F", kind: "jingle", mix: { lead: 0, brass: -1, strings: -5, bass: -4, drums: -4, bubbles: -7 } });
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
    const song = new Song("jingle_defeat", "Fish Fighters - Defeat", 96, { key: "F", kind: "jingle", mix: { lead: 0, bass: -3, strings: -6, keys: -5, bubbles: -4 } });
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
    const end = song.endBeat ?? Math.max(...song.parts.flatMap(p => p instanceof Part ? p.events.map(e => e.time + e.length) : p.events.map(e => e[0])));
    // The game loops the second body copy: [loopStartSample, loopStartSample + loopLengthSample) at 48 kHz
    const introSamples = Math.round(song.intro * spb * RATE), lengthSamples = Math.round(song.length * spb * RATE);
    loops[song.name] = {
        title: song.title.replace("Fish Fighters - ", ""),
        kind: song.kind,
        key: song.key,
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

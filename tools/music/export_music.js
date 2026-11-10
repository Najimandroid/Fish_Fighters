// Turns the mixes (audio_sources/waveform/Fish Fighters Music/Exported/<piece>.wav, 48 kHz float, from mix_music.js) into the
// game's files: assets/audios/music/<piece>.ogg and assets/audios/music/music.json (loop points in samples).
//
// - Loudness: the themes at -16 LUFS (measured over their loop), the jingles at -15 LUFS, peaks kept under -1 dBFS.
// - Seamless loop: the game plays the intro, then loops [loopStart, loopStart + loopLength). The last 100 ms of the loop
//   are crossfaded into the audio just before loopStart, so the jump lands exactly where the waveform already is
//   (the notes of both body copies are identical; this only evens out the reverb tails, which never repeat exactly).
//   Everything after the loop end is cut, the game never plays it.
//
// Usage: node tools/music/export_music.js     (needs ffmpeg on the PATH)

const fs = require("fs");
const os = require("os");
const path = require("path");
const { execFileSync } = require("child_process");
const { loudness } = require("./loudness");

const ROOT = path.join(__dirname, "..", "..");
const RENDERS = path.join(ROOT, "audio_sources", "waveform", "Fish Fighters Music", "Exported");
const LOOPS = path.join(ROOT, "audio_sources", "midi", "loops.json");
const OUT = path.join(ROOT, "assets", "audios", "music");
const RATE = 48000, CROSSFADE = RATE / 10, PEAK_LIMIT = Math.pow(10, -1 / 20);

function readWav(file) {
    const b = fs.readFileSync(file);
    let p = 12, format = null, data = null;
    while (p < b.length) {
        const id = b.toString("ascii", p, p + 4), size = b.readUInt32LE(p + 4);
        if (id === "fmt ") format = { type: b.readUInt16LE(p + 8), channels: b.readUInt16LE(p + 10), rate: b.readUInt32LE(p + 12), bits: b.readUInt16LE(p + 22) };
        if (id === "data") { data = { start: p + 8, size }; break; }
        p += 8 + size + (size & 1);
    }
    if (!format || format.type !== 3 || format.bits !== 32 || format.channels !== 2 || format.rate !== RATE)
        throw new Error(file + ": expected a 48 kHz stereo 32-bit float WAV (Waveform render settings)");
    const samples = new Float32Array(data.size / 4);
    for (let i = 0; i < samples.length; i++) samples[i] = b.readFloatLE(data.start + i * 4);
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

const loops = JSON.parse(fs.readFileSync(LOOPS, "utf8"));
const manifest = {};
fs.mkdirSync(OUT, { recursive: true });

for (const [name, info] of Object.entries(loops)) {
    let samples = readWav(path.join(RENDERS, name + ".wav"));
    const looped = info.loopStartSample !== null;
    let measured;

    if (looped) {
        const start = info.loopStartSample, end = start + info.loopLengthSample;
        if (end * 2 > samples.length) throw new Error(name + ": the render is shorter than its loop");
        for (let i = 0; i < CROSSFADE; i++) {
            const w = (i + 1) / CROSSFADE;
            for (let ch = 0; ch < 2; ch++) {
                const at = (end - CROSSFADE + i) * 2 + ch;
                samples[at] = samples[at] * (1 - w) + samples[(start - CROSSFADE + i) * 2 + ch] * w;
            }
        }
        // 50 ms past the loop end, never played: the loop's own start, so the encoder sees the same music after the loop
        // end as after the loop start (a downbeat right after the loop start is otherwise smeared back only there)
        for (let i = 0; i < (RATE / 20) * 2; i++) samples[end * 2 + i] = samples[start * 2 + i];
        samples = samples.slice(0, (end + RATE / 20) * 2);
        measured = loudness(samples.subarray(start * 2, end * 2));
    } else {
        let frames = samples.length / 2;
        while (frames > 0 && Math.abs(samples[frames * 2 - 1]) < 1e-4 && Math.abs(samples[frames * 2 - 2]) < 1e-4) frames--;
        frames = Math.min(samples.length / 2, frames + RATE / 20);
        samples = samples.slice(0, frames * 2);
        const fade = RATE / 50;
        for (let i = 0; i < fade; i++) for (let ch = 0; ch < 2; ch++) samples[(frames - fade + i) * 2 + ch] *= 1 - i / fade;
        measured = loudness(samples);
    }

    const target = looped ? -16 : -15;
    let peak = 0;
    for (const s of samples) peak = Math.max(peak, Math.abs(s));
    const gain = Math.min(Math.pow(10, (target - measured) / 20), PEAK_LIMIT / peak);
    for (let i = 0; i < samples.length; i++) samples[i] *= gain;

    const temp = path.join(os.tmpdir(), "fish_fighters_" + name + ".wav");
    writeFloatWav(temp, samples);
    const file = path.join(OUT, name + ".ogg");
    execFileSync("ffmpeg", ["-hide_banner", "-loglevel", "error", "-y", "-i", temp, "-c:a", "libvorbis", "-q:a", "6", file]);
    fs.rmSync(temp);

    // title and kind (menu / stage / jingle) are for the editor's music lists
    manifest[name] = { title: info.title, kind: info.kind, file: "assets/audios/music/" + name + ".ogg", sampleRate: RATE };
    if (info.key) manifest[name].key = info.key;   // The tonic: the battle start jingle and the boss stinger match it
    if (looped) Object.assign(manifest[name], { loopStart: info.loopStartSample, loopLength: info.loopLengthSample });
    else manifest[name].length = info.lengthSeconds;   // Jingles: where the last note ends (the reverb rings after it)
    console.log(name.padEnd(16), (samples.length / 2 / RATE).toFixed(2).padStart(7) + " s",
        "  " + (measured + 20 * Math.log10(gain)).toFixed(1) + " LUFS", "  peak " + (20 * Math.log10(peak * gain)).toFixed(1) + " dBFS",
        "  " + (fs.statSync(file).size / 1024 / 1024).toFixed(2) + " MB");
}
fs.writeFileSync(path.join(OUT, "music.json"), JSON.stringify(manifest, null, 4) + "\n");

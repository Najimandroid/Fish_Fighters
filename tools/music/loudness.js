// Integrated loudness (ITU-R BS.1770: K-weighting, 400 ms blocks, absolute and relative gates) of interleaved
// stereo samples at 48 kHz, in LUFS. Used to balance the stems (render_stems.js) and level the game files (export_music.js).

function loudness(samples, rate = 48000) {
    if (rate !== 48000) throw new Error("The K-weighting coefficients are for 48 kHz");
    const stages = [
        [[1.53512485958697, -2.69169618940638, 1.19839281085285], [-1.69065929318241, 0.73248077421585]],
        [[1, -2, 1], [-1.99004745483398, 0.99007225036621]],
    ];
    const frames = samples.length / 2, weighted = new Float32Array(samples.length);
    for (let ch = 0; ch < 2; ch++) {
        let source = samples;
        for (const [[b0, b1, b2], [a1, a2]] of stages) {
            let x1 = 0, x2 = 0, y1 = 0, y2 = 0;
            for (let i = 0; i < frames; i++) {
                const input = source[i * 2 + ch];
                const y = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
                x2 = x1; x1 = input; y2 = y1; y1 = y;
                weighted[i * 2 + ch] = y;
            }
            source = weighted;
        }
    }
    const block = Math.round(0.4 * rate), hop = Math.round(0.1 * rate), blocks = [];
    for (let start = 0; start + block <= frames; start += hop) {
        let sum = 0;
        for (let i = start * 2; i < (start + block) * 2; i++) sum += weighted[i] * weighted[i];
        blocks.push(sum / block);
    }
    const lufs = z => -0.691 + 10 * Math.log10(Math.max(z, 1e-12));
    const loud = blocks.filter(z => lufs(z) > -70);
    if (!loud.length) return -70;
    const relative = lufs(loud.reduce((a, b) => a + b) / loud.length) - 10;
    const gated = loud.filter(z => lufs(z) > relative);
    return lufs(gated.reduce((a, b) => a + b) / gated.length);
}

// Loudness above 300 Hz (a 2nd order high-pass first): what laptop and phone speakers play of the music
function speakerLoudness(samples, rate = 48000) {
    const w = 2 * Math.PI * 300 / rate, alpha = Math.sin(w) / (2 * 0.707), cos = Math.cos(w), a0 = 1 + alpha;
    const b0 = (1 + cos) / 2 / a0, b1 = -(1 + cos) / a0, b2 = b0, a1 = -2 * cos / a0, a2 = (1 - alpha) / a0;
    const high = new Float32Array(samples.length);
    for (let ch = 0; ch < 2; ch++) {
        let x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        for (let i = ch; i < samples.length; i += 2) {
            const x = samples[i], y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x; y2 = y1; y1 = y;
            high[i] = y;
        }
    }
    return loudness(high, rate);
}

// The themes' gain: to -16 LUFS, or more for a bass-heavy one, until its loudness above 300 Hz reaches the other
// themes' (-17.8 LUFS measured on them), never more than 3 dB above the -16 LUFS gain
const THEME_LUFS = -16, SPEAKER_LUFS = -17.8, MAX_SPEAKER_BOOST_DB = 3;
function themeGainDb(samples, rate = 48000) {
    const full = THEME_LUFS - loudness(samples, rate);
    const speaker = SPEAKER_LUFS - speakerLoudness(samples, rate);
    return Math.min(Math.max(full, speaker), full + MAX_SPEAKER_BOOST_DB);
}

module.exports = { loudness, speakerLoudness, themeGainDb };

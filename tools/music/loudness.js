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

module.exports = { loudness };

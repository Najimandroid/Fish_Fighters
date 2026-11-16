// A small PNG writer and reader for RGBA images (8 bits per channel), with node's zlib: enough for the art tools.

const fs = require("fs");
const zlib = require("zlib");

const CRC_TABLE = Array.from({ length: 256 }, (_, n) => { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; return c >>> 0; });
function crc32(buffers) {
    let c = 0xffffffff;
    for (const b of buffers) for (let i = 0; i < b.length; i++) c = CRC_TABLE[(c ^ b[i]) & 0xff] ^ (c >>> 8);
    return (c ^ 0xffffffff) >>> 0;
}
function chunk(type, data) {
    const length = Buffer.alloc(4), crc = Buffer.alloc(4), name = Buffer.from(type, "latin1");
    length.writeUInt32BE(data.length);
    crc.writeUInt32BE(crc32([name, data]));
    return Buffer.concat([length, name, data, crc]);
}

// { width, height, pixels (RGBA) } -> a PNG file
function writePng(file, { width, height, pixels }) {
    const rows = Buffer.alloc((width * 4 + 1) * height);
    for (let y = 0; y < height; y++) {
        rows[y * (width * 4 + 1)] = 0;   // No filter
        pixels.copy(rows, y * (width * 4 + 1) + 1, y * width * 4, (y + 1) * width * 4);
    }
    const header = Buffer.alloc(13);
    header.writeUInt32BE(width, 0); header.writeUInt32BE(height, 4);
    header[8] = 8; header[9] = 6; header[10] = 0; header[11] = 0; header[12] = 0;   // 8 bits, RGBA
    fs.writeFileSync(file, Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk("IHDR", header),
        chunk("IDAT", zlib.deflateSync(rows, { level: 9 })), chunk("IEND", Buffer.alloc(0))]));
}

// A PNG file (8-bit RGBA, RGB, grey or grey + alpha, not interlaced) -> { width, height, pixels (RGBA) }
function readPng(file) {
    const data = fs.readFileSync(file);
    let p = 8, width = 0, height = 0, type = 0, depth = 0, interlace = 0;
    const idat = [];
    while (p < data.length) {
        const length = data.readUInt32BE(p), name = data.toString("latin1", p + 4, p + 8), body = data.subarray(p + 8, p + 8 + length);
        if (name === "IHDR") { width = body.readUInt32BE(0); height = body.readUInt32BE(4); depth = body[8]; type = body[9]; interlace = body[12]; }
        if (name === "IDAT") idat.push(body);
        p += 12 + length;
    }
    const channels = { 0: 1, 2: 3, 4: 2, 6: 4 }[type];
    if (depth !== 8 || !channels || interlace) throw new Error(`${file}: only 8-bit, non-interlaced grey/RGB/RGBA PNGs are read`);
    const raw = zlib.inflateSync(Buffer.concat(idat)), stride = width * channels, pixels = Buffer.alloc(width * height * 4);
    const line = Buffer.alloc(stride), previous = Buffer.alloc(stride);
    for (let y = 0; y < height; y++) {
        const filter = raw[y * (stride + 1)], row = raw.subarray(y * (stride + 1) + 1, (y + 1) * (stride + 1));
        for (let i = 0; i < stride; i++) {
            const a = i >= channels ? line[i - channels] : 0, b = previous[i], c = i >= channels ? previous[i - channels] : 0;
            const pa = Math.abs(b - c), pb = Math.abs(a - c), pc = Math.abs(a + b - 2 * c);
            const predictor = [0, a, b, (a + b) >> 1, pa <= pb && pa <= pc ? a : pb <= pc ? b : c][filter];
            line[i] = (row[i] + predictor) & 0xff;
        }
        for (let x = 0; x < width; x++) {
            const s = x * channels, d = (y * width + x) * 4;
            if (channels >= 3) { pixels[d] = line[s]; pixels[d + 1] = line[s + 1]; pixels[d + 2] = line[s + 2]; }
            else pixels[d] = pixels[d + 1] = pixels[d + 2] = line[s];
            pixels[d + 3] = channels === 4 ? line[s + 3] : channels === 2 ? line[s + 1] : 255;
        }
        line.copy(previous);
    }
    return { width, height, pixels };
}

module.exports = { writePng, readPng };

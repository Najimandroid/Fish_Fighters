// Reads a FireAlpaca / MediBang Paint .mdp file: its canvas size and its layers, each as RGBA pixels on the canvas.
//
// The format: "mdipack\0", a uint32 (0), the XML's size and the binary part's size (uint32, little endian), the XML
// (the canvas and a <Layer> per layer: name, size, offset, opacity, visibility, the name of its pixel block), then
// "PAC " blocks: magic, block size, compression (1 = zlib), compressed size, raw size, padding to byte 68, a
// 64-byte name, the compressed data. A layer's pixels are BGRA. A tiled layer (binType 2, FireAlpaca's) stores only
// its non-empty tiles: a tile count and the tile size (128), then per tile its column, its row, 0, the length of its
// zlib data and that data (one tile of BGRA pixels), padded to 4 bytes.

const fs = require("fs");
const zlib = require("zlib");

const attribute = (tag, name) => { const m = new RegExp(`\\s${name}="([^"]*)"`).exec(tag); return m ? m[1] : null; };
const number = (tag, name, fallback = 0) => { const v = attribute(tag, name); return v === null ? fallback : Number(v); };

function readMdp(file) {
    const data = fs.readFileSync(file);
    if (data.toString("latin1", 0, 7) !== "mdipack") throw new Error(`${file} is not an .mdp file`);
    const xmlSize = data.readUInt32LE(12);
    const xml = data.toString("utf8", 20, 20 + xmlSize);

    // The pixel blocks, by name
    const blocks = new Map();
    for (let p = 20 + xmlSize; p + 132 <= data.length;) {
        if (data.toString("latin1", p, p + 4) !== "PAC ") throw new Error(`${file}: a broken block at byte ${p}`);
        const size = data.readUInt32LE(p + 4), compression = data.readUInt32LE(p + 8);
        const packed = data.readUInt32LE(p + 12), raw = data.readUInt32LE(p + 16);
        const name = data.toString("latin1", p + 68, p + 132).replace(/\0.*$/s, "");
        blocks.set(name, { compression, packed, raw, bytes: data.subarray(p + 132, p + 132 + packed) });
        p += size;
    }

    const canvas = /<Mdiapp\b[^>]*>/.exec(xml)[0];
    const width = number(canvas, "width"), height = number(canvas, "height");
    const layers = [];
    for (const tag of xml.match(/<Layer\b[^>]*\/?>/g) || []) {
        const layer = {
            name: attribute(tag, "name") || "",
            width: number(tag, "width"), height: number(tag, "height"),
            x: number(tag, "ofsx"), y: number(tag, "ofsy"),
            opacity: number(tag, "alpha", 255) / 255,
            visible: attribute(tag, "visible") !== "false",
            type: attribute(tag, "type"),
            binType: number(tag, "binType", 1),
            bin: attribute(tag, "bin"),
        };
        layer.pixels = () => layerPixels(file, layer, blocks.get(layer.bin), width, height);
        layers.push(layer);
    }
    return { width, height, layers };
}

// A layer's pixels as RGBA on the canvas (straight alpha; a layer reaching past the canvas is cropped to it)
function layerPixels(file, layer, block, width, height) {
    const out = Buffer.alloc(width * height * 4);
    if (!block) return out;   // An empty layer has no block
    if (layer.type && layer.type !== "32bpp") throw new Error(`${file}: layer "${layer.name}" is ${layer.type}, only 32bpp layers are read`);
    const stored = block.compression === 1 ? zlib.inflateSync(block.bytes) : block.bytes;
    const raw = layer.binType === 2 ? untile(stored, layer.width, layer.height) : stored;
    if (raw.length < layer.width * layer.height * 4) throw new Error(`${file}: layer "${layer.name}" has ${raw.length} bytes for ${layer.width}x${layer.height}`);
    for (let y = 0; y < layer.height; y++) {
        const cy = y + layer.y;
        if (cy < 0 || cy >= height) continue;
        for (let x = 0; x < layer.width; x++) {
            const cx = x + layer.x;
            if (cx < 0 || cx >= width) continue;
            const s = (y * layer.width + x) * 4, d = (cy * width + cx) * 4;
            out[d] = raw[s + 2]; out[d + 1] = raw[s + 1]; out[d + 2] = raw[s];
            out[d + 3] = Math.round(raw[s + 3] * layer.opacity);
        }
    }
    return out;
}

// A tiled layer's tiles put together into its width x height BGRA pixels (the missing tiles stay transparent)
function untile(stored, width, height) {
    const out = Buffer.alloc(width * height * 4);
    const count = stored.readUInt32LE(0), size = stored.readUInt32LE(4);
    for (let i = 0, p = 8; i < count; i++) {
        const column = stored.readUInt32LE(p), row = stored.readUInt32LE(p + 4), length = stored.readUInt32LE(p + 12);
        const tile = zlib.inflateSync(stored.subarray(p + 16, p + 16 + length));
        for (let y = 0; y < size; y++) {
            const ly = row * size + y;
            if (ly >= height) break;
            const x0 = column * size, n = Math.min(size, width - x0);
            if (n > 0) tile.copy(out, (ly * width + x0) * 4, y * size * 4, (y * size + n) * 4);
        }
        p += 16 + length + ((4 - (length % 4)) % 4);
    }
    return out;
}

module.exports = { readMdp };

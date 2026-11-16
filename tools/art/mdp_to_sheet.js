// Turns a FireAlpaca .mdp fish (or enemy) into the game's sprite sheet and its icon.
//
// In the .mdp, each frame is a layer named Walk1, Walk2..., Knockback1, Attack1, Attack2... (any other layer is a
// helper and is left out; hidden layers count). The sheet puts the frames side by side the way the game reads them:
// the walk frames (the first one is also the idle frame), the knockback frame, then the attack frames; a frame is the
// canvas (the fish at its size in the game). It prints the numbers to give the game: frameCount and
// knockbackFrameIndex (= the number of walk frames).
// An older file drawn as a whole sheet (the frames side by side on one wide canvas) works too: the frame width is
// guessed from where the frames are drawn, or given with --frame-width.
//
// The icon (200 x 150, like the deploy icons): the idle frame, mirrored to face right, zoomed so the fish is about
// 128 px tall (a long fish less, so it's at most 260 px long), its head near the right edge (x = 172) and the rest of
// it running off the left (a short fish is centered a little to the left instead), like the hand-made icons.
//
// Usage:
//   node tools/art/mdp_to_sheet.js fish.mdp [more.mdp...]   writes export/<name>_sheet.png and export/icon_<name>.png
//                                                          next to each file
//   options: --sheet out.png  --icon out.png  --no-icon  --frame-width px  --json (one line of JSON, for FishEditor)
//            --enemy (an enemy faces right: its icon isn't mirrored) / --fish (a fish: mirrored). Without either, a
//            path through an "enemies" folder (the drawing's, the sheet's or the icon's) means an enemy: the
//            drawings in ...\Enemies\, FishEditor's icons/enemies (an older FishEditor never passes --enemy)
// A sketch (one drawing, no Walk1.. layers) gives a placeholder sheet: the drawing as the walk, knockback and attack
// frame (3 frames, knockback frame 1).
//   node tools/art/mdp_to_sheet.js --icon-from-sheet sheet.png --frames N --icon out.png   (only the icon, from a sheet)
// tools/art/mdp_to_sheet.bat: drop .mdp files on it.

const fs = require("fs");
const path = require("path");
const { readMdp } = require("./mdp");
const { writePng, readPng } = require("./png");

const ICON = { width: 200, height: 150, fishHeight: 128, maxLength: 260, headX: 172, centerX: 90 };
const FRAME_LAYER = /^\s*(walk|knockback|attack)\s*(\d+)\s*$/i;

// ===== Images: { width, height, pixels (RGBA) } =====
const blank = (width, height) => ({ width, height, pixels: Buffer.alloc(width * height * 4) });

// The box around the visible pixels (null if there are none)
function contentBox({ width, height, pixels }, threshold = 8) {
    let x0 = width, y0 = height, x1 = -1, y1 = -1;
    for (let y = 0; y < height; y++)
        for (let x = 0; x < width; x++)
            if (pixels[(y * width + x) * 4 + 3] > threshold) { if (x < x0) x0 = x; if (x > x1) x1 = x; if (y < y0) y0 = y; if (y > y1) y1 = y; }
    return x1 < 0 ? null : { x: x0, y: y0, width: x1 - x0 + 1, height: y1 - y0 + 1 };
}

function crop(image, { x, y, width, height }) {
    const out = blank(width, height);
    for (let row = 0; row < height; row++) {
        const sy = y + row;
        if (sy < 0 || sy >= image.height) continue;
        for (let column = 0; column < width; column++) {
            const sx = x + column;
            if (sx < 0 || sx >= image.width) continue;
            image.pixels.copy(out.pixels, (row * width + column) * 4, (sy * image.width + sx) * 4, (sy * image.width + sx) * 4 + 4);
        }
    }
    return out;
}

function mirror(image) {
    const out = blank(image.width, image.height);
    for (let y = 0; y < image.height; y++)
        for (let x = 0; x < image.width; x++)
            image.pixels.copy(out.pixels, (y * image.width + image.width - 1 - x) * 4, (y * image.width + x) * 4, (y * image.width + x) * 4 + 4);
    return out;
}

// Copies an image onto another at (x, y), alpha-blended, cropped to the destination
function draw(target, image, x, y) {
    for (let row = 0; row < image.height; row++) {
        const ty = y + row;
        if (ty < 0 || ty >= target.height) continue;
        for (let column = 0; column < image.width; column++) {
            const tx = x + column;
            if (tx < 0 || tx >= target.width) continue;
            const s = (row * image.width + column) * 4, d = (ty * target.width + tx) * 4;
            const a = image.pixels[s + 3] / 255, b = target.pixels[d + 3] / 255, outA = a + b * (1 - a);
            if (outA <= 0) continue;
            for (let k = 0; k < 3; k++) target.pixels[d + k] = Math.round((image.pixels[s + k] * a + target.pixels[d + k] * b * (1 - a)) / outA);
            target.pixels[d + 3] = Math.round(outA * 255);
        }
    }
}

// Resizes with a bicubic (Catmull-Rom) filter, widened when shrinking, on premultiplied alpha (no dark fringes)
function resize(image, width, height) {
    const cubic = x => { x = Math.abs(x); return x < 1 ? 1.5 * x * x * x - 2.5 * x * x + 1 : x < 2 ? -0.5 * x * x * x + 2.5 * x * x - 4 * x + 2 : 0; };
    const pass = (src, sw, sh, dw, dh, horizontal) => {
        const out = new Float32Array(dw * dh * 4), n = horizontal ? sw : sh, m = horizontal ? dw : dh;
        const scale = n / m, support = 2 * Math.max(1, scale);
        for (let i = 0; i < m; i++) {
            const center = (i + 0.5) * scale - 0.5, first = Math.ceil(center - support), last = Math.floor(center + support);
            const weights = [];
            let total = 0;
            for (let j = first; j <= last; j++) { const w = cubic((j - center) / Math.max(1, scale)); weights.push([Math.min(n - 1, Math.max(0, j)), w]); total += w; }
            for (const lane of horizontal ? Array.from({ length: dh }, (_, y) => y) : Array.from({ length: dw }, (_, x) => x)) {
                const acc = [0, 0, 0, 0];
                for (const [j, w] of weights) {
                    const s = horizontal ? (lane * sw + j) * 4 : (j * sw + lane) * 4;
                    for (let k = 0; k < 4; k++) acc[k] += src[s + k] * w;
                }
                const d = horizontal ? (lane * dw + i) * 4 : (i * dw + lane) * 4;
                for (let k = 0; k < 4; k++) out[d + k] = acc[k] / total;
            }
        }
        return out;
    };
    const premultiplied = new Float32Array(image.width * image.height * 4);
    for (let i = 0; i < image.width * image.height; i++) {
        const a = image.pixels[i * 4 + 3] / 255;
        for (let k = 0; k < 3; k++) premultiplied[i * 4 + k] = image.pixels[i * 4 + k] * a;
        premultiplied[i * 4 + 3] = image.pixels[i * 4 + 3];
    }
    const wide = pass(premultiplied, image.width, image.height, width, image.height, true);
    const done = pass(wide, width, image.height, width, height, false);
    const out = blank(width, height);
    for (let i = 0; i < width * height; i++) {
        const a = Math.min(255, Math.max(0, done[i * 4 + 3]));
        for (let k = 0; k < 3; k++) out.pixels[i * 4 + k] = a > 0.5 ? Math.min(255, Math.max(0, Math.round(done[i * 4 + k] * 255 / a))) : 0;
        out.pixels[i * 4 + 3] = Math.round(a);
    }
    return out;
}

// The body inside a content box: the thin bits sticking out (whiskers, antennae, a fin's tip) trimmed off its sides,
// so the icon is framed on the fish and they can run past the edge
function bodyBox({ width, pixels }, box) {
    const solid = (x, y) => pixels[(y * width + x) * 4 + 3] > 8;
    const columns = Array.from({ length: box.width }, (_, i) => { let n = 0; for (let y = box.y; y < box.y + box.height; y++) n += solid(box.x + i, y); return n; });
    const rows = Array.from({ length: box.height }, (_, j) => { let n = 0; for (let x = box.x; x < box.x + box.width; x++) n += solid(x, box.y + j); return n; });
    const trim = (counts, limit) => { let a = 0, b = counts.length - 1; while (a < b && counts[a] < limit) a++; while (b > a && counts[b] < limit) b--; return [a, b]; };
    const [x0, x1] = trim(columns, Math.max(3, box.height * 0.07)), [y0, y1] = trim(rows, Math.max(3, box.width * 0.08));
    return { x: box.x + x0, y: box.y + y0, width: x1 - x0 + 1, height: y1 - y0 + 1 };
}

// ===== The icon =====
// isEnemy: an enemy faces right in its sheet and its icon keeps that (a fish faces left and is mirrored)
function makeIcon(frame, isEnemy = false) {
    const idle = isEnemy ? mirror(frame) : frame;
    const icon = blank(ICON.width, ICON.height);
    const box = contentBox(idle);
    if (!box) return icon;
    const body = bodyBox(idle, box);
    const fish = mirror(crop(idle, box));
    const scale = Math.min(ICON.fishHeight / body.height, ICON.maxLength / body.width);
    const width = Math.max(1, Math.round(box.width * scale)), height = Math.max(1, Math.round(box.height * scale));
    // Where the body is in the mirrored, scaled drawing
    const bodyLeft = (box.x + box.width - body.x - body.width) * scale, bodyTop = (body.y - box.y) * scale;
    const bodyWidth = body.width * scale, bodyHeight = body.height * scale;
    // The head (now on the right) near the right edge; a short fish centered a little to the left instead
    const left = Math.round(Math.min(ICON.headX - bodyWidth, ICON.centerX - bodyWidth / 2) - bodyLeft);
    draw(icon, resize(fish, width, height), left, Math.round((ICON.height - bodyHeight) / 2 - bodyTop));
    return icon;
}

// ===== The sheet =====
// The frame layers in the game's order, with what was left out and what looks wrong
function frameLayers(mdp, file) {
    const groups = { walk: [], knockback: [], attack: [] }, ignored = [], warnings = [];
    for (const layer of mdp.layers) {
        const m = FRAME_LAYER.exec(layer.name);
        if (!m) { ignored.push(layer.name); continue; }
        groups[m[1].toLowerCase()].push({ number: Number(m[2]), layer });
        if (layer.opacity < 1) warnings.push(`${layer.name} is at ${Math.round(layer.opacity * 100)} % opacity (kept)`);
    }
    for (const [kind, list] of Object.entries(groups)) {
        list.sort((a, b) => a.number - b.number);
        for (let i = 1; i < list.length; i++) {
            if (list[i].number === list[i - 1].number) throw new Error(`${file}: two layers are named ${list[i].layer.name}`);
            if (list[i].number !== list[i - 1].number + 1) warnings.push(`no ${kind} frame between ${list[i - 1].layer.name} and ${list[i].layer.name}`);
        }
    }
    // A sketch (no frame layers at all): its visible layers make one placeholder frame, used to walk, be knocked back
    // and attack until the frames are drawn
    if (!groups.walk.length && !groups.knockback.length && !groups.attack.length) {
        const drawing = mdp.layers.filter(layer => layer.visible);
        if (!drawing.length) throw new Error(`${file}: no visible layer to make a frame from`);
        warnings.push("a sketch (no Walk1, Knockback1, Attack1... layers): one placeholder frame for walking, knockback and the attack");
        return { sketch: drawing, walk: [drawing[0]], knockback: drawing[0], attack: [drawing[0]], ignored: [], warnings };
    }
    if (!groups.walk.length) throw new Error(`${file}: no Walk layer (name the frames Walk1, Walk2..., Knockback1, Attack1...)`);
    if (!groups.knockback.length) throw new Error(`${file}: no Knockback layer (the game needs one knockback frame, after the walk frames)`);
    if (groups.knockback.length > 1) warnings.push(`the game uses one knockback frame: ${groups.knockback.slice(1).map(f => f.layer.name).join(", ")} left out`);
    if (!groups.attack.length) warnings.push("no Attack layer: the attack will show no frames");
    return { walk: groups.walk.map(f => f.layer), knockback: groups.knockback[0].layer, attack: groups.attack.map(f => f.layer), ignored, warnings };
}

function convert(file, { sheet, icon, frameWidth = 0, isEnemy = false } = {}) {
    const mdp = readMdp(file);
    const layers = frameLayers(mdp, file);
    const ordered = [...layers.walk, layers.knockback, ...layers.attack];
    let canvases;
    if (layers.sketch) {
        // Its layers flattened into one frame (bottom first), three times: walk, knockback, attack
        const flat = blank(mdp.width, mdp.height);
        for (const layer of layers.sketch) draw(flat, { width: mdp.width, height: mdp.height, pixels: layer.pixels() }, 0, 0);
        canvases = ordered.map(() => flat);
    }
    else canvases = ordered.map(layer => ({ width: mdp.width, height: mdp.height, pixels: layer.pixels() }));

    // A file drawn as a whole sheet: each frame is cut from the slot its drawing is in
    let size = { width: mdp.width, height: mdp.height }, guessed = false;
    if (!frameWidth && mdp.width >= 4 * mdp.height && layers.walk.length >= 2) {
        const [a, b] = [contentBox(canvases[0]), contentBox(canvases[1])];
        if (a && b && b.x > a.x) { frameWidth = Math.round((b.x - a.x) / 10) * 10; guessed = true; }
    }
    let frames = canvases;
    if (frameWidth) {
        size = { width: frameWidth, height: mdp.height };
        frames = canvases.map(canvas => {
            const box = contentBox(canvas);
            const slot = box ? Math.floor((box.x + box.width / 2) / frameWidth) : 0;
            return crop(canvas, { x: slot * frameWidth, y: 0, width: frameWidth, height: mdp.height });
        });
    }

    const out = blank(size.width * frames.length, size.height);
    frames.forEach((frame, i) => draw(out, frame, i * size.width, 0));
    fs.mkdirSync(path.dirname(sheet), { recursive: true });
    writePng(sheet, out);
    if (icon) { fs.mkdirSync(path.dirname(icon), { recursive: true }); writePng(icon, makeIcon(frames[0], isEnemy)); }

    const empty = ordered.filter((layer, i) => !contentBox(frames[i])).map(layer => layer.name);
    if (empty.length) layers.warnings.push(`empty: ${empty.join(", ")}`);
    if (guessed) layers.warnings.push(`drawn as a whole sheet: frames of ${frameWidth} px guessed (--frame-width to change it)`);
    return {
        sheet, icon: icon || null, frameCount: frames.length, knockbackFrameIndex: layers.walk.length,
        walk: layers.walk.length, attack: layers.attack.length, frameWidth: size.width, frameHeight: size.height,
        sheetWidth: out.width, ignored: layers.ignored, warnings: layers.warnings,
    };
}

// ===== Command line =====
function main(args) {
    const option = name => { const i = args.indexOf(name); if (i < 0) return null; const v = args[i + 1]; args.splice(i, 2); return v; };
    const flag = name => { const i = args.indexOf(name); if (i < 0) return false; args.splice(i, 1); return true; };
    const json = flag("--json"), noIcon = flag("--no-icon"), forcesEnemy = flag("--enemy"), forcesFish = flag("--fish");
    const fromSheet = option("--icon-from-sheet"), frames = Number(option("--frames") || 1);
    const sheetOut = option("--sheet"), iconOut = option("--icon"), frameWidth = Number(option("--frame-width") || 0);

    // An enemy faces right and keeps it in its icon; a fish faces left and is mirrored. Said by --enemy / --fish, or
    // read from the paths: an "enemies" (or "enemy") folder on the way
    const inEnemyFolder = (...paths) => paths.some(p => p && path.resolve(p).split(/[\\/]+/).slice(0, -1).some(part => /^enem(y|ies)$/i.test(part)));
    const enemyFor = file => forcesEnemy || (!forcesFish && inEnemyFolder(file, sheetOut, iconOut, fromSheet));
    const isEnemy = enemyFor(null);

    if (fromSheet) {
        if (!iconOut) throw new Error("--icon-from-sheet needs --icon out.png");
        const image = readPng(fromSheet), width = Math.floor(image.width / Math.max(1, frames));
        writePng(iconOut, makeIcon(crop(image, { x: 0, y: 0, width, height: image.height }), isEnemy));
        console.log(json ? JSON.stringify({ icon: iconOut }) : `Icon: ${iconOut}`);
        return;
    }
    const files = args.filter(a => !a.startsWith("--"));
    if (!files.length) throw new Error("Usage: node tools/art/mdp_to_sheet.js fish.mdp [--sheet out.png] [--icon out.png | --no-icon] [--frame-width px] [--json]");
    if (files.length > 1 && (sheetOut || iconOut)) throw new Error("--sheet and --icon name one file: give one .mdp with them");

    for (const file of files) {
        const name = path.basename(file, path.extname(file)), folder = path.join(path.dirname(file), "export");
        const result = convert(file, {
            sheet: sheetOut || path.join(folder, `${name}_sheet.png`),
            icon: noIcon ? null : iconOut || path.join(folder, `icon_${name}.png`),
            frameWidth, isEnemy: enemyFor(file),
        });
        if (json) { console.log(JSON.stringify(result)); continue; }
        console.log(`${path.basename(file)}: ${result.walk} walk + 1 knockback + ${result.attack} attack = ${result.frameCount} frames of ${result.frameWidth} x ${result.frameHeight}`);
        console.log(`  sheet: ${result.sheet} (${result.sheetWidth} x ${result.frameHeight})`);
        if (result.icon) console.log(`  icon:  ${result.icon}`);
        console.log(`  in the game (FishEditor): Frames ${result.frameCount}, Knockback frame ${result.knockbackFrameIndex}`);
        if (result.ignored.length) console.log(`  left out (not frames): ${result.ignored.join(", ")}`);
        for (const warning of result.warnings) console.log(`  note: ${warning}`);
    }
}

if (require.main === module) {
    try { main(process.argv.slice(2)); }
    catch (error) {
        if (process.argv.includes("--json")) console.log(JSON.stringify({ error: error.message }));
        else console.error(error.message);
        process.exit(1);
    }
}

module.exports = { convert, makeIcon };

// Fish Fighters for Android: builds SFML for Android, the game, packages the APK and installs it on a phone.
//
//   node tools/android/android.js sfml               SFML 3 built for Android (once): into the NDK's sources/third_party/sfml
//   node tools/android/android.js build [release]    The game (CMake preset android-debug / android-release)
//   node tools/android/android.js apk [release]      The APK (aapt2, zipalign, apksigner with the debug key)
//   node tools/android/android.js install            adb install on the phone plugged in (USB debugging on), then starts it
//   node tools/android/android.js all [release]      build + apk + install
//
// Where the tools are (environment variables, else the folders under ~/android-tools):
//   ANDROID_HOME (the SDK: platform-tools, build-tools, platforms, ndk), ANDROID_NDK, JAVA_HOME, SFML_ANDROID_SRC,
//   CMAKE (cmake.exe), NINJA (ninja.exe)
const fs = require("fs");
const os = require("os");
const path = require("path");
const { spawnSync } = require("child_process");

const ROOT = path.resolve(__dirname, "..", "..");
const TOOLS = path.join(os.homedir(), "android-tools");
const PACKAGE = "com.fishfighters.game";
const ABI = "arm64-v8a";
const MIN_SDK = 26;
const TARGET_SDK = 34;

function first_existing(...candidates) {
    for (const candidate of candidates) if (candidate && fs.existsSync(candidate)) return candidate;
    return candidates.find(Boolean);
}
function newest_dir(parent) {
    if (!fs.existsSync(parent)) return null;
    const dirs = fs.readdirSync(parent).filter((name) => fs.statSync(path.join(parent, name)).isDirectory()).sort();
    return dirs.length ? path.join(parent, dirs[dirs.length - 1]) : null;
}

const SDK = first_existing(process.env.ANDROID_HOME, process.env.ANDROID_SDK_ROOT, path.join(TOOLS, "sdk"));
const NDK = first_existing(process.env.ANDROID_NDK, newest_dir(path.join(SDK, "ndk")));
const JAVA_HOME = first_existing(process.env.JAVA_HOME, newest_dir(path.join(TOOLS, "jdk")));
const SFML_SRC = first_existing(process.env.SFML_ANDROID_SRC, newest_dir(path.join(TOOLS, "src")));
const VS_CMAKE = "C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake";
const CMAKE = first_existing(process.env.CMAKE, path.join(VS_CMAKE, "CMake/bin/cmake.exe"), "cmake");
const NINJA = first_existing(process.env.NINJA, path.join(VS_CMAKE, "Ninja/ninja.exe"), "ninja");
const BUILD_TOOLS = newest_dir(path.join(SDK, "build-tools"));
const exe = (name) => (process.platform === "win32" ? name + ".exe" : name);
const bat = (name) => (process.platform === "win32" ? name + ".bat" : name);

function run(command, args, options = {}) {
    console.log(`> ${path.basename(command)} ${args.join(" ")}`);
    const env = { ...process.env, JAVA_HOME, ANDROID_HOME: SDK, ANDROID_NDK: NDK, PATH: path.join(JAVA_HOME, "bin") + path.delimiter + process.env.PATH };
    const isBatch = command.endsWith(".bat");
    const result = spawnSync(command, args, { stdio: "inherit", cwd: options.cwd || ROOT, env, shell: isBatch });
    if (result.status !== 0 && !options.allowFail) {
        console.error(`failed: ${path.basename(command)} (${result.status ?? result.error})`);
        process.exit(1);
    }
    return result.status;
}

function build_dir(config) { return path.join(ROOT, "out", "build", `android-${config}`); }   // The presets' folder

// SFML 3.0's Android file stream crashes on a file that isn't in the APK (an assert, compiled out, then a null
// pointer) instead of failing to open it like on PC; the game loads optional files (portraits, settings.json...).
// This makes a missing file fail to open. Applied once to the SFML source (marked, so it isn't applied twice)
function patch_sfml() {
    const file = path.join(SFML_SRC, "src/SFML/System/Android/ResourceStream.cpp");
    let source = fs.readFileSync(file, "utf8");
    if (source.includes("Fish Fighters patch")) return;
    const guard = "    if (!m_file)\n        return std::nullopt; // Fish Fighters patch: a missing file\n";
    const edits = [
        ['    assert(m_file && "Failed to initialize ResourceStream file");', "    // Fish Fighters patch: a missing file leaves m_file empty (FileInputStream::open fails)"],
        ["    const auto numBytesRead = AAsset_read(", guard + "    const auto numBytesRead = AAsset_read("],
        ["    const auto newPosition = AAsset_seek(", guard + "    const auto newPosition = AAsset_seek("],
        ["    return getSize().value() - ", guard + "    return getSize().value() - "],
        ["    return AAsset_getLength(m_file.get());", guard + "    return AAsset_getLength(m_file.get());"],
    ];
    for (const [from, to] of edits) {
        if (!source.includes(from)) { console.error(`SFML patch: can't find ${from}`); process.exit(1); }
        source = source.replace(from, to);
    }
    fs.writeFileSync(file, source);
    console.log("SFML patched: a missing file fails to open instead of crashing");
}

// SFML 3 for Android: shared libraries, installed where the NDK looks (SFML's own rule)
function sfml() {
    patch_sfml();
    const dir = path.join(TOOLS, "build", "sfml-android");
    run(CMAKE, ["-S", SFML_SRC, "-B", dir, "-G", "Ninja", `-DCMAKE_MAKE_PROGRAM=${NINJA}`,
        `-DCMAKE_TOOLCHAIN_FILE=${path.join(NDK, "build/cmake/android.toolchain.cmake")}`,
        `-DANDROID_ABI=${ABI}`, `-DANDROID_PLATFORM=android-${MIN_SDK}`, "-DANDROID_STL=c++_shared",
        "-DCMAKE_BUILD_TYPE=Release", "-DSFML_BUILD_NETWORK=OFF", "-DSFML_BUILD_EXAMPLES=OFF",
        "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"]);   // SFML's FreeType asks for a CMake older than VS's accepts
    run(CMAKE, ["--build", dir]);
    run(CMAKE, ["--install", dir]);
}

function build(config) {
    run(CMAKE, ["--preset", `android-${config}`, `-DCMAKE_MAKE_PROGRAM=${NINJA}`]);
    run(CMAKE, ["--build", build_dir(config)]);
}

// The APK by hand (no Gradle: a NativeActivity has no Java code): resources and assets with aapt2, the .so files
// added, aligned, signed with the debug key (~/.android/debug.keystore, made once)
function apk(config) {
    const out = build_dir(config);
    const stage = path.join(out, "apk");
    fs.rmSync(stage, { recursive: true, force: true });
    fs.mkdirSync(path.join(stage, "lib", ABI), { recursive: true });

    // Libraries: the game, SFML's, the C++ runtime
    const sfmlLibs = path.join(NDK, "sources/third_party/sfml/lib", ABI);
    const libs = [path.join(out, "libfishfighters.so")];
    for (const name of fs.readdirSync(sfmlLibs)) if (/^libsfml-(system|window|graphics|audio)\.so$/.test(name)) libs.push(path.join(sfmlLibs, name));
    const prebuilt = newest_dir(path.join(NDK, "toolchains/llvm/prebuilt"));
    libs.push(path.join(prebuilt, "sysroot/usr/lib/aarch64-linux-android/libc++_shared.so"));
    for (const lib of libs) fs.copyFileSync(lib, path.join(stage, "lib", ABI, path.basename(lib)));

    // Assets: the game's assets/ and game_data/ (without the player's save), under the APK's assets/
    const assets = path.join(stage, "assets");
    fs.cpSync(path.join(ROOT, "assets"), path.join(assets, "assets"), { recursive: true });
    fs.cpSync(path.join(ROOT, "game_data"), path.join(assets, "game_data"), { recursive: true,
        filter: (source) => !/player\.json$|settings\.json$|errors\.log$/.test(source) });

    const unsigned = path.join(out, "fishfighters-unsigned.apk");
    const aligned = path.join(out, "fishfighters-aligned.apk");
    const final = path.join(out, `FishFighters-${config}.apk`);
    // Resources: the launcher icon is the game's icon
    const manifest = path.join(ROOT, "android", "AndroidManifest.xml");
    const res = path.join(out, "res");
    fs.rmSync(res, { recursive: true, force: true });
    fs.mkdirSync(path.join(res, "mipmap"), { recursive: true });
    fs.copyFileSync(path.join(ROOT, "assets/images/app/game_icon.png"), path.join(res, "mipmap", "ic_launcher.png"));
    // The theme: fullscreen, and on Android 9+ drawn under the camera cutout too (the layout keeps its buttons in
    // the middle 16:9; only the backgrounds go there)
    const theme = (cutout) => `<?xml version="1.0" encoding="utf-8"?>\n<resources>\n    <style name="FishTheme" parent="@android:style/Theme.NoTitleBar.Fullscreen">\n`
        + (cutout ? `        <item name="android:windowLayoutInDisplayCutoutMode">shortEdges</item>\n` : "") + `    </style>\n</resources>\n`;
    fs.mkdirSync(path.join(res, "values"), { recursive: true });
    fs.mkdirSync(path.join(res, "values-v28"), { recursive: true });
    fs.writeFileSync(path.join(res, "values", "styles.xml"), theme(false));
    fs.writeFileSync(path.join(res, "values-v28", "styles.xml"), theme(true));
    const compiled = path.join(out, "res.zip");
    run(path.join(BUILD_TOOLS, exe("aapt2")), ["compile", "--dir", res, "-o", compiled]);
    run(path.join(BUILD_TOOLS, exe("aapt2")), ["link", "-o", unsigned, "--manifest", manifest,
        "-I", path.join(SDK, "platforms", `android-${TARGET_SDK}`, "android.jar"), compiled,
        "--min-sdk-version", String(MIN_SDK), "--target-sdk-version", String(TARGET_SDK), "--auto-add-overlay",
        ...(config === "debug" ? ["--debug-mode"] : [])]);
    // The libraries and the assets, stored as they are: Android maps the libraries straight from the APK, and the
    // music streams from it (aapt2 -A would write Windows' \ in the paths, which Android can't find)
    add_to_apk(unsigned, stage, ["lib", "assets"]);
    run(path.join(BUILD_TOOLS, exe("zipalign")), ["-f", "-p", "4", unsigned, aligned]);

    const keystore = path.join(os.homedir(), ".android", "debug.keystore");
    if (!fs.existsSync(keystore)) {
        fs.mkdirSync(path.dirname(keystore), { recursive: true });
        run(path.join(JAVA_HOME, "bin", exe("keytool")), ["-genkeypair", "-keystore", keystore, "-storepass", "android",
            "-alias", "androiddebugkey", "-keypass", "android", "-keyalg", "RSA", "-keysize", "2048", "-validity", "10000",
            "-dname", "CN=Android Debug,O=Android,C=US"]);
    }
    run(path.join(BUILD_TOOLS, bat("apksigner")), ["sign", "--ks", keystore, "--ks-pass", "pass:android",
        "--key-pass", "pass:android", "--out", final, aligned]);
    console.log(`APK: ${final}`);
    return final;
}

// Adds folders of stage to the APK, stored (not compressed), with the JDK's jar tool (always / in the paths)
function add_to_apk(apkPath, stage, folders) {
    run(path.join(JAVA_HOME, "bin", exe("jar")), ["uf0", apkPath, ...folders.flatMap((folder) => ["-C", stage, folder])]);
}

// Stops early (before a long build) when no phone is plugged in with USB debugging on
function require_phone() {
    const adb = path.join(SDK, "platform-tools", exe("adb"));
    const result = spawnSync(adb, ["devices"], { encoding: "utf8" });
    const phones = (result.stdout || "").split(/\r?\n/).slice(1).filter((line) => /\tdevice$/.test(line));
    if (phones.length === 0) {
        console.error("No phone found: plug it in with USB, turn on USB debugging (Developer options) and accept the prompt on the phone.");
        process.exit(1);
    }
    console.log(`Phone: ${phones[0].split("\t")[0]}`);
}

function install(config) {
    const adb = path.join(SDK, "platform-tools", exe("adb"));
    const apkPath = path.join(build_dir(config), `FishFighters-${config}.apk`);
    run(adb, ["install", "-r", apkPath]);
    run(adb, ["shell", "am", "start", "-n", `${PACKAGE}/android.app.NativeActivity`]);
}

const [step = "all", configArg] = process.argv.slice(2);
const config = configArg === "release" ? "release" : "debug";
console.log(`SDK ${SDK}\nNDK ${NDK}\nJDK ${JAVA_HOME}`);
if (step === "sfml") sfml();
else if (step === "build") build(config);
else if (step === "apk") apk(config);
else if (step === "install") install(config);
else if (step === "all") { require_phone(); build(config); apk(config); install(config); }
else { console.error("steps: sfml | build | apk | install | all"); process.exit(1); }

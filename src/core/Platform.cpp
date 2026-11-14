#include "Platform.h"

#include <filesystem>
#include <fstream>

#if defined(__ANDROID__)
#include <SFML/System/FileInputStream.hpp>
#include <SFML/System/NativeActivity.hpp>
#include <android/native_activity.h>
#endif

namespace
{
#if defined(__ANDROID__)
    // A file of the APK, read whole with SFML (its streams read the APK's assets on Android)
    bool read_from_apk(const std::string& path, std::string& text)
    {
        sf::FileInputStream stream;
        if (!stream.open(path)) return false;
        const std::optional<std::size_t> size = stream.getSize();
        if (!size) return false;
        text.assign(*size, '\0');
        return *size == 0 || stream.read(text.data(), *size) == *size;
    }
#endif
}

std::stringstream Platform::open(const std::string& path)
{
    std::stringstream text;
    if (std::ifstream file(writable(path), std::ios::binary); file)
    {
        text << file.rdbuf();
        text.clear();   // An empty file isn't an error
        return text;
    }
#if defined(__ANDROID__)
    if (std::string data; read_from_apk(path, data))
    {
        text.str(data);
        return text;
    }
#endif
    text.setstate(std::ios::failbit);
    return text;
}

std::string Platform::writable(const std::string& path)
{
#if defined(__ANDROID__)
    static const std::filesystem::path root = sf::getNativeActivity()->internalDataPath;
    const std::filesystem::path full = root / path;
    std::error_code error;
    std::filesystem::create_directories(full.parent_path(), error);
    return full.string();
#else
    return path;
#endif
}

bool Platform::exists(const std::string& path)
{
#if defined(__ANDROID__)
    std::string unused;
    std::error_code error;
    return std::filesystem::exists(writable(path), error) || read_from_apk(path, unused);
#else
    return std::filesystem::exists(path);
#endif
}

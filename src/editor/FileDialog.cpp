#include "FileDialog.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>

#include <string>

namespace FileDialog
{
    std::optional<std::filesystem::path> open_png(const sf::Window& owner, const char* title)
    {
        wchar_t fileName[MAX_PATH] = L"";

        // Dialog title, widened (titles are plain ASCII)
        std::wstring wideTitle;
        for (const char* c = title; *c; ++c) wideTitle += static_cast<wchar_t>(*c);

        OPENFILENAMEW dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.hwndOwner = static_cast<HWND>(owner.getNativeHandle());
        dialog.lpstrFilter = L"PNG images (*.png)\0*.png\0";
        dialog.lpstrFile = fileName;
        dialog.nMaxFile = MAX_PATH;
        dialog.lpstrTitle = wideTitle.c_str();
        dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

        if (!GetOpenFileNameW(&dialog)) return std::nullopt;
        return std::filesystem::path(fileName);
    }
}

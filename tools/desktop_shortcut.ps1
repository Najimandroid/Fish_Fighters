# Creates or updates a desktop shortcut to a program that was just built.
# Called after every build of FishEditor (CMake option FISH_EDITOR_DESKTOP_SHORTCUT), so the shortcut
# always opens the latest build. It never fails the build: problems are only reported.
#
# Usage: powershell -NoProfile -ExecutionPolicy Bypass -File desktop_shortcut.ps1 -Target <exe> -Name <shortcut name> [-Description <text>]
param(
    [Parameter(Mandatory = $true)][string]$Target,
    [Parameter(Mandatory = $true)][string]$Name,
    [string]$Description = ""
)

try {
    $target = [System.IO.Path]::GetFullPath($Target)
    if (-not (Test-Path $target)) { throw "$target doesn't exist" }

    # The real desktop folder (it can be moved, e.g. into OneDrive)
    $desktop = [Environment]::GetFolderPath("Desktop")
    $path = Join-Path $desktop "$Name.lnk"

    $shell = New-Object -ComObject WScript.Shell
    $shortcut = $shell.CreateShortcut($path)
    $shortcut.TargetPath = $target
    $shortcut.WorkingDirectory = Split-Path $target   # Its layout file (fish_editor.ini) stays next to the build
    $shortcut.IconLocation = "$target,0"              # The icon built into the exe
    if ($Description) { $shortcut.Description = $Description }
    $shortcut.Save()

    Write-Output "Desktop shortcut '$Name' -> $target"
}
catch {
    Write-Output "warning: the desktop shortcut '$Name' could not be updated: $($_.Exception.Message)"
}
exit 0

@echo off
rem Drop FireAlpaca .mdp files on this file: each becomes a sprite sheet and an icon, in an "export" folder next to it
rem (layers named Walk1, Walk2..., Knockback1, Attack1...; see mdp_to_sheet.js)
node "%~dp0mdp_to_sheet.js" %*
echo.
pause

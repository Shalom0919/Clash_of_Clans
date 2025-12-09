@echo off
cd /d "%~dp0"
PowerShell -NoProfile -ExecutionPolicy Bypass -Command "& './Script.ps1'"
pause
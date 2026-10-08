<#
.SYNOPSIS
    Builds VYRA Bible and produces the Windows setup (release\VYRA-Bible-Setup-<version>.exe).

.DESCRIPTION
    1. configures and builds the plugin with the "windows-x64" CMake preset
    2. installs it into a staging folder (release\stage)
    3. runs Inno Setup on installer\vyra-bible.iss

    Prerequisites (Windows 64 bits):
      - Visual Studio 2022 with the "Desktop development with C++" workload
      - CMake 3.28 or later
      - Inno Setup 6.3 or later  (winget install JRSoftware.InnoSetup)
    The first configure downloads the OBS sources and Qt6 (internet required, a few hundred MB).

.EXAMPLE
    .\scripts\build-installer.ps1
#>
[CmdletBinding()]
param(
    [string]$Config = "RelWithDebInfo",
    [string]$Version
)

$ErrorActionPreference = "Stop"

function Invoke-Checked {
    param([string]$Description, [scriptblock]$Command)
    Write-Host "==> $Description" -ForegroundColor Cyan
    & $Command
    if ($LASTEXITCODE -ne 0) {
        throw "Step failed ($Description), exit code $LASTEXITCODE"
    }
}

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $root

if (-not $Version) {
    $Version = (Get-Content (Join-Path $root "buildspec.json") -Raw | ConvertFrom-Json).version
}

$stage = Join-Path $root "release\stage"
$buildDir = Join-Path $root "build_x64"

Invoke-Checked "Configure (preset windows-x64)" { cmake --preset windows-x64 }
Invoke-Checked "Build ($Config)" { cmake --build --preset windows-x64 --config $Config }

if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
Invoke-Checked "Stage files" { cmake --install $buildDir --config $Config --prefix $stage }

# The setup must carry everything the plugin needs at run time: refuse to build an incomplete one.
$required = @(
    "vyra-bible\bin\64bit\vyra-bible.dll",
    "vyra-bible\data\bibles\lsg1910.tsv",
    "vyra-bible\data\overlay\index.html",
    "vyra-bible\data\overlay\overlay.css",
    "vyra-bible\data\overlay\overlay.js",
    "vyra-bible\data\locale\fr-FR.ini",
    "vyra-bible\data\locale\en-US.ini"
)
$missing = $required | Where-Object { -not (Test-Path (Join-Path $stage $_)) }
if ($missing) {
    throw "Staged files missing (setup not built): $($missing -join ', ')"
}
Write-Host "==> Staged files complete ($($required.Count) checked)" -ForegroundColor Cyan

$iscc = (Get-Command iscc -ErrorAction SilentlyContinue).Source
if (-not $iscc) {
    $candidates = @(
        "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
        "${env:ProgramFiles}\Inno Setup 6\ISCC.exe",
        "${env:LOCALAPPDATA}\Programs\Inno Setup 6\ISCC.exe"
    )
    $iscc = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $iscc) {
    throw "Inno Setup (ISCC.exe) not found. Install it with: winget install JRSoftware.InnoSetup"
}

Invoke-Checked "Build setup (Inno Setup)" {
    & $iscc "/DAppVersion=$Version" "/DStageDir=$stage" (Join-Path $root "installer\vyra-bible.iss")
}

$setup = Join-Path $root "release\VYRA-Bible-Setup-$Version.exe"
if (-not (Test-Path $setup)) { throw "Expected setup not found: $setup" }

# A zip for people who cannot (or do not want to) run a setup: portable OBS, or no administrator rights.
# It holds the OBS folder layout, so it is simply extracted over the OBS folder.
$zip = Join-Path $root "release\VYRA-Bible-$Version-manual.zip"
$zipRoot = Join-Path $root "release\zip"
if (Test-Path $zipRoot) { Remove-Item $zipRoot -Recurse -Force }
New-Item -ItemType Directory -Force -Path (Join-Path $zipRoot "obs-plugins\64bit"), (Join-Path $zipRoot "data\obs-plugins\vyra-bible") | Out-Null
Copy-Item (Join-Path $stage "vyra-bible\bin\64bit\*") (Join-Path $zipRoot "obs-plugins\64bit") -Recurse
Copy-Item (Join-Path $stage "vyra-bible\data\*") (Join-Path $zipRoot "data\obs-plugins\vyra-bible") -Recurse
Copy-Item (Join-Path $root "docs\INSTALLATION.md") (Join-Path $zipRoot "LISEZMOI-INSTALLATION.md")
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $zipRoot "*") -DestinationPath $zip
Remove-Item $zipRoot -Recurse -Force

# Checksums, so that a download can be checked.
$sums = foreach ($f in @($setup, $zip)) { "{0}  {1}" -f (Get-FileHash $f -Algorithm SHA256).Hash.ToLower(), (Split-Path $f -Leaf) }
Set-Content -Path (Join-Path $root "release\SHA256SUMS.txt") -Value $sums -Encoding ascii
Write-Host ($sums -join [Environment]::NewLine)
Write-Host "Done: $setup" -ForegroundColor Green

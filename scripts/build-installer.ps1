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
Write-Host "Done: $setup" -ForegroundColor Green

[CmdletBinding()]
param([switch]$SkipBuild, [switch]$SmokeTest, [string]$IsccPath, [string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($env:OS -ne 'Windows_NT') { throw 'Build the Windows installer on Windows.' }
if ($SmokeTest -and $env:GITHUB_ACTIONS -ne 'true') { throw 'Installer smoke tests require a disposable GitHub Actions runner.' }
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root 'build\windows-x64'
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $root 'build\packages' }
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
if (-not $IsccPath) {
    $compiler = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($compiler) { $IsccPath = $compiler.Source }
    else {
        foreach ($base in @(${env:ProgramFiles(x86)}, $env:ProgramFiles)) {
            if ($base) {
                $candidate = Join-Path $base 'Inno Setup 6\ISCC.exe'
                if (Test-Path -LiteralPath $candidate -PathType Leaf) { $IsccPath = $candidate; break }
            }
        }
    }
}
if (-not $IsccPath -or -not (Test-Path -LiteralPath $IsccPath -PathType Leaf)) {
    throw 'Install Inno Setup 6.7.4 from https://jrsoftware.org/isdl.php or supply -IsccPath.'
}

# Reuse the bundle architecture/metadata checks and complete licence staging.
# StageOnly creates neither a portable ZIP nor an application executable.
& (Join-Path $root 'Package-VST3.ps1') -SkipBuild:$SkipBuild -StageOnly
$stage = Join-Path $build 'vst3-package-stage'
$unexpected = @(Get-ChildItem -LiteralPath (Join-Path $stage 'RaBASS.vst3') -File -Recurse -Filter '*.exe')
if ($unexpected.Count -ne 0) { throw 'A VST3-only installer must not contain application executables.' }
$match = [regex]::Match((Get-Content (Join-Path $root 'CMakeLists.txt') -Raw), 'project\(RaBASS VERSION (\d+\.\d+\.\d+) LANGUAGES')
if (-not $match.Success) { throw 'Cannot read RaBASS version.' }
$version = $match.Groups[1].Value
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$installer = Join-Path $OutputDirectory "RaBASS-$version-Windows-x64-Setup.exe"
if (Test-Path -LiteralPath $installer) { Remove-Item -LiteralPath $installer -Force }
& $IsccPath "/DAppVersion=$version" "/DStageDir=$stage" "/DOutputDir=$OutputDirectory" (Join-Path $PSScriptRoot 'RaBASS.iss')
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $installer -PathType Leaf)) { throw 'Inno Setup compilation failed.' }

$files = @([IO.Path]::GetFileName($installer), 'RaBASS-Source.zip', 'JUCE-8.0.6-Source.zip')
if ($SmokeTest) {
    & (Join-Path $PSScriptRoot 'Smoke-Test-Installer.ps1') -Installer $installer -Version $version -ReportPath (Join-Path $OutputDirectory 'installer-test-results.json')
    $files += 'installer-test-results.json'
}
$juce = Join-Path $build '_deps\juce-src'
& git -C $root archive --format=zip "--output=$(Join-Path $OutputDirectory 'RaBASS-Source.zip')" HEAD
if ($LASTEXITCODE -ne 0) { throw 'RaBASS source archive failed.' }
& git -C $juce archive --format=zip "--output=$(Join-Path $OutputDirectory 'JUCE-8.0.6-Source.zip')" HEAD
if ($LASTEXITCODE -ne 0) { throw 'JUCE source archive failed.' }
$sourceCommit = & git -C $root rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Cannot read source commit.' }
$checksums = foreach ($file in $files) {
    $hash = (Get-FileHash -LiteralPath (Join-Path $OutputDirectory $file) -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $file"
}
[IO.File]::WriteAllLines((Join-Path $OutputDirectory 'SHA256SUMS.txt'), $checksums, [Text.Encoding]::ASCII)
@{ version = $version; sourceCommit = $sourceCommit; format = 'VST3'; standalone = $false; files = $files } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $OutputDirectory 'release-info.json') -Encoding UTF8
Write-Host "WINDOWS VST3 INSTALLER READY: $installer"

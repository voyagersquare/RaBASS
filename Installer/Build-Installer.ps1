[CmdletBinding()]
param([switch]$SkipBuild, [string]$IsccPath, [string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($env:OS -ne 'Windows_NT') { throw 'Build this installer on Windows.' }
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root 'build\windows-x64'
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $root 'build\installer' }
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
if (-not $IsccPath) {
    $compiler = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($compiler) { $IsccPath = $compiler.Source }
    else {
        foreach ($base in @(${env:ProgramFiles(x86)}, $env:ProgramFiles)) {
            if ($base) {
                foreach ($major in @(7, 6)) {
                    $candidate = Join-Path $base "Inno Setup $major\ISCC.exe"
                    if (Test-Path -LiteralPath $candidate -PathType Leaf) { $IsccPath = $candidate; break }
                }
            }
            if ($IsccPath) { break }
        }
    }
}
if (-not $IsccPath -or -not (Test-Path -LiteralPath $IsccPath -PathType Leaf)) {
    throw 'Install Inno Setup 6.3 or newer from https://jrsoftware.org/isdl.php or supply -IsccPath.'
}
if (-not $SkipBuild) {
    & (Join-Path $root 'Build-Windows.cmd') --no-pause
    if ($LASTEXITCODE -ne 0) { throw 'Windows build failed. No installer was created.' }
}
$cmake = Get-Content -LiteralPath (Join-Path $root 'CMakeLists.txt') -Raw
$match = [regex]::Match($cmake, 'project\(RaBASS VERSION (\d+\.\d+\.\d+) LANGUAGES')
if (-not $match.Success) { throw 'Cannot read RaBASS version.' }
$version = $match.Groups[1].Value
$bundle = Join-Path $build 'RaBASS_artefacts\Release\VST3\RaBASS.vst3'
$module = Join-Path $bundle 'Contents\x86_64-win\RaBASS.vst3'
if (-not (Test-Path -LiteralPath $module -PathType Leaf)) { throw "Missing module: $module" }
$reader = New-Object IO.BinaryReader([IO.File]::OpenRead($module))
try {
    if ($reader.BaseStream.Length -lt 128 -or $reader.ReadUInt16() -ne 0x5A4D) { throw 'Invalid Windows binary.' }
    $reader.BaseStream.Position = 0x3C
    $offset = $reader.ReadUInt32()
    if ($offset -gt $reader.BaseStream.Length - 6) { throw 'Invalid PE header.' }
    $reader.BaseStream.Position = $offset
    if ($reader.ReadUInt32() -ne 0x4550 -or $reader.ReadUInt16() -ne 0x8664) { throw 'Expected an x64 PE module.' }
} finally { $reader.Dispose() }
function ConvertFrom-VstJson([string]$Text) {
    # The SDK may write trailing commas. Strip them only outside quoted strings.
    $builder = New-Object Text.StringBuilder
    $quoted = $false; $escaped = $false
    for ($i = 0; $i -lt $Text.Length; $i++) {
        $char = $Text[$i]
        if ($quoted) {
            if ($escaped) { $escaped = $false }
            elseif ($char -eq '\') { $escaped = $true }
            elseif ($char -eq '"') { $quoted = $false }
        } elseif ($char -eq '"') { $quoted = $true }
        elseif ($char -eq ',') {
            $next = $i + 1
            while ($next -lt $Text.Length -and [char]::IsWhiteSpace($Text[$next])) { $next++ }
            if ($next -lt $Text.Length -and ($Text[$next] -eq '}' -or $Text[$next] -eq ']')) { continue }
        }
        [void]$builder.Append($char)
    }
    return ($builder.ToString() | ConvertFrom-Json)
}
$info = ConvertFrom-VstJson (Get-Content -LiteralPath (Join-Path $bundle 'Contents\Resources\moduleinfo.json') -Raw)
if ($info.Name -ne 'RaBASS' -or $info.Version -ne $version -or $info.'Factory Info'.Vendor -ne 'Independent Audio') {
    throw 'VST3 metadata is stale or belongs to a different product.'
}
$stage = Join-Path $build 'installer-stage'
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
foreach ($dir in @('VST3','Docs','Licenses')) { New-Item -ItemType Directory -Path (Join-Path $stage $dir) -Force | Out-Null }
Copy-Item -LiteralPath $bundle -Destination (Join-Path $stage 'VST3') -Recurse
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Install-Info.txt'), (Join-Path $PSScriptRoot 'Third-Party-Notices.txt') -Destination (Join-Path $stage 'Docs')
Copy-Item -LiteralPath (Join-Path $root 'LICENSE') -Destination (Join-Path $stage 'Licenses\RaBASS-AGPL-3.0.txt')
$juce = Join-Path $build '_deps\juce-src'
if (-not (Test-Path -LiteralPath (Join-Path $juce 'LICENSE.md'))) { throw 'JUCE licence tree missing.' }
@((Get-Item -LiteralPath (Join-Path $juce 'LICENSE.md')); (Get-ChildItem -LiteralPath (Join-Path $juce 'modules') -File -Recurse | Where-Object { $_.Name -match '(?i)licen[cs]e|^copying' })) | ForEach-Object {
    $relative = $_.FullName.Substring($juce.Length + 1)
    $destination = Join-Path (Join-Path $stage 'Licenses\JUCE') $relative
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath $_.FullName -Destination $destination
}
foreach ($relative in @('modules/juce_graphics/image_formats/jpglib/README','modules/juce_graphics/image_formats/pnglib/png.h','modules/juce_core/zip/zlib/zlib.h')) {
    $destination = Join-Path (Join-Path $stage 'Licenses\JUCE') $relative
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $juce $relative) -Destination $destination
}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$output = Join-Path $OutputDirectory "RaBASS-$version-Windows-x64-Setup.exe"
foreach ($file in @($output,"$output.sha256")) { if (Test-Path -LiteralPath $file) { Remove-Item -LiteralPath $file -Force } }
& $IsccPath "/DAppVersion=$version" "/DStageDir=$stage" "/DOutputDir=$OutputDirectory" (Join-Path $PSScriptRoot 'RaBASS.iss')
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $output -PathType Leaf)) { throw 'Inno Setup compilation failed.' }
$hash = (Get-FileHash -LiteralPath $output -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText("$output.sha256", "$hash  $([IO.Path]::GetFileName($output))`r`n", [Text.Encoding]::ASCII)
Write-Host "INSTALLER READY: $output"

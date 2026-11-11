[CmdletBinding()]
param([switch]$SkipBuild, [switch]$StageOnly, [string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($env:OS -ne 'Windows_NT') { throw 'Package the Windows VST3 on Windows.' }
$root = $PSScriptRoot
$build = Join-Path $root 'build\windows-x64'
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $root 'build\packages' }
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
if (-not $SkipBuild) {
    & (Join-Path $root 'Build-Windows.cmd') --no-pause
    if ($LASTEXITCODE -ne 0) { throw 'Windows build or tests failed.' }
}
$match = [regex]::Match((Get-Content (Join-Path $root 'CMakeLists.txt') -Raw), 'project\(RaBASS VERSION (\d+\.\d+\.\d+) LANGUAGES')
if (-not $match.Success) { throw 'Cannot read RaBASS version.' }
$version = $match.Groups[1].Value
$bundle = Join-Path $build 'RaBASS_artefacts\Release\VST3\RaBASS.vst3'
$module = Join-Path $bundle 'Contents\x86_64-win\RaBASS.vst3'
if (-not (Test-Path -LiteralPath $module -PathType Leaf)) { throw 'The built Windows VST3 module is missing.' }
$reader = New-Object IO.BinaryReader([IO.File]::OpenRead($module))
try {
    if ($reader.BaseStream.Length -lt 128 -or $reader.ReadUInt16() -ne 0x5A4D) { throw 'Invalid Windows binary.' }
    $reader.BaseStream.Position = 0x3C
    $offset = $reader.ReadUInt32()
    if ($offset -gt $reader.BaseStream.Length - 6) { throw 'Invalid PE header.' }
    $reader.BaseStream.Position = $offset
    if ($reader.ReadUInt32() -ne 0x4550 -or $reader.ReadUInt16() -ne 0x8664) { throw 'Expected an x64 VST3 module.' }
} finally { $reader.Dispose() }
function ConvertFrom-VstJson([string]$Text) {
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
    throw 'The VST3 metadata does not match this version.'
}
$juce = Join-Path $build '_deps\juce-src'
if (-not (Test-Path -LiteralPath (Join-Path $juce 'LICENSE.md'))) { throw 'The JUCE source and licence tree is missing.' }
$stage = Join-Path $build 'vst3-package-stage'
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Path (Join-Path $stage 'Licenses\JUCE') -Force | Out-Null
Copy-Item -LiteralPath $bundle -Destination $stage -Recurse
Copy-Item -LiteralPath (Join-Path $root 'README.md') -Destination $stage
Copy-Item -LiteralPath (Join-Path $root 'LICENSE'), (Join-Path $root 'ThirdParty\LICENSE-AGPL-3.0.txt'), (Join-Path $root 'ThirdParty\NOTICES.txt') -Destination (Join-Path $stage 'Licenses')
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
if ($StageOnly) {
    Write-Host "VERIFIED VST3 STAGE READY: $stage"
    return
}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$portable = Join-Path $OutputDirectory "RaBASS-$version-Windows-x64-VST3.zip"
if (Test-Path -LiteralPath $portable) { Remove-Item -LiteralPath $portable -Force }
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$stream = [IO.File]::Create($portable)
$zip = [IO.Compression.ZipArchive]::new($stream, [IO.Compression.ZipArchiveMode]::Create)
try {
    Get-ChildItem -LiteralPath $stage -File -Recurse | ForEach-Object {
        # ZIP entry names always use forward slashes, including when built by
        # Windows PowerShell 5.1. Compress-Archive can retain backslashes.
        $entryName = $_.FullName.Substring($stage.Length + 1).Replace('\', '/')
        [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $_.FullName, $entryName, [IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
} finally { $zip.Dispose(); $stream.Dispose() }
$verification = [IO.Compression.ZipFile]::OpenRead($portable)
try {
    if ($null -eq $verification.GetEntry('RaBASS.vst3/Contents/x86_64-win/RaBASS.vst3')) {
        throw 'The portable ZIP has an invalid VST3 bundle layout.'
    }
} finally { $verification.Dispose() }
& git -C $root archive --format=zip "--output=$(Join-Path $OutputDirectory 'RaBASS-Source.zip')" HEAD
if ($LASTEXITCODE -ne 0) { throw 'RaBASS source archive failed.' }
& git -C $juce archive --format=zip "--output=$(Join-Path $OutputDirectory 'JUCE-8.0.6-Source.zip')" HEAD
if ($LASTEXITCODE -ne 0) { throw 'JUCE source archive failed.' }
$sourceCommit = & git -C $root rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Cannot read source commit.' }
$files = @([IO.Path]::GetFileName($portable), 'RaBASS-Source.zip', 'JUCE-8.0.6-Source.zip')
$checksums = foreach ($file in $files) {
    $hash = (Get-FileHash -LiteralPath (Join-Path $OutputDirectory $file) -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $file"
}
[IO.File]::WriteAllLines((Join-Path $OutputDirectory 'SHA256SUMS.txt'), $checksums, [Text.Encoding]::ASCII)
@{ version = $version; sourceCommit = $sourceCommit; files = $files } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $OutputDirectory 'release-info.json') -Encoding UTF8
Write-Host "PORTABLE VST3 READY: $portable"

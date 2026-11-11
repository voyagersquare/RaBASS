[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Installer,
    [Parameter(Mandatory=$true)][string]$Version,
    [Parameter(Mandatory=$true)][string]$ReportPath
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($env:GITHUB_ACTIONS -ne 'true' -or $env:OS -ne 'Windows_NT') {
    throw 'Run install/uninstall tests only on a disposable GitHub Actions Windows runner.'
}
$root = Split-Path -Parent $PSScriptRoot
$source = [IO.Path]::GetFullPath((Join-Path $root 'build\windows-x64\RaBASS_artefacts\Release\VST3\RaBASS.vst3'))
$bundle = Join-Path $env:CommonProgramW6432 'VST3\RaBASS.vst3'
$app = Join-Path $env:ProgramW6432 'Independent Audio\RaBASS'
$registryPath = 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{70CBCE73-9A92-4C0C-8A4C-91E374B9CB90}_is1'
if ((Test-Path -LiteralPath $bundle) -or (Test-Path -LiteralPath $app) -or (Test-Path -LiteralPath $registryPath)) {
    throw 'A previous RaBASS installation exists on this runner; refusing to modify it.'
}
$presetFolder = Join-Path $env:APPDATA 'Independent Audio\RaBASS\Presets'
New-Item -ItemType Directory -Path $presetFolder -Force | Out-Null
$preset = Join-Path $presetFolder ('installer-preservation-' + [guid]::NewGuid().ToString() + '.rabasspreset')
[IO.File]::WriteAllText($preset, 'User preset preservation fixture: installer must leave these bytes untouched.')
$presetHash = (Get-FileHash -LiteralPath $preset -Algorithm SHA256).Hash

function Assert-PresetPreserved {
    if (-not (Test-Path -LiteralPath $preset -PathType Leaf) -or (Get-FileHash -LiteralPath $preset -Algorithm SHA256).Hash -ne $presetHash) {
        throw 'The installer or uninstaller changed the user preset fixture.'
    }
}
function Invoke-Setup([string]$Exe, [string]$Phase) {
    $log = Join-Path $env:RUNNER_TEMP "RaBASS-install-$Phase.log"
    $process = Start-Process -FilePath $Exe -ArgumentList '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-',"/LOG=`"$log`"" -Wait -PassThru
    if ($process.ExitCode -ne 0) { throw "Setup phase $Phase exited with $($process.ExitCode). See $log" }
}
function Assert-Installed {
    Get-ChildItem -LiteralPath $source -File -Recurse | ForEach-Object {
        $relative = $_.FullName.Substring($source.Length + 1)
        $installed = Join-Path $bundle $relative
        if (-not (Test-Path -LiteralPath $installed -PathType Leaf) -or (Get-FileHash -LiteralPath $_.FullName).Hash -ne (Get-FileHash -LiteralPath $installed).Hash) {
            throw "Installed VST3 file differs from the tested build: $relative"
        }
    }
    $entry = Get-ItemProperty -LiteralPath $registryPath
    if ($entry.DisplayName -ne 'RaBASS' -or $entry.DisplayVersion -ne $Version) { throw 'Windows Installed apps registration is incorrect.' }
    if (-not (Test-Path -LiteralPath (Join-Path $app 'Documentation\Install-Info.txt'))) { throw 'Installation documentation is missing.' }
    if (-not (Test-Path -LiteralPath (Join-Path $app 'Licenses\LICENSE-AGPL-3.0.txt'))) { throw 'Installation licence is missing.' }
    $executables = @(Get-ChildItem -LiteralPath $app -File -Recurse -Filter '*.exe')
    if ($executables.Count -ne 1 -or $executables[0].Name -notmatch '^unins[0-9]+\.exe$') { throw 'Only the Windows uninstaller may be an executable in the installation folder.' }
    if (@(Get-ChildItem -LiteralPath $bundle -File -Recurse -Filter '*.exe').Count -ne 0) { throw 'A standalone application was installed in the VST3 bundle.' }
    Assert-PresetPreserved
}
function Uninstall-And-Check([string]$Phase) {
    Invoke-Setup (Join-Path $app 'Uninstall\unins000.exe') $Phase
    for ($attempt=0; $attempt -lt 100 -and (Test-Path -LiteralPath $app); $attempt++) { Start-Sleep -Milliseconds 100 }
    if ((Test-Path -LiteralPath (Join-Path $bundle 'Contents\x86_64-win\RaBASS.vst3')) -or (Test-Path -LiteralPath $registryPath)) {
        throw 'Uninstall left the VST3 module or Windows application registration behind.'
    }
    if (Test-Path -LiteralPath $app) { throw 'Uninstall left installation files behind.' }
    Assert-PresetPreserved
}
try {
    Invoke-Setup $Installer 'fresh'
    Assert-Installed
    Invoke-Setup $Installer 'reinstall'
    Assert-Installed
    Uninstall-And-Check 'uninstall'

    # Model an existing manually copied VST3: its module must be overwritten.
    New-Item -ItemType Directory -Path (Join-Path $bundle 'Contents\x86_64-win') -Force | Out-Null
    [IO.File]::WriteAllText((Join-Path $bundle 'Contents\x86_64-win\RaBASS.vst3'), 'Previous portable VST3 fixture')
    Invoke-Setup $Installer 'portable-upgrade'
    Assert-Installed
    Uninstall-And-Check 'portable-upgrade-uninstall'

    $report = [ordered]@{
        version = $Version
        platform = 'Windows x64'
        format = 'VST3'
        checks = [ordered]@{
            freshInstall = $true
            installedBundleChecksums = $true
            reinstall = $true
            portableUpgrade = $true
            installedAppsRegistration = $true
            vst3Only = $true
            uninstall = $true
            userPresetsPreserved = $true
        }
    }
    $report | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $ReportPath -Encoding UTF8
    Write-Host 'PASS: fresh install, bundle checksums, reinstall, portable upgrade, VST3-only payload, uninstall and user preset preservation'
} finally {
    if (Test-Path -LiteralPath $preset) { Remove-Item -LiteralPath $preset -Force }
}

[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Installer)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($env:GITHUB_ACTIONS -ne 'true') { throw 'Run this install/uninstall smoke test only on a disposable GitHub Actions Windows runner.' }
$bundle = Join-Path $env:CommonProgramW6432 'VST3\RaBASS.vst3'
$app = Join-Path $env:ProgramW6432 'Independent Audio\RaBASS'
if (Test-Path -LiteralPath $bundle) { throw 'A previous RaBASS installation exists on this runner.' }
function Invoke-Setup([string]$Exe,[string]$Log) {
    $process = Start-Process -FilePath $Exe -ArgumentList '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-',"/LOG=`"$Log`"" -Wait -PassThru
    if ($process.ExitCode -ne 0) { throw "Setup exited with $($process.ExitCode). See $Log" }
}
Invoke-Setup $Installer (Join-Path $env:RUNNER_TEMP 'RaBASS-install-fresh.log')
$installed = Join-Path $bundle 'Contents\x86_64-win\RaBASS.vst3'
if (-not (Test-Path -LiteralPath $installed -PathType Leaf)) { throw 'VST3 was not installed.' }
$source = Join-Path $PSScriptRoot '..\build\windows-x64\RaBASS_artefacts\Release\VST3\RaBASS.vst3'
Get-ChildItem -LiteralPath $source -File -Recurse | ForEach-Object {
    $relative = $_.FullName.Substring([IO.Path]::GetFullPath($source).Length + 1)
    $destination = Join-Path $bundle $relative
    if ((Get-FileHash -LiteralPath $_.FullName).Hash -ne (Get-FileHash -LiteralPath $destination).Hash) { throw "Installed file differs: $relative" }
}
# Reinstall exercises the upgrade path without changing the stable application ID.
Invoke-Setup $Installer (Join-Path $env:RUNNER_TEMP 'RaBASS-install-upgrade.log')
$uninstaller = Join-Path $app 'Uninstall\unins000.exe'
Invoke-Setup $uninstaller (Join-Path $env:RUNNER_TEMP 'RaBASS-install-uninstall.log')
if (Test-Path -LiteralPath $installed) { throw 'Uninstall left the VST3 module behind.' }
Write-Host 'PASS: installation, bundle checksums, upgrade and uninstall'

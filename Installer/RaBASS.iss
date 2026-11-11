#ifndef AppVersion
  #error AppVersion is required
#endif
#ifndef StageDir
  #error StageDir is required
#endif
#ifndef OutputDir
  #error OutputDir is required
#endif
#if !FileExists(StageDir + "\RaBASS.vst3\Contents\x86_64-win\RaBASS.vst3")
  #error Missing Windows x64 VST3 module
#endif

[Setup]
; Retain the original RaBASS 0.1.0 identity for in-place installer upgrades.
AppId={{70CBCE73-9A92-4C0C-8A4C-91E374B9CB90}
AppName=RaBASS
AppVersion={#AppVersion}
AppVerName=RaBASS {#AppVersion}
AppPublisher=Independent Audio
AppPublisherURL=https://github.com/voyagersquare/RaBASS
AppSupportURL=https://github.com/voyagersquare/RaBASS/issues
VersionInfoVersion={#AppVersion}.0
VersionInfoCompany=Independent Audio
VersionInfoDescription=RaBASS VST3 Bass Instrument Installer
DefaultDirName={autopf}\Independent Audio\RaBASS
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableWelcomePage=no
UsePreviousAppDir=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
PrivilegesRequired=admin
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
OutputDir={#OutputDir}
OutputBaseFilename=RaBASS-{#AppVersion}-Windows-x64-Setup
LicenseFile=..\ThirdParty\LICENSE-AGPL-3.0.txt
InfoBeforeFile=Install-Info.txt
SetupLogging=yes
CloseApplications=yes
CloseApplicationsFilter=*.exe,*.dll,*.vst3
RestartApplications=no
UninstallDisplayName=RaBASS
UninstallFilesDir={app}\Uninstall
Uninstallable=yes
ChangesAssociations=no
ChangesEnvironment=no

[Languages]
Name: english; MessagesFile: compiler:Default.isl

[Files]
Source: "{#StageDir}\RaBASS.vst3\*"; DestDir: "{commoncf64}\VST3\RaBASS.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#StageDir}\README.md"; DestDir: "{app}\Documentation"; Flags: ignoreversion
Source: "Install-Info.txt"; DestDir: "{app}\Documentation"; Flags: ignoreversion
Source: "{#StageDir}\Licenses\*"; DestDir: "{app}\Licenses"; Flags: ignoreversion recursesubdirs createallsubdirs

; VST3 instrument only: no standalone program, shortcuts or launch action.
; AppData user presets and DAW projects are never installed or removed here.

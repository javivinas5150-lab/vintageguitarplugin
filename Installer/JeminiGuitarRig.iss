; Installer for Jemini Guitar Rig - VST3 + Standalone, Windows 64-bit
; Compile with Inno Setup 6:  ISCC.exe JeminiGuitarRig.iss   (or open it in Inno Setup and press Ctrl+F9)
;
; Same layout as the pedal installers (JeminiPedal.iss.inc), but the rig is
; built from the repository root project, so it has its own paths.
;   VST3       -> C:\Program Files\Common Files\VST3\JeminiGuitarRig.vst3
;   Standalone -> C:\Program Files\Jemini\Jemini Guitar Rig\JeminiGuitarRig.exe  (+ Start menu)
; Static C++ runtime: no VC++ redistributable needed. Presets in %APPDATA% are kept.

#define AppCode      "JeminiGuitarRig"
#define AppName      "Jemini Guitar Rig"
#define AppIdGuid    "{14C21C67-2DFD-4CBA-AD88-39BB752CD7B7}"
#define AppVersion   "0.1.0"
#define Publisher    "Javi Vinas"
; AppIdGuid must never change: upgrades and uninstall rely on it.

#define BuildDir      "..\Builds\VisualStudio2026\x64\Release"
#define Vst3Bundle    BuildDir + "\VST3\" + AppCode + ".vst3"
#define StandaloneExe BuildDir + "\Standalone Plugin\" + AppCode + ".exe"

#if !FileExists(StandaloneExe)
  #error Standalone build not found - build the Release configuration in Visual Studio first
#endif
#if !FileExists(Vst3Bundle + "\Contents\x86_64-win\" + AppCode + ".vst3")
  #error VST3 build not found - build the Release configuration in Visual Studio first
#endif

[Setup]
AppId={{#AppIdGuid}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#Publisher}
DefaultDirName={autopf}\Jemini\{#AppName}
DefaultGroupName=Jemini
DisableProgramGroupPage=yes
UninstallDisplayName={#AppName}
UninstallDisplayIcon={app}\{#AppCode}.exe
OutputDir=Output
OutputBaseFilename={#AppCode}_Setup_{#AppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
CloseApplications=yes
VersionInfoVersion={#AppVersion}
VersionInfoCompany={#Publisher}
VersionInfoDescription={#AppName} installer

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Types]
Name: "full";   Description: "VST3 + Standalone"
Name: "custom"; Description: "Custom"; Flags: iscustom

[Components]
Name: "vst3";       Description: "VST3 plugin (Cubase and other DAWs)"; Types: full custom
Name: "standalone"; Description: "Standalone application";               Types: full custom

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Components: standalone; Flags: unchecked

[InstallDelete]
; Replace an older VST3 bundle completely so no stale files remain
Type: filesandordirs; Name: "{commoncf64}\VST3\{#AppCode}.vst3"; Components: vst3

[Files]
; VST3 bundle (whole folder)
Source: "{#Vst3Bundle}\*"; DestDir: "{commoncf64}\VST3\{#AppCode}.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
; Standalone
Source: "{#StandaloneExe}"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}";            Filename: "{app}\{#AppCode}.exe"; Components: standalone
Name: "{group}\Uninstall {#AppName}";  Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}";      Filename: "{app}\{#AppCode}.exe"; Components: standalone; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppCode}.exe"; Description: "{cm:LaunchProgram,{#AppName}}"; Components: standalone; Flags: nowait postinstall skipifsilent unchecked

[UninstallDelete]
; The VST3 bundle folder (presets and recordings are left alone)
Type: filesandordirs; Name: "{commoncf64}\VST3\{#AppCode}.vst3"

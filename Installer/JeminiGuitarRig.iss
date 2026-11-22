; ============================================================================
;  Jemini Guitar Rig - Windows installer (Inno Setup 6)
;
;  Build: compile the solution in Release x64 first, then open this file in
;  Inno Setup Compiler and press Compile (Ctrl+F9). The setup .exe is written
;  to Installer\Output.
; ============================================================================

#define AppName      "Jemini Guitar Rig"
#define AppVersion   "1.0.0"
#define AppPublisher "Javi Vinas"
#define AppExe       "JeminiGuitarRig.exe"
#define BuildDir     "..\Builds\VisualStudio2026\x64\Release"

[Setup]
AppId={{14C21C67-2DFD-4CBA-AD88-39BB752CD7B7}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=Output
OutputBaseFilename=JeminiGuitarRig-{#AppVersion}-Setup-Win64
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#AppName}
UninstallDisplayIcon={app}\{#AppExe}
VersionInfoVersion={#AppVersion}
VersionInfoCompany={#AppPublisher}
VersionInfoDescription={#AppName} installer

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full";   Description: "{cm:FullInstall}"
Name: "custom"; Description: "{cm:CustomInstall}"; Flags: iscustom

[Components]
Name: "vst3";       Description: "VST3 plugin (Cubase, etc.)";  Types: full custom
Name: "standalone"; Description: "{cm:StandaloneApp}";          Types: full custom

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Components: standalone

[Files]
; VST3 bundle -> C:\Program Files\Common Files\VST3
Source: "{#BuildDir}\VST3\JeminiGuitarRig.vst3\*"; DestDir: "{commoncf64}\VST3\JeminiGuitarRig.vst3"; \
    Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3
; Standalone application -> C:\Program Files\Jemini Guitar Rig
Source: "{#BuildDir}\Standalone Plugin\{#AppExe}"; DestDir: "{app}"; Flags: ignoreversion; Components: standalone

[Icons]
Name: "{group}\{#AppName}";                       Filename: "{app}\{#AppExe}"; Components: standalone
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}";                 Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,{#AppName}}"; \
    Flags: nowait postinstall skipifsilent; Components: standalone

[InstallDelete]
; Remove an older copy of the VST3 bundle so no stale files are left behind
Type: filesandordirs; Name: "{commoncf64}\VST3\JeminiGuitarRig.vst3"; Components: vst3

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\JeminiGuitarRig.vst3"

[CustomMessages]
spanish.FullInstall=Instalación completa
spanish.CustomInstall=Instalación personalizada
spanish.StandaloneApp=Aplicación standalone (sin DAW)
english.FullInstall=Full installation
english.CustomInstall=Custom installation
english.StandaloneApp=Standalone application (no DAW needed)

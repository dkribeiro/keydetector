; Inno Setup script for KeyDetector (VST3, Windows x64).
; Build the plugin in Release first, then compile this with Inno Setup 6:
;   ISCC.exe packaging\windows\installer.iss

#define MyAppName "KeyDetector"
#define MyAppVersion "0.1.0"

[Setup]
AppName={#MyAppName}
AppVersion={#MyAppVersion}
DefaultDirName={autopf}\Common Files\VST3
DisableDirPage=yes
ArchitecturesInstallIn64BitMode=x64
OutputBaseFilename=KeyDetector-Windows-Setup
Compression=lzma2
SolidCompression=yes
PrivilegesRequired=admin

[Files]
Source: "..\..\build\KeyDetector_artefacts\Release\VST3\KeyDetector.vst3\*"; \
  DestDir: "{commoncf64}\VST3\KeyDetector.vst3"; \
  Flags: recursesubdirs createallsubdirs ignoreversion

[Run]

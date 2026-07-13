; Instalador de Windows para LaDistorneta (Inno Setup).
; Instala el VST3 en la carpeta estandar del sistema.
; En GitHub Actions se compila con: iscc installer\LaDistorneta.iss

[Setup]
AppName=LaDistorneta
AppVersion=0.0.1
AppPublisher=HappyDickAudio
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename=LaDistorneta-0.0.1-Windows
Compression=lzma2
SolidCompression=yes

[Files]
; El .vst3 de Windows es una carpeta (bundle), se copia entera
Source: "..\build\LaDistorneta_artefacts\Release\VST3\LaDistorneta.vst3\*"; DestDir: "{commoncf64}\VST3\LaDistorneta.vst3"; Flags: recursesubdirs ignoreversion

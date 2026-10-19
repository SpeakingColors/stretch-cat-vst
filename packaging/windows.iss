; Windows installer + uninstaller (Inno Setup). Called as: ISCC /DName=StretchCat "/DProduct=Stretch Cat" /DVersion=0.1.0 packaging/windows.iss
; Files are staged in ..\stage by the release workflow. Works offline.
; The uninstaller removes everything it installed. Your saved presets (%APPDATA%\Studio 87\StretchCat) are kept
; unless you answer Yes when it asks. A silent uninstall (/SILENT, /VERYSILENT) always keeps them.
[Setup]
AppName={#Product}
AppVersion={#Version}
AppPublisher=Studio 87
AppPublisherURL=https://github.com/SpeakingColors
DefaultDirName={autopf}\Studio 87\{#Product}
DefaultGroupName=Studio 87
OutputDir=..\Output
OutputBaseFilename={#Name}-Setup
LicenseFile=..\LICENSE
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
Compression=lzma2
SolidCompression=yes
UninstallDisplayName={#Product}
UninstallDisplayIcon={app}\{#Product}.exe
CloseApplications=yes

[Files]
Source: "..\stage\vst3\*"; DestDir: "{commoncf64}\VST3\{#Product}.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion
Source: "..\stage\{#Product}.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\stage\{#Name}-manual.pdf"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#Product}"; Filename: "{app}\{#Product}.exe"
Name: "{group}\{#Product} manual"; Filename: "{app}\{#Name}-manual.pdf"
Name: "{group}\Uninstall {#Product}"; Filename: "{uninstallexe}"

[UninstallDelete]
; the plugin folder is removed by the uninstall log; these catch anything the program wrote next to its files
Type: filesandordirs; Name: "{commoncf64}\VST3\{#Product}.vst3"
Type: filesandordirs; Name: "{app}"
; standalone app settings (audio device choice only, no presets); unverified exact location, removed if present
Type: filesandordirs; Name: "{userappdata}\{#Product}"
Type: files; Name: "{userappdata}\{#Product}.settings"

[Code]
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  presets, vendor: String;
begin
  if CurUninstallStep <> usPostUninstall then exit;
  presets := ExpandConstant('{userappdata}\Studio 87\{#Name}');
  vendor := ExpandConstant('{userappdata}\Studio 87');
  if DirExists(presets) and (not UninstallSilent) then
    if MsgBox('Also delete your saved {#Product} presets?' + #13#10 + #13#10 + presets + #13#10 + #13#10 +
              'Choose No to keep them (recommended if you plan to reinstall).', mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES then
      begin
        DelTree(presets, True, True, True);
      end;
  RemoveDir(vendor);   { only succeeds when empty }
end;

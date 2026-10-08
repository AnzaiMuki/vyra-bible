; VYRA Bible - Windows installer (Inno Setup 6.3 or later)
;
; Build (done by scripts\build-installer.ps1, which passes the two defines):
;   ISCC.exe /DAppVersion=0.1.0 /DStageDir=C:\...\release\stage installer\vyra-bible.iss
;
; StageDir is the result of `cmake --install --prefix <StageDir>`:
;   <StageDir>\vyra-bible\bin\64bit\vyra-bible.dll
;   <StageDir>\vyra-bible\data\locale\*.ini

#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif
#ifndef StageDir
  #define StageDir "..\release\stage"
#endif

#define AppName "VYRA Bible"
#define Publisher "VYRA Concept"

[Setup]
; Never change this GUID: it identifies the product for upgrades and uninstall.
AppId={{EE77695A-271A-4879-8D8B-3FB2EBB45A72}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#Publisher}
VersionInfoVersion={#AppVersion}
; The "application folder" is the OBS Studio folder: the plugin must live inside it.
DefaultDirName={code:GetObsDir}
DirExistsWarning=no
DisableProgramGroupPage=yes
UsePreviousAppDir=no
; Keep OBS's folder clean: the uninstaller lives with the plugin data.
UninstallFilesDir={app}\data\obs-plugins\vyra-bible
UninstallDisplayName={#AppName} (OBS Studio plugin)
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
LicenseFile=..\LICENSE
SetupIconFile=vyra.ico
UninstallDisplayIcon={app}\data\obs-plugins\vyra-bible\vyra.ico
WizardImageFile=wizard-large-1x.bmp,wizard-large-2x.bmp
WizardSmallImageFile=wizard-small-55.bmp,wizard-small-110.bmp
AppCopyright=Copyright (C) 2026 VYRA Concept
VersionInfoCompany={#Publisher}
VersionInfoDescription={#AppName} setup
VersionInfoProductName={#AppName}
OutputDir=..\release
OutputBaseFilename=VYRA-Bible-Setup-{#AppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
; If OBS is running and holds the DLL (update case), offer to close it.
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"; InfoAfterFile: "after-fr.txt"
Name: "english"; MessagesFile: "compiler:Default.isl"; InfoAfterFile: "after-en.txt"

[CustomMessages]
french.ObsNotFound=Le dossier choisi ne contient pas OBS Studio 64 bits (bin\64bit\obs64.exe est introuvable).%n%nChoisissez le dossier d'installation d'OBS Studio, par exemple C:\Program Files\obs-studio.
english.ObsNotFound=The selected folder does not contain 64-bit OBS Studio (bin\64bit\obs64.exe was not found).%n%nPick the OBS Studio installation folder, for example C:\Program Files\obs-studio.
french.SelectDirHint=Dossier d'installation d'OBS Studio
english.SelectDirHint=OBS Studio installation folder

[Files]
Source: "{#StageDir}\vyra-bible\bin\64bit\*"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion
Source: "{#StageDir}\vyra-bible\data\*"; DestDir: "{app}\data\obs-plugins\vyra-bible"; Flags: ignoreversion recursesubdirs createallsubdirs

Source: "vyra.ico"; DestDir: "{app}\data\obs-plugins\vyra-bible"; Flags: ignoreversion

[Code]
// Finds the OBS Studio folder. The OBS installer records it in the registry;
// when that key is missing we fall back to the default location.
function GetObsDir(Param: String): String;
var
  Path: String;
begin
  if RegQueryStringValue(HKLM64, 'SOFTWARE\OBS Studio', '', Path) and DirExists(Path) then
    Result := Path
  else
    Result := ExpandConstant('{commonpf64}\obs-studio');
end;

// Refuse a folder that is not OBS Studio: installing elsewhere would do nothing useful.
function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if CurPageID = wpSelectDir then
  begin
    if not FileExists(AddBackslash(WizardDirValue) + 'bin\64bit\obs64.exe') then
    begin
      MsgBox(CustomMessage('ObsNotFound'), mbError, MB_OK);
      Result := False;
    end;
  end;
end;

procedure InitializeWizard();
begin
  WizardForm.SelectDirLabel.Caption := CustomMessage('SelectDirHint');
end;

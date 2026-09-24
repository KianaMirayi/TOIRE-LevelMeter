; ============================================================================
;  TOIRE Level Meter - Windows VST3 installer
;
;  Build:
;    1. powershell -ExecutionPolicy Bypass -File installer\fetch-deps.ps1
;    2. cmake --build build --config Release
;    3. "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" installer\TOIRE-LevelMeter.iss
;    Output lands in dist\.
;
;  Five things drive the shape of this script:
;
;  1. A VST3 plugin is a BUNDLE (a folder), not a single file. All four entries
;     inside it are required - drop WebView2Loader.dll or Contents\webui and the
;     plugin either fails to load or comes up blank.
;
;  2. ArchitecturesInstallIn64BitMode is what makes {commoncf} resolve to the
;     64-bit Common Files folder. Without it a 64-bit install lands among the
;     x86 files and most DAWs never find the plugin.
;
;  3. WebView2 is a hard runtime dependency - the UI is a web page. Windows 11
;     ships it, older Windows 10 machines may not have it. The Evergreen
;     bootstrapper is fetched by fetch-deps.ps1 and only run when it is absent.
;
;  4. {app} is the bundle folder itself and [Files] installs into {app}, NOT into
;     a hardcoded {commoncf} path. The uninstaller then lives inside the bundle
;     instead of dropping unins000.exe into the shared VST3 root, where it would
;     collide with every other Inno-based plugin installer.
;
;  5. Paths are relative to this file, so the repo can be cloned anywhere.
; ============================================================================

#define AppName      "TOIRE Level Meter"
#define AppVersion   "1.0.1"
#define AppPublisher "TOIRE"

[Setup]
AppId={{B4E1C7A0-2D83-4F16-9C5E-7A0D3B8E6F25}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
VersionInfoVersion={#AppVersion}
VersionInfoDescription={#AppName} VST3 installer

; {app} is the bundle folder itself; {commoncf} is C:\Program Files\Common Files in 64-bit mode.
DefaultDirName={commoncf}\VST3\TOIRE Level Meter.vst3
DisableDirPage=yes
DisableProgramGroupPage=yes

; The shared VST3 folder is machine-wide, so admin rights are required. The
; per-user override stays on the command line for test installs only.
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=commandline

ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.17763

Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern

; Ask the user to close anything holding the plugin, but never relaunch a DAW.
CloseApplications=yes
RestartApplications=no

OutputDir=..\dist
OutputBaseFilename=TOIRE-LevelMeter-{#AppVersion}-win64
UninstallDisplayName={#AppName} {#AppVersion} (VST3)
UninstallDisplayIcon={app}\Contents\x86_64-win\TOIRE Level Meter.vst3

InfoBeforeFile=BEFORE-INSTALL.txt

[Languages]
#if FileExists(AddBackslash(SourcePath) + "deps\ChineseSimplified.isl")
Name: "chinesesimplified"; MessagesFile: "deps\ChineseSimplified.isl"
#else
Name: "english"; MessagesFile: "compiler:Default.isl"
#endif

[Files]
; {app} is the bundle folder, so the bundle's own Contents\ layout is preserved.
Source: "..\build\TOIRELevelMeter_artefacts\Release\VST3\TOIRE Level Meter.vst3\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

; dontcopy: only unpacked to {tmp} if PrepareToInstall decides the runtime is missing.
Source: "deps\MicrosoftEdgeWebview2Setup.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall dontcopy

[Run]
Filename: "{tmp}\MicrosoftEdgeWebview2Setup.exe"; Parameters: "/silent /install"; StatusMsg: "正在安装 Microsoft WebView2 运行时（插件界面依赖，需要联网，约几十秒）..."; Flags: waituntilterminated; Check: WebView2Missing

[Code]
function KeyHasWebView2(const RootKey: Integer; const SubKey: String): Boolean;
var
  Version: String;
begin
  Result := RegQueryStringValue(RootKey, SubKey, 'pv', Version) and (Version <> '');
end;

function WebView2Missing: Boolean;
begin
  { Microsoft documents the per-machine runtime under WOW6432Node and the per-user
    one under plain SOFTWARE in HKCU.

    The registry view is pinned explicitly with HKLM32/HKLM64 instead of relying on
    the installer's current mode: a 32-bit query of 'SOFTWARE\WOW6432Node\...' is
    redirected a second time and silently finds nothing, which would report the
    runtime as present when it is not - and that user gets a blank UI.

    Both paths are tried in each view so the answer does not depend on how a
    particular Windows build laid the keys out. }
  Result := not (KeyHasWebView2(HKLM64, 'SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}')
              or KeyHasWebView2(HKLM64, 'SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}')
              or KeyHasWebView2(HKLM32, 'SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}')
              or KeyHasWebView2(HKLM32, 'SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}')
              or KeyHasWebView2(HKCU,   'SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}')
              or KeyHasWebView2(HKCU,   'SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}'));
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  { dontcopy files are not unpacked unless asked for, so pull it out before [Run] needs it. }
  if WebView2Missing then
    ExtractTemporaryFile('MicrosoftEdgeWebview2Setup.exe');

  Result := '';
end;

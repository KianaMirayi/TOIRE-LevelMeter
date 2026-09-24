# ============================================================================
#  Fetches the third-party files the installer needs but that are NOT committed.
#
#  Run once after cloning, before building the installer:
#      powershell -ExecutionPolicy Bypass -File installer\fetch-deps.ps1
#
#  Two files land in installer\deps\:
#    MicrosoftEdgeWebview2Setup.exe  - Evergreen Bootstrapper, bundled into the
#                                      installer and run when the runtime is
#                                      missing on the target machine.
#    ChineseSimplified.isl           - Inno Setup messages, for a Chinese UI.
# ============================================================================

$ErrorActionPreference = 'Stop'

$deps = Join-Path $PSScriptRoot 'deps'
New-Item -ItemType Directory -Force -Path $deps | Out-Null

# curl.exe first: PowerShell 5.1's Invoke-WebRequest has trouble with some of
# these redirects. Invoke-WebRequest is the fallback.
function Fetch($url, $out) {
    & curl.exe -L --fail --silent --show-error --max-time 180 -o $out $url 2>$null
    if ($LASTEXITCODE -eq 0 -and (Test-Path $out) -and (Get-Item $out).Length -gt 0) { return $true }

    try {
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        Invoke-WebRequest -Uri $url -OutFile $out -UseBasicParsing -TimeoutSec 180
        return ((Test-Path $out) -and (Get-Item $out).Length -gt 0)
    } catch {
        return $false
    }
}

# --- 1) Microsoft WebView2 Evergreen Bootstrapper ---------------------------
$boot = Join-Path $deps 'MicrosoftEdgeWebview2Setup.exe'

if ((Test-Path $boot) -and (Get-Item $boot).Length -gt 500000) {
    Write-Host 'WebView2 bootstrapper: already present.'
}
else {
    Write-Host 'WebView2 bootstrapper: downloading...'
    if (-not (Fetch 'https://go.microsoft.com/fwlink/p/?LinkId=2124703' $boot)) {
        throw 'Could not download the WebView2 bootstrapper.'
    }

    # This file gets redistributed to end users, so refuse anything not signed by Microsoft.
    $sig = Get-AuthenticodeSignature $boot
    if ($sig.Status -ne 'Valid' -or $sig.SignerCertificate.Subject -notlike '*Microsoft Corporation*') {
        Remove-Item $boot -Force
        throw "Downloaded bootstrapper is not validly signed by Microsoft (status: $($sig.Status))."
    }
    Write-Host '  OK - signature valid, signed by Microsoft Corporation.'
}

# --- 2) Inno Setup Simplified Chinese messages ------------------------------
# Inno only reads non-ASCII .isl files that start with a UTF-8 BOM.
$isl = Join-Path $deps 'ChineseSimplified.isl'

if ((Test-Path $isl) -and (Get-Item $isl).Length -gt 5000) {
    Write-Host 'Chinese language file: already present.'
}
else {
    Write-Host 'Chinese language file: downloading...'
    $sources = @(
        'https://raw.githubusercontent.com/jrsoftware/issrc/refs/heads/main/Files/Languages/ChineseSimplified.isl',
        'https://cdn.jsdelivr.net/gh/jrsoftware/issrc@main/Files/Languages/ChineseSimplified.isl'
    )

    $text = $null
    foreach ($url in $sources) {
        if (Fetch $url $isl) {
            $candidate = [System.IO.File]::ReadAllText($isl, (New-Object System.Text.UTF8Encoding $false))
            if ($candidate -match 'LanguageName=') { $text = $candidate; break }
            Write-Host "  $url did not return an .isl file, trying the next source."
        }
        else {
            Write-Host "  failed: $url"
        }
    }

    if ($null -eq $text) {
        if (Test-Path $isl) { Remove-Item $isl -Force }
        Write-Host '  WARNING: no source worked. The installer will use an English UI instead.'
    }
    else {
        [System.IO.File]::WriteAllText($isl, $text, (New-Object System.Text.UTF8Encoding $true))
        Write-Host '  OK - UTF-8 BOM added.'
    }
}

Write-Host ''
Write-Host 'Dependencies ready in installer\deps\.'

# Run on the owner Windows PC. Requires GitHub CLI and repository admin access.
param(
  [string]$SourceZip = (Join-Path $PSScriptRoot 'SKELLAY_PRIVATE_NATIVE_FOUNDATION_0.6.zip'),
  [string]$PasswordFile = (Join-Path $env:LOCALAPPDATA 'Skellay/build-password.txt'),
  [string]$RunId = ''
)
$ErrorActionPreference = 'Stop'
$repo = 'skellyruno/skellay'
if (!(Get-Command gh -ErrorAction SilentlyContinue)) { throw 'Install GitHub CLI first: winget install --id GitHub.cli' }
& gh auth status
if ($LASTEXITCODE -ne 0) { throw 'Run gh auth login first.' }
if (!(Test-Path -LiteralPath $SourceZip -PathType Leaf)) { throw 'Place the private v0.6 source ZIP next to this script.' }
if ((Get-FileHash -LiteralPath $SourceZip -Algorithm SHA256).Hash.ToLowerInvariant() -ne 'fc89c48100701a804b044065b4dddca5343cbcf7fa08c2d19fbdbab6907708a4') { throw 'Wrong source ZIP or checksum mismatch.' }
$payload = [Convert]::ToBase64String([IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $SourceZip).Path))
$chunk = 24000
if ($payload.Length -le 3*$chunk -or $payload.Length -gt 4*$chunk) { throw 'Unexpected package length.' }
if (!(Test-Path -LiteralPath $PasswordFile)) {
  New-Item -ItemType Directory -Force (Split-Path -Parent $PasswordFile) | Out-Null
  $random = [byte[]]::new(32)
  $rng = [Security.Cryptography.RandomNumberGenerator]::Create()
  try { $rng.GetBytes($random) } finally { $rng.Dispose() }
  [IO.File]::WriteAllText($PasswordFile, [Convert]::ToBase64String($random))
}
$password = ([IO.File]::ReadAllText($PasswordFile)).Trim()
if ([Convert]::FromBase64String($password).Length -ne 32) { throw 'Invalid existing password file; do not overwrite without preserving old archive access.' }
function Set-PrivateSecret([string]$Name,[string]$Value) {
  $Value | & gh secret set $Name --repo $repo --app actions
  if ($LASTEXITCODE -ne 0) { throw "Failed to provision $Name." }
}
for ($i=0; $i -lt 4; $i++) {
  $start = $i*$chunk
  Set-PrivateSecret "SKELLAY_NATIVE_$($i+1)" ($payload.Substring($start, [Math]::Min($chunk,$payload.Length-$start)))
}
Set-PrivateSecret 'SKELLAY_BUILD_PASSWORD' $password
Write-Host "Private build secrets configured. Archive password saved locally: $PasswordFile"
if ($RunId) {
  & gh run rerun $RunId --repo $repo
  if ($LASTEXITCODE -ne 0) { throw 'Secrets were saved, but rerun failed. Open the newest workflow run and rerun it manually.' }
}

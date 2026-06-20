# Cursor stop hook — 변경사항 있으면 자동 백업 커밋 (2분 디바운스)
$ErrorActionPreference = 'SilentlyContinue'

$HookDir = $PSScriptRoot
$RepoRoot = (Resolve-Path (Join-Path $HookDir '..\..')).Path
$DebounceFile = Join-Path $HookDir '.last-backup'
$MinIntervalSec = 120

Set-Location $RepoRoot

if (Test-Path $DebounceFile) {
    try {
        $last = [datetime]::Parse((Get-Content $DebounceFile -Raw).Trim())
        if (((Get-Date) - $last).TotalSeconds -lt $MinIntervalSec) { exit 0 }
    } catch { }
}

# stdin 소비 (stop hook JSON)
$null = [Console]::In.ReadToEnd()

$status = git status --porcelain 2>&1
if ([string]::IsNullOrWhiteSpace($status)) { exit 0 }

git add -A
git reset HEAD -- .claude/ 2>$null
git reset HEAD -- WiNILL/_recover_main.cpp WiNILL/_git_main_utf8.cpp 2>$null
git reset HEAD -- WiNILL/_extract_render.ps1 WiNILL/_fix_main.ps1 WiNILL/_patch_main.ps1 2>$null
git reset HEAD -- WiNILL/_restore_render.ps1 WiNILL/_strip_render.ps1 2>$null

$staged = git diff --cached --name-only 2>&1
if ([string]::IsNullOrWhiteSpace($staged)) { exit 0 }

$ts = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'
$msg = "backup: $ts (auto)"

git commit -m $msg
if ($LASTEXITCODE -eq 0) {
    Set-Content -Path $DebounceFile -Value (Get-Date -Format 'o') -NoNewline
}

exit 0

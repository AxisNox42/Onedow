# bin/Onedow → bin/Onedow.zip
# 포함 파일은 scripts/package_manifest.txt 가 정한다 (zip_onedow.sh / macOS CI 와 같은 기준).
# -Source / -Output 은 검증용 임시 fixture 를 위한 선택 인자다.
param(
    [string]$Source = "",
    [string]$Output = ""
)
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$Onedow = if ($Source) { [IO.Path]::GetFullPath($Source) } else { Join-Path $Root "bin\Onedow" }
$Zip = if ($Output) { [IO.Path]::GetFullPath($Output) } else { Join-Path $Root "bin\Onedow.zip" }
$Manifest = Join-Path $PSScriptRoot "package_manifest.txt"
# Windows 기본 bsdtar: mtree 로 Unix 권한(macOS/Onedow 0755)을 ZIP 에 기록한다.
# Compress-Archive 는 권한을 기록하지 못하고, PATH 의 GNU tar 는 ZIP 을 만들지 못한다.
$Tar = Join-Path $env:SystemRoot "System32\tar.exe"

if (-not (Test-Path $Onedow)) { throw "bin/Onedow 없음 — package_lts.ps1 먼저" }
if (-not (Test-Path (Join-Path $Onedow "WindowsOS\Onedow.exe"))) {
    throw "WindowsOS\Onedow.exe 없음 — package_lts.ps1 먼저"
}
if (-not (Test-Path $Tar)) { throw "System32\tar.exe 없음 (Windows 10 1803 이상 필요)" }

$Items = foreach ($Line in Get-Content -LiteralPath $Manifest -Encoding UTF8) {
    $Parts = $Line.Trim() -split '\s+'
    if (-not $Parts[0] -or $Parts[0].StartsWith('#')) { continue }
    [pscustomobject]@{ Kind = $Parts[0]; Path = $Parts[1]; Exec = $Parts -contains 'exec' }
}

# Stage only the manifest items; local saves and stale build outputs may live
# beside the executable after testing the package.
$PackageRoot = [IO.Path]::GetFullPath((Join-Path $Root "build\package"))
$StageRoot = Join-Path $PackageRoot ([guid]::NewGuid().ToString("N"))
$Stage = Join-Path $StageRoot "Onedow"
try {
    New-Item -ItemType Directory -Path (Join-Path $Stage "WindowsOS"), (Join-Path $Stage "macOS") -Force | Out-Null
    $ExecPaths = @()
    foreach ($Item in $Items) {
        $Rel = $Item.Path.TrimEnd('/')
        $From = Join-Path $Onedow ($Rel -replace '/', '\')
        if (-not (Test-Path -LiteralPath $From)) {
            if ($Item.Kind -eq 'required') { throw "$($Item.Path) 없음 — package_lts.ps1 먼저" }
            continue
        }
        $To = Join-Path $Stage ($Rel -replace '/', '\')
        New-Item -ItemType Directory -Path (Split-Path $To -Parent) -Force | Out-Null
        Copy-Item -LiteralPath $From -Destination $To -Recurse
        if ($Item.Exec) { $ExecPaths += "Onedow/$Rel" }
    }

    # mtree: every staged directory and file with an explicit mode.
    $Lines = @('#mtree')
    $Entries = @(Get-Item -LiteralPath $Stage) + @(Get-ChildItem -LiteralPath $Stage -Recurse)
    foreach ($Entry in $Entries) {
        $Rel = $Entry.FullName.Substring($StageRoot.Length + 1) -replace '\\', '/'
        if ($Rel -notmatch '^[A-Za-z0-9._/-]+$') { throw "ZIP 경로에 쓸 수 없는 문자: $Rel" }
        if ($Entry.PSIsContainer) { $Lines += "$Rel type=dir mode=0755" }
        elseif ($ExecPaths -contains $Rel) { $Lines += "$Rel type=file mode=0755" }
        else { $Lines += "$Rel type=file mode=0644" }
    }
    $Spec = Join-Path $StageRoot "package.mtree"
    [IO.File]::WriteAllText($Spec, ($Lines -join "`n") + "`n", (New-Object Text.UTF8Encoding $false))

    if (Test-Path -LiteralPath $Zip) { Remove-Item -LiteralPath $Zip -Force }
    Push-Location $StageRoot
    try {
        & $Tar -a -c -f $Zip "@package.mtree"
        if ($LASTEXITCODE -ne 0) { throw "ZIP 생성 실패 (tar exit $LASTEXITCODE)" }
    } finally { Pop-Location }
} finally {
    $ResolvedStageRoot = [IO.Path]::GetFullPath($StageRoot)
    if (-not $ResolvedStageRoot.StartsWith($PackageRoot + [IO.Path]::DirectorySeparatorChar,
                                           [StringComparison]::OrdinalIgnoreCase)) {
        throw "패키징 임시 경로가 build/package 밖에 있습니다."
    }
    if (Test-Path -LiteralPath $ResolvedStageRoot) {
        Remove-Item -LiteralPath $ResolvedStageRoot -Recurse -Force
    }
}

Write-Host ""
Write-Host "LTS ZIP: $Zip"

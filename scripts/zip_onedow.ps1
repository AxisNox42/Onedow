# bin/Onedow → bin/Onedow.zip
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$Onedow = Join-Path $Root "bin\Onedow"
$Zip = Join-Path $Root "bin\Onedow.zip"

if (-not (Test-Path $Onedow)) { throw "bin/Onedow 없음 — package_lts.ps1 먼저" }
if (-not (Test-Path (Join-Path $Onedow "WindowsOS\Onedow.exe"))) {
    throw "WindowsOS\Onedow.exe 없음 — package_lts.ps1 먼저"
}
if (-not (Test-Path (Join-Path $Onedow "README.md"))) {
    throw "README.md 없음 — package_lts.ps1 먼저"
}

# Stage only distribution files; local saves and stale build outputs may live
# beside the executable after testing the package.
$PackageRoot = [IO.Path]::GetFullPath((Join-Path $Root "build\package"))
$StageRoot = Join-Path $PackageRoot ([guid]::NewGuid().ToString("N"))
$Stage = Join-Path $StageRoot "Onedow"
try {
    New-Item -ItemType Directory -Path (Join-Path $Stage "WindowsOS"), (Join-Path $Stage "macOS") -Force | Out-Null
    Copy-Item (Join-Path $Onedow "README.md") $Stage
    Copy-Item (Join-Path $Onedow "Resource") $Stage -Recurse
    Copy-Item (Join-Path $Onedow "WindowsOS\Onedow.exe") (Join-Path $Stage "WindowsOS")
    foreach ($MacFile in @("Onedow", "README.txt")) {
        $MacSource = Join-Path $Onedow "macOS\$MacFile"
        if (Test-Path -LiteralPath $MacSource) {
            Copy-Item -LiteralPath $MacSource -Destination (Join-Path $Stage "macOS")
        }
    }
    Compress-Archive -Path $Stage -DestinationPath $Zip -CompressionLevel Optimal -Force
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

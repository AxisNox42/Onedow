$ErrorActionPreference = "Stop"
$root = "D:\01_Study\02_OpenGL\WiNILL - 복사본"
$rel  = Join-Path $root "x64\Release"
$out  = Join-Path $root "_SUBMIT"
$game = Join-Path $out  "Onedow"

# 깨끗하게 재생성
if (Test-Path $out) { Remove-Item $out -Recurse -Force }
New-Item -ItemType Directory -Force -Path $game | Out-Null

# 1) 실행파일
Copy-Item (Join-Path $rel "WiNILL.exe") (Join-Path $game "Onedow.exe")

# 2) 런타임 리소스 (사운드) — 폰트/아이콘은 exe 에 임베드됨
Copy-Item (Join-Path $rel "Resource") (Join-Path $game "Resource") -Recurse

# 3) README
$readme = @'
Onedow — 데스크톱 디펜스 (Desktop Defense)

[실행]
  Onedow.exe 더블클릭. (Resource 폴더는 같이 있어야 사운드가 납니다.)

[조작]
  WASD      이동
  마우스    조준 / 발사
  SHIFT     대시
  Q/E/R     액티브 스킬
  ESC       일시정지

[흐름]
  PLAY -> 난이도 -> 직업 -> 시작무기 -> 게임. 레벨업/보스 처치 시 증강 선택.

[설정] 메인 메뉴 CONFIG: FPS / 언어 / 사운드 볼륨 등.

요구: Windows 10/11 64-bit. 별도 설치/DLL 불필요.
'@
$readme | Out-File -FilePath (Join-Path $game "README.txt") -Encoding utf8

# 4) zip
$stamp = Get-Date -Format "yyyyMMdd_HHmm"
$zip = Join-Path $out ("Onedow_" + $stamp + ".zip")
Compress-Archive -Path $game -DestinationPath $zip -Force

Write-Output "패키지 폴더: $game"
Get-ChildItem $game -Recurse | Select-Object FullName, Length | Format-Table -Auto
Write-Output ("ZIP: " + $zip + "  (" + [math]::Round((Get-Item $zip).Length/1MB,1) + " MB)")

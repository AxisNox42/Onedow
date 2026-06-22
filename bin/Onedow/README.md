# Onedow 배포 폴더

플랫폼별 실행 파일은 여기에 패키징됩니다.

```
bin/Onedow/
├── WindowsOS/     ← Windows (Onedow.exe)
└── macOS/         ← macOS (Onedow)
```

## 패키징

**Windows** (이 PC):

```powershell
.\scripts\package_windows.ps1
```

**macOS** (맥에서):

```bash
chmod +x scripts/package_macos.sh
./scripts/package_macos.sh
```

각 폴더 안의 `Onedow` 실행 파일과 `Resource/`, `Font/` 를 **같은 폴더에 둔 채** 실행하세요.

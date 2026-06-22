# macOS 빌드 (LTS)

```bash
./scripts/package_macos.sh    # bin/Onedow/macOS/Onedow
./scripts/zip_onedow.sh       # bin/Onedow.zip
```

## 폴더 구분

| 용도 | 경로 |
|------|------|
| **테스트 (Windows)** | `WiNILL/bin/WiNILL.exe` — `.\scripts\build_windows.ps1` |
| **LTS 배포** | `bin/Onedow/` — `.\scripts\release_windows.ps1` |

```
bin/Onedow/
├── README.txt
├── Resource/  Font/  Icons/
├── WindowsOS/Onedow.exe
└── macOS/Onedow
```

ZIP: `bin/Onedow.zip`

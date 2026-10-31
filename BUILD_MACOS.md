# macOS 빌드 (LTS)

```bash
./scripts/package_macos.sh    # bin/Asteria/macOS/Asteria
./scripts/zip_asteria.sh       # bin/Asteria.zip
```

## 폴더 구분

| 용도 | 경로 |
|------|------|
| **테스트 (Windows)** | `build/windows/x64/Release/WiNILL.exe` — `.\scripts\build_windows.ps1` |
| **LTS 배포** | `bin/Asteria/` — `.\scripts\release_windows.ps1` |

```
bin/Asteria/
├── README.txt
├── Resource/
│   ├── Audio/  Font/  Icons/  Image/
│   └── bg_*.png
├── WindowsOS/Asteria.exe
└── macOS/Asteria
```

ZIP: `bin/Asteria.zip`

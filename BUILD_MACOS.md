# macOS 빌드

```bash
./scripts/package_macos.sh    # bin/Zip/Onedow/macOS/Onedow
./scripts/zip_onedow.sh       # bin/Zip/Onedow.zip
```

Windows exe는 맥 CI/수동으로 `bin/Zip/Onedow/WindowsOS/Onedow.exe` 에 넣은 뒤 같은 ZIP 구조 유지.

배포 폴더:

```
bin/Zip/Onedow/
├── README.txt
├── WindowsOS/Onedow.exe
└── macOS/Onedow
```

ZIP: `bin/Zip/Onedow.zip`

WiNILL Resources
================

Font/   — TTF/OTF 폰트 (현재: Dongle-Regular.ttf)
Image/  — 증강 픽토그램·UI 아이콘 (PNG)
Audio/  — 효과음·배경음 (WAV/OGG/MP3)

코드에서는 항상 상대경로 "Resource/<카테고리>/<파일명>" 으로 접근.
프로젝트 빌드 시 working directory 가 .vcxproj 와 동일한 폴더이므로
실행 파일 위치가 아니라 프로젝트 폴더 기준으로 로드됨.

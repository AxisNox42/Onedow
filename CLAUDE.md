# [Context]
- Theme: Win32 DWM 투명 오버레이 기반 가짜 OS 창 전투 로그라이트
- Stack: C++17, OpenGL 3.3, GLFW, glm, Win32
- Entry: `WiNILL/main.cpp` (8k+ Monolithic)

# [Build & Project Rules]
- Build: MSBuild x64 (LNK4098 경고 무시)
- 새 `.cpp` 생성 시 반드시 `WiNILL.vcxproj`에 수동 등록할 것.
- 로컬 `Include/`, `Lib/` 폴더 절대 수정/삭제 금지.

# [Rendering Pipeline - CRITICAL]
1. Immediate Draw 절대 금지: 모든 렌더링(`drawX`)은 반드시 CPU 배치(`g_Batch`)에 적재.
2. 상태 변경 전 Flush 강제: Scissor, Blend, Shader(text/icon 등) 변경 직전에는 무조건 `BatchFlush()` 호출.
3. Fake Window 클리핑: 렌더링은 가짜 창 단위로 `WorldScissor(wx,wy,ww,wh)`를 통해 클리핑됨. 
4. 시야 외 렌더링 금지: 가짜 창 밖(맨 바탕화면)에는 월드를 렌더링하지 않음. 가짜 창을 소유하지 않은 객체는 그리지 말 것.
5. 멀티 윈도우 렌더링: 여러 창에 걸쳐 보이는 객체는 해당 창마다 렌더 패스를 반복할 것. 가짜 창 생성은 반드시 전용 함수를 통함.

# [Directory Zoning]
- `WiNILL/main.cpp` : 전역 상태, 게임 루프 (최소한만 건드릴 것)
- `WiNILL/Render/` : OpenGL 래퍼, BatchFlush, 셰이더 클래스
- `WiNILL/Entity/` : 플레이어, 보스, 총알 등 게임 객체 로직
- `WiNILL/System/` : 윈도우 창 제어, Win32 DWM 오버레이 로직
- `Include/`, `Lib/` : 외부 라이브러리 (절대 건드리지 말 것)
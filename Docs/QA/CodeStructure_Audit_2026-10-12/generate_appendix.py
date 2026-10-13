"""Render complete file coverage from the lexical audit and manual role map.
Only writes File_Coverage.md and external_boundaries.tsv beside this script.
"""
from pathlib import Path
from collections import defaultdict, Counter
import csv
import hashlib
import json
import re

OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[2]

def read(name):
    with (OUT / name).open(encoding='utf-8-sig', newline='') as f:
        return list(csv.DictReader(f, delimiter='\t'))

def link(path, line=None, label=None):
    target = (ROOT / path).as_posix() + (f':{line}' if line else '')
    return f'[{label or path}](<{target}>)'

# A role is not an assertion that every branch has been behaviorally tested.
roles = {
 'main.cpp': '앱 수명·게임 루프·전환·런 시작/종료·전투·보상·HUD의 조정 중심',
 'Achievements.h': '업적 정의·해금 상태·진행 갱신',
 'AugmentSlots.h': '증강 슬롯/보유 목록·교체 상태',
 'Codex.h': '도감 대상/진행/검색/필터 및 해금 상태',
 'GameContext.h': '게임 전역 상태를 다른 영역에 노출하는 extern 접점',
 'GameManager.cpp': '게임 상태 갱신·점수/XP·증강 추첨·내부 셰이더 초기화',
 'GameManager.h': 'GameState와 GameManager 데이터/인터페이스',
 'GameVersion.h': '제품 버전 정의',
 'Meta.h': '런 밖 메타 진행·구매 효과·플레이어 스탯 적용',
 'TutorialText.h': '언어별 튜토리얼 텍스트/선택',
 'SceneAugment.cpp': '증강 선택·미리보기·카드·소유 패널 UI와 표시 계산',
 'SceneBoot.cpp': '부팅/초기 화면 연출',
 'SceneCodex.inl': '독립 CODEX 화면의 도감 UI 구현',
 'SceneCodexInline.inl': '메인 메뉴 안 도감 패널 UI 구현',
 'SceneContext.h': '화면에 전달하는 게임/렌더/상태 컨텍스트',
 'SceneInternal.h': '화면 내부 공용 선언',
 'SceneMainMenu.inl': '메인 메뉴 내용·선택·패널 진입',
 'SceneMenus.cpp': '메뉴 공통 상태·연출·back 처리 및 여러 inl의 번역 단위',
 'ScenePause.cpp': '일시정지 화면 표시/입력',
 'ScenePause.h': '일시정지용 좁은 컨텍스트/인터페이스',
 'SceneRunConfig.cpp': '직업/무기/시련/Creative 런 구성·능력표·로드아웃 확정',
 'SceneRunEnd.cpp': '사망/게임오버 결과 UI',
 'SceneRunEnd.h': '런 종료 화면용 컨텍스트/인터페이스',
 'Scenes.h': '화면 진입점 묶음',
 'SceneSettings.cpp': '설정 레이아웃·편집·스냅샷·취소/적용·변경 감지',
 'SceneShop.inl': '상점 패널·선택/스크롤·구매 UI',
 'SceneSkills.cpp': '스킬 슬롯 상태·장착/재장착·HUD',
 'SceneSkills.h': '스킬 타입/슬롯과 장착·표시 함수 선언',
 'SceneStart.cpp': '튜토리얼/Ready/시작 화면 및 텍스트 처리',
 'SceneStart.h': '시작 화면용 컨텍스트/인터페이스',
 'SceneTextContext.h': '텍스트 렌더러 중심의 좁은 화면 컨텍스트',
 'SceneUI.cpp': '화면 공통 UI 도형/텍스트/전환 기능',
 'SceneUI.h': '공통 UI 인터페이스와 전환 관련 선언',
 'WindowChrome.cpp': '앱 창 테두리/제목/버튼 그리기와 동작',
 'WindowChrome.h': '창 UI 인터페이스',
 'Augment.h': '증강 타입/희귀도/분류/자격/설명 선택의 공용 모델',
 'AugmentCatalog.cpp': '증강 카탈로그 슬롯/이름/설명/예약 데이터',
 'AugmentDescKR.h': '한국어 증강 상세 설명 오버라이드',
 'Bullet.h': '탄환 상태·이동/유도·수명·피해/반사 속성',
 'CollisionSystem.h': '탄환-몹/플레이어 충돌·피해·처치·관통/도탄',
 'EnemyProgression.h': '진행도에 따른 적 스펙 배율/상한',
 'EnemyShield.h': '적 실드 상태/피해 처리와 관련 효과',
 'EnemySpawner.cpp': '일반/원거리 스폰 시간·종류 선택·진행도/개체 제한',
 'EnemySpawner.h': '스폰 초기화/갱신 인터페이스',
 'ExpSystem.h': 'XP 획득/레벨 관련 계산·표현 접점',
 'Monster.h': '일반/특수 몹 데이터·스펙·AI 상태·사망 형상/연출',
 'MonsterManager.h': '일반/원거리 몹 소유·갱신·검색·특수 패턴 조정',
 'PlayerRuntime.cpp': '플레이어 이동·대시·스킬 타이머·입력 edge·실행 상태',
 'PlayerRuntime.h': '플레이어 런타임 상태와 이동/타이머 인터페이스',
 'PlayerStats.h': '기본 스탯·증강 적용·누적 보너스·흡혈/스탯 계산',
 'ProjectileSystem.cpp': '플레이어 투사체 발사·갱신·지우기 등 처리',
 'ProjectileSystem.h': '투사체 시스템 호출 인터페이스/입력',
 'RangedMob.h': '원거리 몹 상태·차징/발사·이동·피격/사망 상태',
 'Weapons.h': '시작 무기 타입/기본 적용 규칙',
 'BlurShader.cpp': '블러 셰이더·캡처 framebuffer·렌더 상태 저장/복원',
 'BlurShader.h': '블러 초기화/캡처/그리기 인터페이스',
 'Camera.cpp': '카메라 좌표/흔들림 갱신·변환',
 'Camera.h': '카메라 상태/인터페이스',
 'DrawPrim.h': '공통 도형/배치/렌더 pass 기본 연산',
 'EnemyParticles.h': '적 파티클/폭발 등 상태와 생성·갱신·표시',
 'EntityDraw.cpp': '몹·탄환·필드·특수 패턴 렌더와 가시성 계산',
 'EntityDraw.h': '엔티티 렌더 진입점',
 'GameFonts.cpp': '플랫폼/내장 폰트 로드·텍스트 렌더러 설정',
 'GameFonts.h': '게임 폰트 로드/설정 인터페이스',
 'IconSystem.h': '아이콘 로딩/마스크·텍스처·셰이더·일반/회전/darken 그리기',
 'Juice.h': '피해 숫자·잔상·히트스톱·콤보·파티클; 전투 누적 부수효과 포함',
 'MainShader.cpp': '메인 셰이더·gradient 텍스처·기하 제출',
 'MainShader.h': '메인 셰이더·색/기하 렌더 인터페이스',
 'NebulaGlowShader.cpp': '성운 glow 셰이더/quad 초기화·그리기',
 'NebulaGlowShader.h': '성운 glow 인터페이스',
 'PlanetShader.cpp': '행성 셰이더/quad 초기화·그리기',
 'PlanetShader.h': '행성 셰이더 인터페이스',
 'PlayerWeaponShell.cpp': '플레이어 무기 외형·선분 기하·표시 상태 참조',
 'PlayerWeaponShell.h': '플레이어 무기 표시 인터페이스',
 'stb_impl.cpp': '외부 stb 라이브러리 구현 매크로의 단일 번역 단위',
 'TextRenderer.h': '폰트/glyph atlas·측정·줄바꿈 지원·일반/회전 텍스트·GL 수명',
 'UiColors.h': 'UI 색/알파 정책',
 'UiLayout.h': 'UI 레이아웃/보간 계산 도구',
 'resource.h': 'Windows 내장 리소스 ID',
 'Audio.cpp': '오디오 엔진/장치·BGM/SFX·설정 적용 상태',
 'Audio.h': '오디오 재생/설정/수명 인터페이스',
 'CrashHandler.h': '예외/크래시 처리와 진단 경계',
 'DebugToolkit.cpp': '개발 패널·필드 편집·화면 캡처·테스트용 상태 조정',
 'DebugToolkit.h': '디버그 도구 상태/호출 인터페이스',
 'EmbeddedResource.h': 'Windows 내장 리소스의 공통 읽기 helper',
 'FakeWindow.cpp': '플레이어 가상 창 이동/충돌·위치/크기 조정',
 'FakeWindow.h': '가상 창 bounds/인터페이스',
 'GameplayTelemetry.h': '런/증강 스탯 snapshot·차이·이벤트 기록',
 'Input.cpp': '키/마우스/텍스트 callback·Windows IME·화면별 입력 편집',
 'Input.h': '입력 상태/callback·IME 관련 인터페이스',
 'Platform.h': '플랫폼별 창/타이머/OS 기능 분기',
 'SaveSystem.h': '설정·진행 저장/로드·초기화·런 결과 반영',
 'Settings.h': '옵션 기본값·언어·시련 계산·여러 공용 게임 설정 상태',
 'SmokeCapture.h': '자동 UI 스모크 캡처 설정/경로',
 'SystemInfo.h': '시스템/환경 정보 수집',
 'Translations.h': '언어별 공용 번역 데이터/선택',
 'WindowFx.cpp': '창/화면 효과 갱신과 적용',
 'WindowFx.h': '창 효과 상태/인터페이스',
 'blur_render_test.cpp': 'GL 환경의 블러 캡처/framebuffer 상태 회귀 테스트',
 'genesis_launcher_test.cpp': 'Genesis 투척·앵커/탄환 패턴 테스트',
 'magnetar_field_test.cpp': 'Magnetar 장/탄환 상호작용 테스트',
 'mob_death_fx_test.cpp': '몹 종류별 사망 효과/상태 테스트',
 'quasar_beam_test.cpp': 'Quasar 빔 동작 테스트',
 'quasar_nova_test.cpp': 'Quasar nova 패턴 테스트',
 'regulus_beam_test.cpp': 'Regulus 빔 패턴 테스트',
}

group_paths = {
 '01': ['main.cpp','CollisionSystem.h','Juice.h','PlayerStats.h','Codex.h','GameManager.cpp'],
 '02': ['main.cpp','PlayerStats.h','Meta.h','Weapons.h','SceneAugment.cpp','SceneRunConfig.cpp','Settings.h'],
 '03': ['main.cpp','PlayerStats.h'], '04': ['main.cpp','PlayerRuntime.cpp','MonsterManager.h'],
 '05': ['main.cpp','GameManager.cpp','SaveSystem.h','Achievements.h'],
 '06': ['PlayerRuntime.cpp','PlayerRuntime.h','SceneSkills.cpp','SceneSkills.h'],
 '07': ['GameManager.cpp','Augment.h','AugmentSlots.h'],
 '08': ['Settings.h','SceneRunConfig.cpp'],
 '09': ['Monster.h','RangedMob.h','EnemyProgression.h','EnemySpawner.cpp','Codex.h','SceneCodex.inl','SceneCodexInline.inl'],
 '10': ['Weapons.h','SceneRunConfig.cpp','main.cpp'],
 '11': ['Augment.h','AugmentCatalog.cpp','AugmentDescKR.h','PlayerStats.h','IconSystem.h'],
 '12': ['CollisionSystem.h','MonsterManager.h','Codex.h'],
 '13': ['main.cpp','CollisionSystem.h'], '14': ['CollisionSystem.h'],
 '15': ['Monster.h','main.cpp','MonsterManager.h'], '16': ['Monster.h','RangedMob.h'],
 '17': ['MonsterManager.h','CollisionSystem.h','main.cpp'], '18': ['EnemySpawner.cpp','MonsterManager.h'],
 '19': ['Settings.h','Translations.h','TutorialText.h','SceneMenus.cpp','SceneSettings.cpp','SceneShop.inl','SceneStart.cpp','SceneRunConfig.cpp','SceneCodex.inl','SceneCodexInline.inl','WindowFx.cpp'],
 '20': ['SceneUI.cpp','SceneUI.h','UiLayout.h','SceneSettings.cpp','SceneShop.inl','SceneCodex.inl','SceneCodexInline.inl','SceneRunConfig.cpp','SceneAugment.cpp'],
 '21': ['SceneCodexInline.inl','SceneShop.inl','SceneSettings.cpp','SceneRunConfig.cpp'],
 '22': ['SceneMenus.cpp','SceneCodex.inl','SceneCodexInline.inl','DebugToolkit.cpp'],
 '23': ['SceneCodex.inl','SceneCodexInline.inl','SceneAugment.cpp','SceneRunConfig.cpp'],
 '24': ['SceneStart.cpp'], '25': ['SceneCodexInline.inl','SceneShop.inl','SceneSettings.cpp','SceneRunConfig.cpp','SceneUI.h'],
 '26': ['Settings.h','SceneSettings.cpp','SaveSystem.h'],
 '27': ['Audio.cpp','Settings.h','SceneSettings.cpp','main.cpp'],
 '28': ['SaveSystem.h','Settings.h','Augment.h'],
 '29': ['GameplayTelemetry.h','PlayerStats.h','DebugToolkit.cpp','SceneAugment.cpp'],
 '30': ['main.cpp','SceneSkills.cpp'], '31': ['Monster.h','EntityDraw.cpp'],
 '32': ['EntityDraw.cpp','SceneMenus.cpp','SceneAugment.cpp','PlayerWeaponShell.cpp','DrawPrim.h'],
 '33': ['UiLayout.h','UiColors.h','SceneUI.h','SceneAugment.cpp','SceneMenus.cpp','DebugToolkit.cpp'],
 '34': ['IconSystem.h'], '35': ['TextRenderer.h','IconSystem.h'],
 '36': ['MainShader.cpp','MainShader.h','IconSystem.h','TextRenderer.h','GameManager.cpp','NebulaGlowShader.cpp','PlanetShader.cpp','BlurShader.cpp'],
 '37': ['MainShader.cpp','EmbeddedResource.h','IconSystem.h','Audio.cpp','GameFonts.cpp'],
 '38': ['MainShader.cpp','MainShader.h','IconSystem.h','TextRenderer.h','BlurShader.cpp','NebulaGlowShader.cpp','PlanetShader.cpp','DrawPrim.h'],
 '39': ['blur_render_test.cpp','genesis_launcher_test.cpp','magnetar_field_test.cpp','mob_death_fx_test.cpp','quasar_beam_test.cpp','quasar_nova_test.cpp','regulus_beam_test.cpp'],
 '40': ['GameVersion.h','resource.h'],
}

files = read('files.tsv')
incs = read('includes.tsv')
funcs = read('functions.tsv')
calls = read('call_sites.tsv')
refs = read('global_references.tsv')
support = read('support_files.tsv')
by_inc = defaultdict(list)
by_func = defaultdict(list)
by_ref = defaultdict(list)
dependents = defaultdict(set)
for x in incs:
    by_inc[x['source']].append(x)
    if x['kind']=='owned':
        dependents[x['resolved_target']].add(x['source'])
for x in funcs:
    by_func[x['file']].append(x)
for x in refs:
    by_ref[x['file']].append(x)
call_count = Counter(x['file'] for x in calls)
assert len(files) == len({x['file'] for x in files}) == 105
assert all(Path(x['file']).name in roles for x in files), 'Missing manual role'
summary = json.loads((OUT/'summary.json').read_text(encoding='utf-8'))
assert sum(len(v) for v in by_func.values()) == summary['lexical_functions']

# Focused direct write candidates. Alias/member-offset writes are reviewed
# separately in Global_Writes.md; this is deliberately not C++ dataflow.
FOCUS = ('g_Stats','g_Combo','g_RainKillAccum','g_ComboTimer','g_ComboPulse',
         'g_ComboMilestone','g_Skills','g_SkillReplaceIdx')
ACCESS = r'(?:\s*\[[^\]\n]*\]|\.[A-Za-z_]\w*)*'
WRITE = re.compile(r'(?P<prefix>\+\+|--)?\b(?P<symbol>'+'|'.join(FOCUS)+r')\b'
                   + ACCESS + r'\s*(?P<operator>\+\+|--|[+*/%&|^-]=|=(?!=))?')

def direct_writes(line):
    return [(m['symbol'], m['prefix'] or m['operator']) for m in WRITE.finditer(line)
            if m['prefix'] or m['operator']]

assert direct_writes('g_Stats = PlayerStats();') == [('g_Stats','=')]
assert direct_writes('++g_Stats.vampireKillStreak; g_Skills[i].cd -= dt;') == [('g_Stats','++'),('g_Skills','-=')]
assert not direct_writes('if (g_Stats.maxHP == 0) read(g_Stats);')
focus_rows=[]
for x in files:
    p=x['file']
    for n,line in enumerate((ROOT/p).read_text(encoding='utf-8',errors='replace').splitlines(),1):
        code=line.split('//',1)[0]
        enclosing=[f for f in by_func[p] if int(f['body_line'])<=n<=int(f['end_line'])]
        owner=min(enclosing,key=lambda f:int(f['end_line'])-int(f['body_line']))['name'] if enclosing else 'global_or_unresolved'
        for symbol,operator in direct_writes(code):
            focus_rows.append((symbol,p,n,owner,'direct_write_or_initializer',operator,line.strip()))
        for m in re.finditer(r'g_Stats\.(Apply|ApplyCommonMultBoost|RestoreWarlordStacks|RegisterKill)\s*\(',code):
            focus_rows.append(('g_Stats',p,n,owner,'known_mutating_member_call',m[1],line.strip()))
        for m in re.finditer(r'(ApplyMeta|ApplyWeapon)\s*\(\s*g_Stats\b',code):
            focus_rows.append(('g_Stats',p,n,owner,'known_mutable_reference_argument',m[1],line.strip()))
with (OUT/'focus_write_candidates.tsv').open('w',encoding='utf-8-sig',newline='') as f:
    w=csv.writer(f,delimiter='\t')
    w.writerow(['symbol','file','line','lexical_enclosing_function','kind','operator_or_callee','source'])
    w.writerows(focus_rows)

doc = ['# 전체 파일 구조 조사표', '',
       '기준: 2026-10-12 로컬 작업 트리. 게임 98개+테스트 7개 전체의 구조 자료를 수집했다. '
       '각 파일의 책임은 구조 분류이며 모든 실행 분기를 테스트했다는 의미가 아니다. '
       '검토 ID는 본문 중복 목록과 연결되며 의존 파일까지 포함한다. ID가 없는 파일도 구조 조사 범위에 포함된다.', '',
       '함수 목록은 어휘 분석으로 검출한 후보 전체이다. 람다·일부 생성자/연산자·매크로는 빠질 수 있다. '
       '호출 수는 이름 기반 구문 위치, g_* 목록은 읽기/쓰기 구분 없는 참조다. include의 조건부 분기는 합산한다. '
       '중복·의존 해석과 이관 순서는 ' + link((OUT/'README.md').relative_to(ROOT).as_posix(),label='조사 본문') + '을 참고한다.', '',
       '## 1. 전체 소스 책임/영향 표', '',
       '| 파일 | 줄 | 함수 후보 | 직접 의존/피의존 | 책임 | 검토 ID |',
       '|---|---:|---:|---:|---|---|']
for x in files:
    p = x['file']; name = Path(p).name
    ids = ', '.join('D'+k for k,v in group_paths.items() if name in v) or '구조/인터페이스 조사'
    doc.append(f"| {link(p)} | {x['lines']} | {x['lexical_function_definitions']} | "
               f"{x['direct_owned_dependencies']}/{x['direct_dependents']} | {roles[name]} | {ids} |")
doc += ['', '## 2. 파일별 직접 의존·함수·전역 참조', '']
for x in files:
    p = x['file']
    doc += [f'### {p}', '', roles[Path(p).name] + '.', '']
    owned = sorted({i['resolved_target'] for i in by_inc[p] if i['kind']=='owned'})
    ext = sorted({i['include_text'] for i in by_inc[p] if i['kind']!='owned'})
    doc.append('자체 include: ' + (', '.join(link(t) for t in owned) or '없음') + '.')
    doc += ['', '직접 피의존: ' + (', '.join(link(t) for t in sorted(dependents[p])) or '없음') + '.']
    doc += ['', '외부/표준 include: ' + (', '.join('`'+i+'`' for i in ext) or '없음') + '.', '']
    doc.append('함수 후보: ' + (', '.join(link(p,f['start_line'],f["name"]+':'+f['start_line']) for f in by_func[p]) or '없음/검출 범위 밖') + '.')
    names = sorted(i['symbol'] for i in by_ref[p])
    doc += ['', '참조 `g_*`: ' + (', '.join('`'+n+'`' for n in names) or '없음') + '.', '',
            f"이름 기반 호출 위치 {call_count[p]}개. 간접 포함 의존 {x['transitive_owned_dependencies']}파일, "
            f"간접 피의존 {x['transitive_dependents']}파일. 상세 위치는 TSV에 기록.", '']

doc += ['## 3. 빌드·리소스·스크립트 전체 목록', '',
        '| 파일 | 줄 | 책임/검토 |', '|---|---:|---|']
for x in support:
    p = x['file']
    if p=='glad.c': role='생성된 외부 OpenGL loader; 변경/중복 통합 제외'
    elif '/test_' in p: role='테스트 빌드/실행; D39'
    elif p.endswith('.rc') or p.endswith('.filters'): role='리소스/IDE 메타데이터; D40'
    else: role='빌드·패키징·릴리스 설정; D40'
    doc.append(f"| {link(p)} | {x['lines']} | {role} |")

external = sorted([p for p in (ROOT/'Include').rglob('*') if p.is_file()] + [ROOT/'WiNILL/System/miniaudio.h', ROOT/'glad.c'])
with (OUT/'external_boundaries.tsv').open('w',encoding='utf-8-sig',newline='') as f:
    w=csv.writer(f,delimiter='\t'); w.writerow(['file','bytes','sha256','scope'])
    for p in external:
        data=p.read_bytes()
        w.writerow([p.relative_to(ROOT).as_posix(),len(data),hashlib.sha256(data).hexdigest(),'external_not_refactoring_target'])
counts=Counter(p.relative_to(ROOT).parts[1] if p.is_relative_to(ROOT/'Include') and len(p.relative_to(ROOT).parts)>2
               else p.relative_to(ROOT).as_posix() for p in external)
doc += ['', '## 4. 외부 경계', '',
        '외부 코드/헤더 파일 목록과 해시는 `external_boundaries.tsv`에 별도 기록했다. '
        '외부 내부 구현의 중복 제거는 이번 범위가 아니다. 소스가 직접 포함한 외부 이름은 위 파일별 include와 includes.tsv에서 확인할 수 있다.', '',
        '| 경계 | 파일 수 |', '|---|---:|']
for name,n in sorted(counts.items()): doc.append(f'| {name} | {n} |')
doc += ['', '## 5. 자료 교차 확인', '',
        f'- 소스 표 {len(files)}행, 함수 후보 {len(funcs)}행, 호출 위치 {len(calls)}행, 보조 파일 {len(support)}행.',
        '- 모든 소스 파일에 수동 책임 분류가 있으며 모든 함수 후보를 파일별 목록에 연결했다.',
        '- 본문의 D01~D40과 자동 후보 판정표를 함께 읽어야 한다. 식별자 정규화 후보 수를 실제 중복 규칙 수로 사용하지 않는다.',
        '- 조사 시점 원본 무변경 검증은 verification.json에 기록한다. 빌드/실행 검증과 구분한다.', '']
(OUT/'File_Coverage.md').write_text('\n'.join(doc),encoding='utf-8')
print(json.dumps({'source_rows':len(files),'functions':len(funcs),'support_rows':len(support),'external_files':len(external)},ensure_ascii=False))

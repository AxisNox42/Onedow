# 경험치·스탯 텔레메트리 로그 읽는 법

이 문서는 `onedow_gameplay_telemetry.txt`를 이용해 경험치 수급량과 증강 전후 스탯 변화를 확인하는 방법을 설명한다.

## 1. 로그 기록 시작하기

1. 게임 설정에서 `DEBUG MODE`를 켠다.
2. 게임 화면에서 `F1`을 눌러 디버그 콘솔을 연다.
3. `능력치` 탭 아래의 `밸런스 테스트: 꺼짐`을 눌러 `밸런스 테스트: 켜짐`으로 만든다.
4. 장비와 증강을 선택하고 게임을 시작한다.

로그는 실행 중인 `WiNILL.exe`와 같은 폴더에 생성된다.

```text
onedow_gameplay_telemetry.txt
```

Debug 빌드와 Release 빌드 모두 같은 방식으로 기록한다. 밸런스 테스트가 켜져 있지 않으면 경험치나 스탯 이벤트를 기록하지 않는다. 테스트를 끄면 현재 런의 로그를 종료하고, 다시 켜면 다음 유효한 런부터 새 구간을 기록한다.

기본 샘플 주기는 **10초**다. 로그 파일은 이어쓰기 방식이므로 여러 번 실행한 결과가 한 파일에 쌓인다.

## 2. 로그 전체 구조

한 런은 다음 순서로 기록된다.

```text
RUN_BEGIN  : 런 시작 정보
SAMPLE     : 주기적인 경험치·요약 스탯
LEVEL_UP   : 레벨업 순간의 경험치 정보
AUGMENT    : 증강 적용 전후의 스탯 변화
RUN_END    : 런 종료 정보
```

각 줄은 대체로 다음 형식이다.

```text
기록종류,key=value,key=value,key=value
```

`#`로 시작하는 줄은 필드 설명용 주석이다. `RUN_BEGIN`부터 다음 구분선까지를 하나의 런으로 보면 된다.

## 3. `SAMPLE` 읽기

`SAMPLE`은 기본적으로 약 10초마다 한 번씩 출력된다. 경험치 획득 속도를 볼 때 가장 먼저 확인할 기록이다.

```text
SAMPLE,game_sec=30.01,level=3,current_xp=240,required_xp=500,progress_pct=48.00,interval_xp=120,interval_xp_per_sec=12.00,interval_xp_per_min=720.00,total_xp=840,max_hp=100.00,damage_multiplier=1.15,effective_damage=23.00,fire_interval_sec=0.42,move_speed_multiplier=1.00,regen_per_sec=0.00,xp_multiplier=1.10,xp_per_sec=0.00,total_augments=2,kill_count=47
```

| 필드 | 의미 |
|---|---|
| `game_sec` | 런 시작 후 경과한 게임 시간(초) |
| `level` | 현재 레벨 |
| `current_xp` | 현재 레벨에서 보유 중인 경험치 |
| `required_xp` | 현재 레벨업에 필요한 경험치 |
| `progress_pct` | 현재 레벨 진행률(%) |
| `interval_xp` | 직전 샘플 이후 획득한 경험치 |
| `interval_xp_per_sec` | 샘플 구간의 초당 경험치 |
| `interval_xp_per_min` | 샘플 구간의 분당 경험치(XP/min) |
| `total_xp` | 기록 시작 후 누적된 경험치 획득량 |
| `max_hp` | 최대 체력 |
| `damage_multiplier` | 피해 배율 |
| `effective_damage` | 0 거리 기준 유효 피해 |
| `fire_interval_sec` | 발사 간격(초) |
| `move_speed_multiplier` | 이동 속도 배율 |
| `regen_per_sec` | 초당 회복량 |
| `xp_multiplier` | 경험치 배율 |
| `xp_per_sec` | 지속적으로 발생하는 초당 경험치 수치 |
| `total_augments` | 획득한 증강 수 |
| `kill_count` | 처치 수 |

### 분당 경험치 계산

코드에서 사용하는 계산식은 다음과 같다.

```text
interval_xp_per_sec = interval_xp / interval_seconds
interval_xp_per_min = interval_xp_per_sec * 60
```

예를 들어 10초 동안 경험치 120을 얻었다면:

```text
120 / 10 * 60 = 720 XP/min
```

런 시작 직후의 첫 `SAMPLE`은 기준점이므로 `interval_xp`와 분당 경험치가 0에 가깝다. 런 종료 직전에 10초가 지나지 않았더라도 마지막 구간을 계산해 기록한다.

## 4. `LEVEL_UP` 읽기

`LEVEL_UP`은 실제 레벨업이 발생할 때마다 한 줄씩 기록된다.

```text
LEVEL_UP,game_sec=42.50,level_before=2,level_after=3,required_xp=500,xp_before=490,xp_after=0,level_interval_sec=18.20,level_interval_xp=206,level_interval_xp_per_min=679.12,run_total_xp=706,run_average_xp_per_min=997.18,source=experience
```

| 필드 | 의미 |
|---|---|
| `level_before`, `level_after` | 레벨업 전후 레벨 |
| `required_xp` | 이번 레벨업에 필요했던 경험치 |
| `xp_before`, `xp_after` | 레벨업 직전·직후의 현재 레벨 경험치 |
| `level_interval_sec` | 직전 레벨업 이후 걸린 시간 |
| `level_interval_xp` | 직전 레벨업 이후 누적된 경험치 획득량 |
| `level_interval_xp_per_min` | 해당 레벨 구간의 분당 경험치 |
| `run_total_xp` | 런 전체 누적 경험치 획득량 |
| `run_average_xp_per_min` | 런 시작부터 현재까지의 평균 분당 경험치 |
| `source` | `experience`는 일반 레벨업, `manual_shortcut`은 디버그 F키 레벨업 |

레벨별 성장 속도를 비교할 때는 `level_interval_xp_per_min`을 사용한다. 전체 런의 평균 효율을 보고 싶으면 `run_average_xp_per_min`을 사용한다.

`current_xp`와 `xp_after`는 레벨업 때 필요한 경험치를 차감한 뒤의 값이다. 반면 `total_xp`, `level_interval_xp`는 기록된 경험치 획득량을 누적한 값이므로 레벨업으로 차감되지 않는다.

## 5. `AUGMENT` 읽기

`AUGMENT`는 증강을 적용한 직후 출력된다. 헤더 한 줄 다음에 여러 개의 `stat=` 줄이 이어진다.

```text
AUGMENT,game_sec=42.50,level=3,index=17,code=SNIPER
  stat=damage_multiplier,before=1.0000,after=1.1500,delta=0.1500
  stat=effective_damage_at_zero_distance,before=20.0000,after=23.0000,delta=3.0000
  stat=crit_chance_pct,before=5,after=15,delta=10
  stat=sniper,before=0,after=1,delta=1
```

### 증강 헤더

| 필드 | 의미 |
|---|---|
| `game_sec` | 증강을 선택한 게임 시간 |
| `level` | 증강 선택 당시 레벨 |
| `index` | `ALL_AUGS` 배열의 증강 인덱스 |
| `code` | 증강 내부 이름 |

### 변화량 줄

```text
stat=이름,before=적용 전,after=적용 후,delta=after-before
```

- 실수형 스탯은 소수점 4자리까지 기록한다.
- 정수형 스탯은 정수로 기록한다.
- 불리언 스탯은 `0=꺼짐`, `1=켜짐`이다.
- `delta`가 양수면 값이 증가했고, 음수면 감소했다.
- `fire_interval_sec`처럼 낮을수록 좋은 스탯은 `delta`가 음수인지 확인해야 한다.

주요 변화 항목은 피해, 체력, 탄속, 발사 간격, 이동 속도, 회복, 경험치 배율, 치명타, 관통, 드론·레이저·차크람 수, 처치 수, 무기·증강 활성화 여부다. `effective_damage_at_zero_distance`는 기본 피해에 0 거리 기준 피해 배율을 적용한 값이다.

## 6. `RUN_BEGIN`과 `RUN_END`

### `RUN_BEGIN`

```text
RUN_BEGIN,serial=4,timestamp=2026-09-21_22-10-00,build=Release,mode=normal,sample_interval_sec=10.00
```

| 필드 | 의미 |
|---|---|
| `serial` | 프로그램 실행 중 런 번호 |
| `timestamp` | 런 시작 시각 |
| `build` | `Debug` 또는 `Release` |
| `mode` | `normal` 또는 `creative` |
| `sample_interval_sec` | 샘플 주기 |

### `RUN_END`

```text
RUN_END,game_sec=180.40,level=8,current_xp=210,total_xp=3560,average_xp_per_min=1185.22,reason=gameover
```

`reason`은 런이 끝난 원인을 나타낸다.

| reason | 의미 |
|---|---|
| `victory` | 승리 |
| `gameover` | 사망 또는 게임 오버 |
| `abandoned` | 중도 포기 |
| `reset_or_restart` | 재시작 또는 런 리셋 |
| `application_exit` | 프로그램 종료 |
| `balance_test_disabled` | 밸런스 테스트 끄기 |
| `debug_mode_disabled` | 디버그 모드 끄기 |
| `replaced_by_new_run` | 새 런으로 교체됨 |

## 7. 밸런스 분석 방법

### 경험치 효율 비교

1. 같은 모드와 비슷한 시작 조건으로 3회 이상 플레이한다.
2. 각 런의 `RUN_END.average_xp_per_min`을 먼저 비교한다.
3. 특정 구간의 효율은 `SAMPLE.interval_xp_per_min`을 비교한다.
4. 레벨별 성장 속도는 `LEVEL_UP.level_interval_xp_per_min`을 비교한다.
5. 한 번의 10초 샘플보다 여러 샘플의 평균이나 중앙값을 사용하는 것이 안정적이다.

### 증강 효율 비교

1. `AUGMENT`의 `code`로 어떤 증강인지 확인한다.
2. `stat=...` 줄에서 `before`, `after`, `delta`를 비교한다.
3. 피해·체력·경험치 배율은 양의 `delta`가 일반적으로 유리하다.
4. 발사 간격·쿨다운은 음의 `delta`가 유리할 수 있다.
5. 증강 직후 다음 `SAMPLE`의 경험치와 처치 수 변화를 함께 확인한다.

## 8. 해석할 때 주의할 점

- 로그는 밸런스 테스트를 켠 시점부터의 구간만 기록한다. 켜기 전 플레이 내용은 복구되지 않는다.
- `total_xp`는 게임 UI의 현재 경험치가 아니라 기록된 경험치 획득량의 누적값이다.
- 디버그 F키로 강제 레벨업한 기록은 `source=manual_shortcut`이므로 일반 플레이의 레벨업과 분리해서 본다.
- 프로그램을 강제 종료하면 마지막 `RUN_END`가 없을 수 있다. 이 경우 다음 `RUN_BEGIN` 또는 파일 끝을 기준으로 마지막 런을 확인한다.
- 여러 런이 한 파일에 이어지므로 분석 전에 `RUN_BEGIN` 단위로 나누는 것이 좋다.

## 9. 빠른 확인 순서

```text
1. RUN_END.average_xp_per_min          전체 런 효율
2. SAMPLE.interval_xp_per_min          10초 구간 효율
3. LEVEL_UP.level_interval_xp_per_min  레벨별 효율
4. AUGMENT.stat ... delta              증강 전후 변화
5. RUN_END.reason                      런 종료 원인
```

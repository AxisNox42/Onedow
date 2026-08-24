# Enemy Constellation Concept

## Active Demo Roster

- Process / 성진: 가장 작은 단일 별점. 플레이어에게 직선 접근하는 기본 추격체.
- Adware / 펄사: 원거리에서 멈춰 팝업 탄을 쏘는 맥동 별.
- DDoS / 성진류: 작은 삼각 성진 파편들이 물량으로 몰려오는 흐름. 개별 보상은 낮고 군집 압박을 담당.
- Botnet / 성군: 여러 성진류를 제어하는 별자리 허브. 더 이상 Process를 소환하지 않고 DDoS 파편을 방출.

## Implementation Notes

- `MobKind::SPAWNER` summon output changed from a normal `Monster` process to `MobKind::DDOS`.
- Botnet visual uses a central hub plus orbiting DDoS shard satellites so the summon relationship is readable.
- Process and DDoS visuals now lean into dot-line constellation silhouettes.

## Later Cleanup

- Current legacy kinds can still exist through old spawn tables, trials, augments, or codex data.
- When the four-enemy roster is locked, remove or disable spawn branches for Worm, Trojan, Crasher, Bug, Kernel, Spyware, Firewall, Bad Sector, Registry Error, Bomber/Ransomware, and their related augments/debuff hooks.

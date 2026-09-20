# PLAY Page v2 implementation

This is the implementation addendum for the PLAY page redesign.

## Layout

- Center: one large, non-clickable weapon hero constellation with a depth-scaled orbital network.
- Upper-left: `RIFLE / FIELD` weapon tabs.
- Left detail column: selected trial explanation with wrapping and an independent scroll viewport.
- Right column: all 20 `TRIAL_DEFS`, grouped as `EARLY / MID / LATE / BOSS`.
- Bottom-left shared rail: `BACK` only.
- Immediately to the right of `BACK`: `PLAY / START RUN`.

## Trial list language

Each row intentionally uses a three-line decision summary:

1. Trial identifier and `ON / OFF` state.
2. Compact effect grammar, for example `BOMBERS EARLIER + FASTER`.
3. Payoff and pressure tags, for example `MID / SCORE +20%`.

Long sentences stay in the left detail column and are clipped/scrollable there. This keeps the catalogue readable even when a trial description becomes a full sentence.

The catalogue permits zero to four active trials. A fifth activation is rejected with `MAX 4 ACTIVE`. When `PLAY` is pressed, the selected definitions are packed into the existing four gameplay trial slots.

## Visual language

`CircleTexture` is used for the constellation's local dark contrast and colored light layers. `bg_linear.png` is bounded to the BACK/PLAY controls. The constellation does not use the parallelogram texture.

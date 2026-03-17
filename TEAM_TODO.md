# F1 Reaction Game - Team TODO

## Goal
Build a simple F1-inspired reaction-time game:
- A start light sequence using LEDs.
- Reaction timer starts when lights turn off.
- Player presses a button as fast as possible.
- Show result and optional leaderboard.

No gameplay code yet in this phase. We are defining process and tasks only.

## Team Rules
- Keep changes small and testable.
- One feature per commit.
- Update this file when priorities change.
- Do not add advanced features before core game loop works.

## MVP Scope (First Playable Version)
- 1 player mode.
- LED start sequence (red lights style, simple version).
- Start timing exactly when LEDs go OFF.
- One reaction button input.
- Show reaction time result.
- Basic retry flow.

## Hardware Assumptions (to confirm)
- Arduino-compatible board.
- 3 to 5 LEDs for start sequence.
- 1 reaction button.
- Optional: OLED display (menu + feedback).
- Optional: 2 navigation buttons (up/down or next/select).

## Work Plan
1. Confirm hardware and pin map.
2. Define game states on paper (idle, countdown, go, result, menu).
3. Build and test LED sequence only.
4. Build and test button reading/debouncing only.
5. Add timer measurement and result calculation.
6. Add OLED basic screens.
7. Add menu navigation.
8. Add simple settings and leaderboard.
9. Final test + polish.

## Game State Checklist
- `IDLE`: waiting to start.
- `COUNTDOWN`: LED sequence running.
- `GO`: LEDs off, timer active.
- `RESULT`: reaction shown.
- `MENU`: settings/leaderboard navigation.

## Menu (Simple)
- Start Game
- Leaderboard
- Settings

### Settings (Very Simple)
- `Show results at end of game`: ON/OFF

## Leaderboard (Simple Version)
- Keep top 5 fastest times.
- Store in RAM first (persistence later if needed).
- Display ranked list on OLED.

## UX Notes
- Keep text short and readable on OLED.
- Use milliseconds as display unit.
- Include false-start handling (button pressed too early) in later step.

## Definition of Done (MVP)
- A user can start a round and get a reliable reaction time.
- Timing starts when LEDs turn OFF.
- Result appears at end of game (if setting ON).
- Leaderboard shows top times.
- Menu navigation is stable and understandable.

## Open Questions
- Exact OLED model and library choice?
- Final number of LEDs?
- Need persistent leaderboard across power cycles?
- Should false starts invalidate the run or add penalty?

## Next Team Action
At next step, we only decide:
1. Exact components and pin mapping.
2. Final MVP state diagram.
3. Menu button scheme (2-button or 3-button).

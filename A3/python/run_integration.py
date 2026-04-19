#!/usr/bin/env python3
"""Deterministic system integration test runner for Treasure Runner."""

import os
import argparse
from treasure_runner.bindings import Direction
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.models.exceptions import GameError, ImpassableError


def get_state(engine: GameEngine) -> dict:
    player = engine.player
    room = player.get_room()
    x, y = player.get_position()
    collected = player.get_collected_count()
    return {"room": room, "x": x, "y": y, "collected": collected}


def fmt_state(state: dict) -> str:
    return f"room={state['room']}|x={state['x']}|y={state['y']}|collected={state['collected']}"


def try_move(engine: GameEngine, direction: Direction) -> str:
    try:
        engine.move_player(direction)
        return "OK"
    except ImpassableError:
        return "BLOCKED"
    except GameError:
        return "BLOCKED"


def run_sweep(engine: GameEngine, direction: Direction, phase: str, step: int, lines: list) -> int:
    dir_name = direction.name
    lines.append(f"SWEEP_START|phase={phase}|dir={dir_name}")

    moves = 0
    while True:
        before = get_state(engine)
        result = try_move(engine, direction)
        after = get_state(engine)
        delta = after["collected"] - before["collected"]
        step += 1

        lines.append(
            f"MOVE|step={step}|phase={phase}|dir={dir_name}|result={result}"
            f"|before={fmt_state(before)}|after={fmt_state(after)}|delta_collected={delta}"
        )

        if result == "BLOCKED":
            lines.append(f"SWEEP_END|phase={phase}|reason=BLOCKED|moves={moves}")
            break
        moves += 1

    return step


def run_integration(config_path: str, log_path: str) -> None:
    engine = GameEngine(config_path)

    room_count = engine.get_room_count()
    width, height = engine.get_room_dimensions()

    lines = []
    lines.append(
        f"RUN_START|config={config_path}|rooms={room_count}"
        f"|room_width={width}|room_height={height}"
    )

    step = 0
    spawn = get_state(engine)
    lines.append(f"STATE|step={step}|phase=SPAWN|state={fmt_state(spawn)}")

    entry_dir = None
    spawn_room = spawn["room"]
    spawn_x, spawn_y = spawn["x"], spawn["y"]

    for candidate in [Direction.SOUTH, Direction.EAST, Direction.NORTH, Direction.WEST]:
        before_probe = get_state(engine)
        try:
            engine.move_player(candidate)
            after_probe = get_state(engine)
            if after_probe["x"] != before_probe["x"] or after_probe["y"] != before_probe["y"] or after_probe["room"] != before_probe["room"]:
                engine.reset()
                entry_dir = candidate
                break
        except (ImpassableError, GameError):
            continue

    if entry_dir is None:
        entry_dir = Direction.SOUTH

    lines.append(f"ENTRY|direction={entry_dir.name}")

    before = get_state(engine)
    result = try_move(engine, entry_dir)
    after = get_state(engine)
    delta = after["collected"] - before["collected"]
    step += 1
    lines.append(
        f"MOVE|step={step}|phase=ENTRY|dir={entry_dir.name}|result={result}"
        f"|before={fmt_state(before)}|after={fmt_state(after)}|delta_collected={delta}"
    )

    sweep_order = [
        (Direction.SOUTH, "SWEEP_SOUTH"),
        (Direction.WEST,  "SWEEP_WEST"),
        (Direction.NORTH, "SWEEP_NORTH"),
        (Direction.EAST,  "SWEEP_EAST"),
    ]

    for direction, phase in sweep_order:
        step = run_sweep(engine, direction, phase, step, lines)

    final = get_state(engine)
    lines.append(f"STATE|step={step}|phase=FINAL|state={fmt_state(final)}")
    lines.append(f"RUN_END|steps={step}|collected_total={final['collected']}")

    engine.destroy()

    with open(log_path, "w") as f:
        f.write("\n".join(lines) + "\n")

    for line in lines:
        print(line)


def parse_args():
    parser = argparse.ArgumentParser(description="Treasure Runner integration test logger")
    parser.add_argument(
        "--config",
        required=True,
        help="Path to generator config file",
    )
    parser.add_argument(
        "--log",
        required=True,
        help="Output log path",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    config_path = os.path.abspath(args.config)
    log_path = os.path.abspath(args.log)
    run_integration(config_path, log_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
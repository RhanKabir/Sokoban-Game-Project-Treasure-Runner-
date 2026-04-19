import sys
import os
import argparse
import curses
import json
from datetime import datetime, timezone

def main():
    parser = argparse.ArgumentParser(description="Treasure Runner")
    parser.add_argument("--config", required=True, help="Path to the .ini config file")
    parser.add_argument("--profile", required=True, help="Path to the player profile JSON file")
    args = parser.parse_args()

    # Add the python directory to sys.path so treasure_runner is importable
    script_dir = os.path.dirname(os.path.abspath(__file__))
    if script_dir not in sys.path:
        sys.path.insert(0, script_dir)

    from treasure_runner.models.game_engine import GameEngine
    from treasure_runner.ui.game_ui import GameUI

    # Load or create profile
    profile = load_or_create_profile(args.profile)

    # Create engine
    engine = GameEngine(args.config)

    try:
        # Launch the UI via curses wrapper
        curses.wrapper(lambda stdscr: run_ui(stdscr, engine, profile, args.profile))
    finally:
        engine.destroy()


def load_or_create_profile(profile_path: str) -> dict:
    if os.path.exists(profile_path):
        with open(profile_path, "r") as f:
            return json.load(f)
    else:
        # Prompt for name before curses takes over the terminal
        name = input("No profile found. Enter your player name: ").strip()
        if not name:
            name = "Player"
        profile = {
            "player_name": name,
            "games_played": 0,
            "max_treasure_collected": 0,
            "most_rooms_world_completed": 0,
            "timestamp_last_played": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        }
        os.makedirs(os.path.dirname(os.path.abspath(profile_path)), exist_ok=True)
        with open(profile_path, "w") as f:
            json.dump(profile, f, indent=2)
        print(f"Profile created at {profile_path}")
        return profile


def save_profile(profile_path: str, profile: dict) -> None:
    profile["timestamp_last_played"] = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    with open(profile_path, "w") as f:
        json.dump(profile, f, indent=2)


def run_ui(stdscr, engine, profile, profile_path: str) -> None:
    from treasure_runner.ui.game_ui import GameUI

    ui = GameUI(stdscr, engine, profile)
    ui.run()

    # After the game ends, update and save profile stats
    profile["games_played"] = profile.get("games_played", 0) + 1
    collected = engine.player.get_collected_count()
    if collected > profile.get("max_treasure_collected", 0):
        profile["max_treasure_collected"] = collected

    save_profile(profile_path, profile)


if __name__ == "__main__":
    main()
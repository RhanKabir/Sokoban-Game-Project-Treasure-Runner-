import unittest
import os

from treasure_runner.models.game_engine import GameEngine
from treasure_runner.bindings import Direction


CONFIG_PATH = CONFIG_PATH = os.path.join(os.path.dirname(__file__), "..", "..", "assets", "starter.ini")


class TestGameEngine(unittest.TestCase):

    def setUp(self):
        self.engine = GameEngine(CONFIG_PATH)

    def tearDown(self):
        self.engine.destroy()

    def test_engine_creation(self):
        self.assertIsNotNone(self.engine)

    def test_player_exists(self):
        player = self.engine.player
        self.assertIsNotNone(player)

    def test_get_room_count(self):
        count = self.engine.get_room_count()
        self.assertIsInstance(count, int)
        self.assertGreater(count, 0)

    def test_get_room_dimensions(self):
        width, height = self.engine.get_room_dimensions()

        self.assertIsInstance(width, int)
        self.assertIsInstance(height, int)
        self.assertGreater(width, 0)
        self.assertGreater(height, 0)

    def test_render_current_room(self):
        room_str = self.engine.render_current_room()

        self.assertIsInstance(room_str, str)
        self.assertGreater(len(room_str), 0)

    def test_get_room_ids(self):
        ids = self.engine.get_room_ids()

        self.assertIsInstance(ids, list)
        self.assertGreater(len(ids), 0)

        for room_id in ids:
            self.assertIsInstance(room_id, int)

    def test_move_player(self):
        try:
            self.engine.move_player(Direction.NORTH)
        except Exception:
            pass

    def test_reset_engine(self):
        self.engine.reset()
        room = self.engine.player.get_room()
        self.assertIsInstance(room, int)

class TestPlayer(unittest.TestCase):

    def setUp(self):
        self.engine = GameEngine(CONFIG_PATH)
        self.player = self.engine.player

    def tearDown(self):
        self.engine.destroy()

    def test_get_room(self):
        room = self.player.get_room()

        self.assertIsInstance(room, int)

    def test_get_position(self):
        x, y = self.player.get_position()

        self.assertIsInstance(x, int)
        self.assertIsInstance(y, int)

    def test_get_collected_count(self):
        count = self.player.get_collected_count()

        self.assertIsInstance(count, int)
        self.assertGreaterEqual(count, 0)

    def test_has_collected_treasure_false(self):
        result = self.player.has_collected_treasure(0)

        self.assertIsInstance(result, bool)

    def test_get_collected_treasures(self):
        treasures = self.player.get_collected_treasures()

        self.assertIsInstance(treasures, list)

        for t in treasures:
            self.assertIsInstance(t, dict)

            self.assertIn("id", t)
            self.assertIn("name", t)
            self.assertIn("starting_room_id", t)
            self.assertIn("initial_x", t)
            self.assertIn("initial_y", t)
            self.assertIn("x", t)
            self.assertIn("y", t)
            self.assertIn("collected", t)


if __name__ == "__main__":
    unittest.main()
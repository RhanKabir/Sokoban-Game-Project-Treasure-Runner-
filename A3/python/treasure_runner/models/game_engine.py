import ctypes
from ..bindings import lib, Status, Direction, Treasure
from .player import Player
from .exceptions import status_to_exception

class GameEngine:
    def __init__(self, config_path: str):
        self._eng = ctypes.c_void_p()
        status = lib.game_engine_create(config_path.encode("utf-8"), ctypes.byref(self._eng))
        if status != Status.OK:
            raise status_to_exception(status, f"Failed to create GameEngine from {config_path}")
        player_ptr = lib.game_engine_get_player(self._eng)
        if not player_ptr:
            raise RuntimeError("Failed to retrieve player from GameEngine")
        self._player = Player(player_ptr)

        self._destroyed = False

    @property
    def player(self) -> Player:
        return self._player

    def destroy(self) -> None:
        if not self._destroyed and self._eng:
            lib.game_engine_destroy(self._eng)
            self._eng = None
            self._destroyed = True

    def move_player(self, direction: Direction) -> None:
        status = lib.game_engine_move_player(self._eng, direction)
        if status != Status.OK:
            raise status_to_exception(status, f"Failed to move player {direction.name}")

    def render_current_room(self) -> str:
        c_str = ctypes.c_char_p()
        status = lib.game_engine_render_current_room(self._eng, ctypes.byref(c_str))
        if status != Status.OK:
            raise status_to_exception(status, "Failed to render current room")

        result = ctypes.string_at(c_str).decode("utf-8")
        lib.game_engine_free_string(c_str)
        return result

    def get_room_count(self) -> int:
        count = ctypes.c_int()
        status = lib.game_engine_get_room_count(self._eng, ctypes.byref(count))

        if status != Status.OK:
            raise status_to_exception(status, "Failed to get room count")
        return count.value

    def get_room_dimensions(self) -> tuple[int, int]:
        width = ctypes.c_int()
        height = ctypes.c_int()

        status = lib.game_engine_get_room_dimensions(self._eng, ctypes.byref(width), ctypes.byref(height))
        if status != Status.OK:
            raise status_to_exception(status, "Failed to get room dimensions")
        return (width.value, height.value)

    def get_room_ids(self) -> list[int]:
        ids_ptr = ctypes.POINTER(ctypes.c_int)()
        count = ctypes.c_int()

        status = lib.game_engine_get_room_ids(self._eng, ctypes.byref(ids_ptr), ctypes.byref(count))
        if status != Status.OK:
            raise status_to_exception(status, "Failed to get room IDs")

        ids = []
        for i in range(count.value):
            ids.append(ids_ptr[i])

        lib.game_engine_free_string(ids_ptr)
        return ids

    def get_all_treasures(self) -> int:
        count = ctypes.c_int()

        status = lib.game_engine_get_all_treasures(self._eng, ctypes.byref(count))
        if status != Status.OK:
            raise status_to_exception(status, "Failed to get treasure count")
        return count.value

    def get_adjacency_matrix(self) -> list[list[int]]:
        matrix_ptr = ctypes.POINTER(ctypes.c_int)()
        count = ctypes.c_int()

        status = lib.game_engine_get_adjacency_matrix(self._eng, ctypes.byref(matrix_ptr), ctypes.byref(count))
        if status != Status.OK:
            raise status_to_exception(status, "Failed to get adjacency matrix")

        n = count.value

        matrix = []
        for i in range(n):
            row = []
            for j in range(n):
                row.append(matrix_ptr[i * n + j])
            matrix.append(row)

        return matrix

    def reset(self) -> None:
        status = lib.game_engine_reset(self._eng)
        if status != Status.OK:
            raise status_to_exception(status, "Failed to reset")

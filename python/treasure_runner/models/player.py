import ctypes
from ..bindings import lib, Status, Direction, Treasure
from .exceptions import status_to_exception

class Player:
    def __init__(self, ptr):
        self._ptr = ptr

    def get_room(self) -> int:
        room_id = lib.player_get_room(self._ptr)

        return room_id

    def get_position(self) -> tuple[int, int]:
        x = ctypes.c_int()
        y = ctypes.c_int()

        status = lib.player_get_position(self._ptr, ctypes.byref(x), ctypes.byref(y))

        if status != Status.OK:
            raise status_to_exception(status, "Failed to get player position")
        return (x.value, y.value)

    def get_collected_count(self) -> int:

        return lib.player_get_collected_count(self._ptr)

    def has_collected_treasure(self, treasure_id: int) -> bool:

        return lib.player_has_collected_treasure(self._ptr, treasure_id)

    def get_collected_treasures(self) -> list[dict]:

        count = ctypes.c_int()
        treasure_array = lib.player_get_collected_treasures(self._ptr, ctypes.byref(count))

        if not treasure_array:
            return []

        treasures = []
        for i in range(count.value):
            treasure = treasure_array[i].contents
            treasures.append({
                "id": treasure.id,
                "name": treasure.name.decode("utf-8"),
                "starting_room_id": treasure.starting_room_id,
                "initial_x": treasure.initial_x,
                "initial_y": treasure.initial_y,
                "x": treasure.x,
                "y": treasure.y,
                "collected": treasure.collected
            })
        return treasures

import curses
import math
from ..bindings import Direction 

class GameUI:

    COLOR_DEFAULT = 1
    COLOR_WALL     = 2
    COLOR_TREASURE = 3
    COLOR_PLAYER   = 4
    COLOR_EXIT     = 5
    COLOR_PUSHABLE = 6

    def __init__(self, stdscr, engine, profile) -> None:
        self._stdscr = stdscr
        self._engine = engine
        self._profile = profile
        self._message = ""
        self._running = True
        self._visited = set()

    def init_screen(self) -> None:
        curses.cbreak() # Don't wait for Enter
        curses.noecho() # Don't show typed characters
        self._stdscr.keypad(True) # Enable arrow keys
        curses.start_color() # Enable color support
        curses.use_default_colors() # Use terminal defaults
        self._init_colors()
        self._clear_screen()

    def _init_colors(self) -> None:
        try:
            curses.init_pair(self.COLOR_DEFAULT,  curses.COLOR_WHITE,  -1)
            curses.init_pair(self.COLOR_WALL,     curses.COLOR_WHITE,  -1)
            curses.init_pair(self.COLOR_TREASURE, curses.COLOR_YELLOW, -1)
            curses.init_pair(self.COLOR_PLAYER,   curses.COLOR_CYAN,   -1)
            curses.init_pair(self.COLOR_EXIT,     curses.COLOR_GREEN,  -1)
            curses.init_pair(self.COLOR_PUSHABLE, curses.COLOR_MAGENTA,-1)
        except curses.error:
            pass

    def _clear_screen(self) -> None:
        self._stdscr.clear()
        self._stdscr.refresh()

    def show_splash(self) -> None:
        self._stdscr.erase()
        max_y, max_x = self._stdscr.getmaxyx()
        if max_y < 10 or max_x < 40:
            self._stdscr.addstr(0, 0, f"Terminal too small! Got {max_x}x{max_y}, need 40x10")
            self._stdscr.refresh()
            self._stdscr.getch()
            self._running = False
        return
        self._stdscr.addstr(0, 2, "== TREASURE RUN ==", curses.color_pair(self.COLOR_TREASURE) | curses.A_BOLD)
        self._draw_profile()
        self._stdscr.addstr(8, 2, "Press any key to start ...")
        self._stdscr.refresh()
        self._stdscr.getch()

    def show_quit_screen(self) -> None:
        self._stdscr.erase()
        self._stdscr.addstr(0, 2, "== GAME OVER ==", curses.color_pair(self.COLOR_TREASURE) | curses.A_BOLD)
        self._draw_profile()
        self._stdscr.addstr(8, 2, "Press any key to exit ...")
        self._stdscr.refresh()
        self._stdscr.getch()

    def _draw_profile(self) -> None:
        p = self._profile
        try:
            self._stdscr.addstr(2, 2, f"Player:       {p.get('player_name', '?')}")
            self._stdscr.addstr(3, 2, f"Games played: {p.get('games_played', 0)}")
            self._stdscr.addstr(4, 2, f"Best treasure:{p.get('max_treasure_collected', 0)}")
            self._stdscr.addstr(5, 2, f"Most rooms:{p.get('most_rooms_world_completed', 0)}")
            self._stdscr.addstr(6, 2, f"Last played:  {p.get('timestamp_last_played', 'never')}")
        except curses.error:
            pass


    def run(self):
        self.init_screen()
        self.show_splash()
        self._clear_screen()

        while self._running == True:
            self.render()
            key = self._stdscr.getch()
            self._handle_key(key)
        
        self.show_quit_screen()

    def _handle_key(self, key: int) -> None:
        if key == ord('q'):
            self._running = False
            return
        if key == ord('r'):
            self._engine.reset()
            self.message("Game reset")
            return
        
        direction = self.read_direction(key)

        if direction is not None:
            try:
                self._engine.move_player(direction)
                collected = self._engine.player.get_collected_count()
                total = self._engine.get_all_treasures()
                self.message(f"{collected}/{total} treasures collected")
                if self._check_victory():
                    self._running = False
            except Exception as e:
                self.message(str(e))
    
    def render(self) -> None:
        self._stdscr.erase()
        self._check_terminal_size()
        self._draw_message()
        self._draw_room()
        self._draw_game_controls()
        self._draw_player_status()
        self._draw_legend()
        self._draw_title_bar() 
        self._draw_minimap()
        self._stdscr.refresh()

    def _check_terminal_size(self) -> None:
        width, height = self._engine.get_room_dimensions()
        max_y, max_x = self._stdscr.getmaxyx()
        n = self._engine.get_room_count()
        cell_width = 5
        cols = math.ceil(math.sqrt(n))
        need_cols = width + 20 + (cols * cell_width)
        need_rows = height + 3 + 8

        if max_y < need_rows or max_x < need_cols:
            self._stdscr.erase()
            try:
                self._stdscr.addstr(0, 0,
                    f"Terminal too small! Need {need_cols}x{need_rows}, got {max_x}x{max_y}",
                    curses.color_pair(self.COLOR_TREASURE) | curses.A_BOLD)
                self._stdscr.addstr(1, 0, "Please resize your terminal and press any key...")
            except curses.error:
                pass
            self._stdscr.refresh()
            self._stdscr.getch()
            self._running = False
            raise RuntimeError(f"Terminal too small")



    def _draw_minimap(self) -> None:
        matrix = self._engine.get_adjacency_matrix()
        if not matrix:
            return

        n = self._engine.get_room_count()
        current = self._engine.player.get_room()
        self._visited.add(current)
        room_ids = self._engine.get_room_ids()

        max_y, max_x = self._stdscr.getmaxyx()
        cell_width = 5
        cell_height = 2
        cols = math.ceil(math.sqrt(n))
        rows = math.ceil(n / cols)

        width, height = self._engine.get_room_dimensions()
        start_col = width + 20
        start_row = 3

        for i in range(n):
            room_id = room_ids[i]
            grid_row = i // cols
            grid_col = i % cols
            draw_row = start_row + grid_row * cell_height
            draw_col = start_col + grid_col * cell_width

            if room_id == current:
                attr = curses.color_pair(self.COLOR_PLAYER) | curses.A_BOLD
            elif room_id in self._visited:
                attr = curses.color_pair(self.COLOR_EXIT)
            else:
                attr = curses.color_pair(self.COLOR_DEFAULT)

            label = f"[{room_id:2}]"
            try:
                self._stdscr.addstr(draw_row, draw_col, label, attr)
            except curses.error:
                pass

            for j in range(n):
                if matrix[i][j] == 1:
                    neighbor_row = j // cols
                    neighbor_col = j % cols
                    if neighbor_row == grid_row and neighbor_col == grid_col + 1:
                        try:
                            self._stdscr.addstr(draw_row, draw_col + 4, "-", curses.A_DIM)
                        except curses.error:
                            pass

                    if neighbor_col == grid_col and neighbor_row == grid_row + 1:
                        try:
                            self._stdscr.addstr(draw_row + 1, draw_col + 1, "|", curses.A_DIM)
                        except curses.error:
                            pass


        
    
    def _draw_message(self) -> None:
        self._stdscr.move(0, 0)
        self._stdscr.clrtoeol()
        self._stdscr.addstr(0, 0, self._message)
    
    def _draw_room(self) -> None:
        roomID = self._engine.player.get_room()
        self._stdscr.addstr(1, 0, f"Room {roomID}")

        room_charstring = self._engine.render_current_room()
        width, height = self._engine.get_room_dimensions()

        for row, line in enumerate(room_charstring.splitlines()):
            for col, ch in enumerate(line):
                try:
                    self._stdscr.addch(row + 3, col, ch, self._color_for_tile(ch))
                except curses.error:
                    pass
    
    def _draw_game_controls(self) -> None:
        width, height = self._engine.get_room_dimensions()
        try:
            self._stdscr.addstr(height + 4, 0, f"Game Controls: WASD/Arrows=move  >=portal  r=reset  q=quit")
        except curses.error:
            pass
    
    def _draw_player_status(self) -> None:
        width, height = self._engine.get_room_dimensions()
        name = self._profile.get("player_name", "?")
        treasures = self._engine.player.get_collected_count()
        
        try:
            self._stdscr.addstr(height + 6, 0, f"Player: {name} Gold Collected: {treasures}")
        except curses.error:
            pass
    
    def _draw_legend(self) -> None:
        width, height = self._engine.get_room_dimensions()
        try:
            self._stdscr.addstr(3, width + 2, f"Game Elements:")
            self._stdscr.addstr(5, width + 2, f"@ - player", self._color_for_tile('@'))
            self._stdscr.addstr(6, width + 2, f"# - wall", self._color_for_tile('#'))
            self._stdscr.addstr(7, width + 2, f"$ - gold", self._color_for_tile('$'))
            self._stdscr.addstr(8, width + 2, f"X - exit", self._color_for_tile('X'))
        except curses.error:
            pass
    
    def _draw_title_bar(self) -> None:
        width, height = self._engine.get_room_dimensions()
        try:
            self._stdscr.addstr(height + 7, 0, f"Treasure Run")
            self._stdscr.addstr(height + 7, 40, f"rihan@uoguelph.ca")
        except curses.error:
            pass

    def read_direction(self, key: int):
        if key in (ord('w'), curses.KEY_UP):
            return Direction.NORTH
        elif key in (ord('s'), curses.KEY_DOWN):
            return Direction.SOUTH
        elif key in (ord('a'), curses.KEY_LEFT):
            return Direction.WEST
        elif key in (ord('d'), curses.KEY_RIGHT):
            return Direction.EAST
        else:
            return None

    def _color_for_tile(self, tile: str) -> int:
        if tile == '@':
            return curses.color_pair(self.COLOR_PLAYER) | curses.A_BOLD
        elif tile == '$':
            return curses.color_pair(self.COLOR_TREASURE) | curses.A_BOLD
        elif tile == 'X':
            return curses.color_pair(self.COLOR_EXIT)
        elif tile == '#':
            return curses.color_pair(self.COLOR_WALL) | curses.A_DIM
        elif tile == 'o':
            return curses.color_pair(self.COLOR_PUSHABLE)
        else:
            return curses.color_pair(self.COLOR_DEFAULT)
    
    def _check_victory(self) -> bool:
        collected = self._engine.player.get_collected_count()
        total = self._engine.get_all_treasures()
        return collected >= total

    def message(self, text: str) -> None:
        self._message = text


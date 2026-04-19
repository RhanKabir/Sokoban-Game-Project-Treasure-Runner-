#include <check.h>
#include <stdlib.h>

#include "room.h"
#include "types.h"

/* ============================================================
 * Room fixtures
 * ============================================================ */
static Room *r = NULL;

static void setup_room(void)
{
    r = room_create(123, "TestRoom", 5, 4);
    ck_assert_ptr_nonnull(r);
}

static void teardown_room(void)
{
    room_destroy(r);
    r = NULL;
}

/* ============================================================
 * Room tests
 * ============================================================ */

START_TEST(test_room_create_basic)
{
    ck_assert_int_eq(r->id, 123);
    ck_assert_int_eq(r->width, 5);
    ck_assert_int_eq(r->height, 4);

    ck_assert_ptr_eq(r->floor_grid, NULL);
    ck_assert_ptr_eq(r->portals, NULL);
    ck_assert_int_eq(r->portal_count, 0);
    ck_assert_ptr_eq(r->treasures, NULL);
    ck_assert_int_eq(r->treasure_count, 0);
}
END_TEST

START_TEST(test_room_get_width)
{
    ck_assert_int_eq(room_get_width(r), 5);
    ck_assert_int_eq(room_get_width(NULL), 0);

}
END_TEST

START_TEST(test_room_get_height)
{
    ck_assert_int_eq(room_get_height(r), 4);
    ck_assert_int_eq(room_get_height(NULL), 0);

}
END_TEST

START_TEST(test_room_set_floor_grid_transfers_ownership)
{
    bool *grid = malloc(6 * sizeof(bool));

    grid[0] = true;  grid[1] = false; grid[2] = true;
    grid[3] = false; grid[4] = true;  grid[5] = false;

    ck_assert_int_eq(room_set_floor_grid(r, grid), OK);

    ck_assert_ptr_eq(r->floor_grid, grid);

}
END_TEST

START_TEST(test_room_set_floor_grid_null_room)
{
    bool *grid = malloc(6 * sizeof(bool));

    ck_assert_int_eq(room_set_floor_grid(NULL, grid), INVALID_ARGUMENT);

    free(grid);
}
END_TEST

START_TEST(test_room_set_portals)
{
    Portal *ps = malloc(2 * sizeof(Portal));

    ps[0].id = 1;
    ps[0].x = 0;
    ps[0].y = 1;
    ps[0].target_room_id = 99;

    ps[0].name = malloc(3);
    ps[0].name[0] = 'A';
    ps[0].name[1] = '0';
    ps[0].name[2] = '\0';

    ps[1].id = 2;
    ps[1].x = 3;
    ps[1].y = 2;
    ps[1].target_room_id = 100;

    ps[1].name = malloc(3);
    ps[1].name[0] = 'B';
    ps[1].name[1] = '1';
    ps[1].name[2] = '\0';

    ck_assert_int_eq(room_set_portals(r, ps, 2), OK);

    ck_assert_ptr_eq(r->portals, ps);
    ck_assert_int_eq(r->portal_count, 2);

    ck_assert_int_eq(r->portals[0].id, 1);
    ck_assert_int_eq(r->portals[0].x, 0);
    ck_assert_int_eq(r->portals[0].y, 1);
    ck_assert_int_eq(r->portals[0].target_room_id, 99);
    ck_assert_str_eq(r->portals[0].name, "A0");

    ck_assert_int_eq(r->portals[1].id, 2);
    ck_assert_int_eq(r->portals[1].x, 3);
    ck_assert_int_eq(r->portals[1].y, 2);
    ck_assert_int_eq(r->portals[1].target_room_id, 100);
    ck_assert_str_eq(r->portals[1].name, "B1");
}
END_TEST

START_TEST(test_room_set_treasures)
{
     Treasure *ts = malloc(sizeof(Treasure) * 2);

    ts[0].id = 1;
    ts[0].x = 0;
    ts[0].y = 1;
    ts[0].initial_x = 0;
    ts[0].initial_y = 1;
    ts[0].collected = false;

    ts[0].name = malloc(4);
    ts[0].name[0] = 'G';
    ts[0].name[1] = 'o';
    ts[0].name[2] = 'l';
    ts[0].name[3] = '\0';

    ts[1].id = 2;
    ts[1].x = 2;
    ts[1].y = 2;
    ts[1].initial_x = 2;
    ts[1].initial_y = 2;
    ts[1].collected = false;

    ts[1].name = malloc(5);
    ts[1].name[0] = 'C';
    ts[1].name[1] = 'o';
    ts[1].name[2] = 'i';
    ts[1].name[3] = 'n';
    ts[1].name[4] = '\0';

    ck_assert_int_eq(room_set_treasures(r, ts, 2), OK);

    ck_assert_ptr_eq(r->treasures, ts);
    ck_assert_int_eq(r->treasure_count, 2);

    ck_assert_int_eq(r->treasures[0].id, 1);
    ck_assert_str_eq(r->treasures[0].name, "Gol");

    ck_assert_int_eq(r->treasures[1].id, 2);
    ck_assert_str_eq(r->treasures[1].name, "Coin");

}
END_TEST

START_TEST(test_room_place_treasure)
{
    ck_assert_int_eq(r->treasure_count, 0);
    ck_assert_ptr_eq(r->treasures, NULL);

    Treasure t;
    t.id = 100;

    t.name = malloc(4);
    t.name[0] = 'G';
    t.name[1] = 'o';
    t.name[2] = 'l';
    t.name[3] = '\0';

    t.x = 2; t.y = 3;
    t.initial_x = 2; t.initial_y = 3;
    t.starting_room_id = r->id;
    t.collected = false;

    ck_assert_int_eq(room_place_treasure(r, &t), OK);

    ck_assert_int_eq(r->treasure_count, 1);
    ck_assert_ptr_nonnull(r->treasures);

    ck_assert_int_eq(r->treasures[0].id, 100);
    ck_assert_int_eq(r->treasures[0].x, 2);
    ck_assert_int_eq(r->treasures[0].y, 3);

    ck_assert_int_eq(room_place_treasure(NULL, &t), INVALID_ARGUMENT);
    ck_assert_int_eq(room_place_treasure(r, NULL), INVALID_ARGUMENT);

}
END_TEST

START_TEST(test_room_get_treasure_at)
{
    Treasure t;
    t.id = 100;

    t.name = malloc(4);
    t.name[0] = 'G';
    t.name[1] = 'o';
    t.name[2] = 'l';
    t.name[3] = '\0';

    t.x = 2; t.y = 3;
    t.initial_x = 2; t.initial_y = 3;
    t.starting_room_id = r->id;
    t.collected = false;

    ck_assert_int_eq(room_place_treasure(r, &t), OK);
    ck_assert_int_eq(room_get_treasure_at(r, t.x, t.y), 100);
    ck_assert_int_eq(room_get_treasure_at(NULL, t.x, t.y), -1);
    ck_assert_int_eq(room_get_treasure_at(r, 0, 0), -1);

}
END_TEST

START_TEST(test_room_get_portal_destination)
{
       Portal *ps = malloc(2 * sizeof(Portal));

    ps[0].id = 1;
    ps[0].x = 0;
    ps[0].y = 1;
    ps[0].target_room_id = 99;

    ps[0].name = malloc(3);
    ps[0].name[0] = 'A';
    ps[0].name[1] = '0';
    ps[0].name[2] = '\0';

    ps[1].id = 2;
    ps[1].x = 3;
    ps[1].y = 2;
    ps[1].target_room_id = 100;

    ps[1].name = malloc(3);
    ps[1].name[0] = 'B';
    ps[1].name[1] = '1';
    ps[1].name[2] = '\0';

    ck_assert_int_eq(room_set_portals(r, ps, 2), OK);
    ck_assert_int_eq(room_get_portal_destination(r, 0, 1), 99);
    ck_assert_int_eq(room_get_portal_destination(r, 5, 5), -1);
    ck_assert_int_eq(room_get_portal_destination(NULL, 0, 1), -1);
}
END_TEST


START_TEST(test_room_is_walkable)
{
    bool *grid = malloc(r->width * r->height * sizeof(bool));

    for (int i = 0; i < r->width * r->height; i++) {
        grid[i] = false;
    }

    grid[1 * r->width + 1] = true;

    ck_assert_int_eq(room_set_floor_grid(r, grid), OK); 

    ck_assert(room_is_walkable(r, 1, 1) == true);   
    ck_assert(room_is_walkable(r, 0, 0) == false);  
    ck_assert(room_is_walkable(r, -1, 0) == false); 
    ck_assert(room_is_walkable(NULL, 1, 1) == false);
}
END_TEST

START_TEST(test_room_classify_tile)
{
    bool *grid = malloc(r->width * r->height * sizeof(bool));

    for (int i = 0; i < r->width * r->height; i++) {
        grid[i] = false;
    }
    grid[1 * r->width + 1] = true;

    ck_assert_int_eq(room_set_floor_grid(r, grid), OK);

    Treasure *ts = malloc(sizeof(Treasure));
    ck_assert_ptr_nonnull(ts);

    ts[0].id = 123;
    ts[0].starting_room_id = r->id;
    ts[0].initial_x = 2; ts[0].initial_y = 2;
    ts[0].x = 2; ts[0].y = 2;
    ts[0].collected = false;

    ts[0].name = malloc(2);
    ts[0].name[0] = 'T';
    ts[0].name[1] = '\0';

    ck_assert_int_eq(room_set_treasures(r, ts, 1), OK);

    Portal *ps = malloc(sizeof(Portal));

    ps[0].id = 1;
    ps[0].x = 3; ps[0].y = 3;
    ps[0].target_room_id = 77;

    ps[0].name = malloc(2);
    ps[0].name[0] = 'P';
    ps[0].name[1] = '\0';

    ck_assert_int_eq(room_set_portals(r, ps, 1), OK);

    int out_id = -999;

    ck_assert_int_eq(room_classify_tile(r, -1, 0, &out_id), ROOM_TILE_INVALID);
    ck_assert_int_eq(room_classify_tile(r, 0, 0, &out_id), ROOM_TILE_WALL);

    out_id = -999;
    ck_assert_int_eq(room_classify_tile(r, 1, 1, &out_id), ROOM_TILE_FLOOR);

    out_id = -999;
    ck_assert_int_eq(room_classify_tile(r, 2, 2, &out_id), ROOM_TILE_TREASURE);
    ck_assert_int_eq(out_id, 123);

    out_id = -999;
    ck_assert_int_eq(room_classify_tile(r, 3, 3, &out_id), ROOM_TILE_PORTAL);
    ck_assert_int_eq(out_id, 77);

    ck_assert_int_eq(room_classify_tile(NULL, 1, 1, &out_id), ROOM_TILE_INVALID);
}
END_TEST

START_TEST(test_room_render)
{
    Charset cs;
    cs.wall = '#';
    cs.floor = '.';
    cs.player = '@';
    cs.treasure = '$';
    cs.portal = 'X';
    cs.pushable = 'O';

    bool *grid = malloc(r->width * r->height * sizeof(bool));

    for (int i = 0; i < r->width * r->height; i++) {
        grid[i] = false;
    }
    grid[1 * r->width + 1] = true;
    ck_assert_int_eq(room_set_floor_grid(r, grid), OK);

    Treasure *ts = malloc(sizeof(Treasure));

    ts[0].id = 5;
    ts[0].starting_room_id = r->id;
    ts[0].initial_x = 1; ts[0].initial_y = 1;
    ts[0].x = 1; ts[0].y = 1;
    ts[0].collected = false;

    ts[0].name = malloc(2);
    ts[0].name[0] = 'A';
    ts[0].name[1] = '\0';

    ck_assert_int_eq(room_set_treasures(r, ts, 1), OK);

    Portal *ps = malloc(sizeof(Portal));

    ps[0].id = 9;
    ps[0].x = 2; ps[0].y = 2;
    ps[0].target_room_id = 10;
    ps[0].name = NULL;

    ck_assert_int_eq(room_set_portals(r, ps, 1), OK);

    char *buf = malloc(r->width * r->height);
    ck_assert_ptr_nonnull(buf);

    ck_assert_int_eq(room_render(r, &cs, buf, r->width, r->height), OK);

    ck_assert_int_eq(buf[0 * r->width + 0], '#');

    ck_assert_int_eq(buf[1 * r->width + 1], '$');

    ck_assert_int_eq(buf[2 * r->width + 2], 'X');

    free(buf);
}
END_TEST

START_TEST(test_room_get_start_position)
{
    bool *grid = malloc(r->width * r->height * sizeof(bool));

    for (int i = 0; i < r->width * r->height; i++) {
        grid[i] = false;
    }
    grid[1 * r->width + 1] = true;
    ck_assert_int_eq(room_set_floor_grid(r, grid), OK);

    Portal *ps = malloc(sizeof(Portal));

    ps[0].id = 1;
    ps[0].x = 3;
    ps[0].y = 2;
    ps[0].target_room_id = 55;
    ps[0].name = NULL;

    ck_assert_int_eq(room_set_portals(r, ps, 1), OK);

    int sx = -1, sy = -1;
    ck_assert_int_eq(room_get_start_position(r, &sx, &sy), OK);

    ck_assert_int_eq(sx, 3);
    ck_assert_int_eq(sy, 2);

    ck_assert_int_eq(room_get_start_position(NULL, &sx, &sy), INVALID_ARGUMENT);
    ck_assert_int_eq(room_get_start_position(r, NULL, &sy), INVALID_ARGUMENT);
}
END_TEST



/* ============================================================
 * Suite builder
 * ============================================================ */
Suite *room_suite(void)
{
    Suite *s = suite_create("Room");

    TCase *tc = tcase_create("Basics");
    tcase_add_checked_fixture(tc, setup_room, teardown_room);

    tcase_add_test(tc, test_room_create_basic);
    tcase_add_test(tc, test_room_get_width);
    tcase_add_test(tc, test_room_get_height);
    tcase_add_test(tc, test_room_set_floor_grid_transfers_ownership);
    tcase_add_test(tc, test_room_set_floor_grid_null_room);
    tcase_add_test(tc, test_room_set_portals);
    tcase_add_test(tc, test_room_set_treasures);
    tcase_add_test(tc, test_room_place_treasure);
    tcase_add_test(tc, test_room_get_treasure_at);
    tcase_add_test(tc, test_room_get_portal_destination);
    tcase_add_test(tc, test_room_is_walkable);
    tcase_add_test(tc, test_room_classify_tile);
    tcase_add_test(tc, test_room_render);
    tcase_add_test(tc, test_room_get_start_position);

    suite_add_tcase(s, tc);
    return s;
}

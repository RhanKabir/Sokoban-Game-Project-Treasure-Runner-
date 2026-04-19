#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "game_engine.h"
#include "player.h"

/* ============================================================
 * GameEngine fixtures
 * ============================================================ */

static GameEngine *eng = NULL;

static void setup_engine(void)
{
    Status s = game_engine_create("../assets/starter.ini", &eng);

    ck_assert_int_eq(s, OK);
    ck_assert_ptr_nonnull(eng);
}

static void teardown_engine(void)
{
    game_engine_destroy(eng);
    eng = NULL;
}

/* ============================================================
 * Tests
 * ============================================================ */

START_TEST(test_engine_create)
{
    ck_assert_ptr_nonnull(eng);
}
END_TEST

START_TEST(test_game_engine_get_player)
{
    const Player *p = game_engine_get_player(eng);
    ck_assert_ptr_nonnull(p);
    ck_assert_ptr_eq(game_engine_get_player(NULL), NULL);
}
END_TEST

START_TEST(test_game_engine_move_player)
{
    int x_before, y_before;
    int x_after, y_after;

    ck_assert_int_eq(player_get_position(eng->player, &x_before, &y_before), OK);

    Status s = game_engine_move_player(eng, DIR_NORTH);

    ck_assert(s == OK || s == ROOM_IMPASSABLE);

    if (s == OK) {
        ck_assert_int_eq(player_get_position(eng->player, &x_after, &y_after), OK);
        ck_assert(y_after == y_before - 1);
    }
}
END_TEST

START_TEST(test_game_engine_get_room_count)
{
    int count = -1;
    ck_assert_int_eq(game_engine_get_room_count(eng, &count), OK);
    ck_assert(count > 0);

    ck_assert_int_eq(game_engine_get_room_count(NULL, &count), INVALID_ARGUMENT);
    ck_assert_int_eq(game_engine_get_room_count(eng, NULL), NULL_POINTER);
}
END_TEST

START_TEST(test_game_engine_get_room_dimensions)
{
    int w = -1, h = -1;

    ck_assert_int_eq(game_engine_get_room_dimensions(eng, &w, &h), OK);
    ck_assert(w > 0);
    ck_assert(h > 0);

    ck_assert_int_eq(game_engine_get_room_dimensions(NULL, &w, &h), INVALID_ARGUMENT);
    ck_assert_int_eq(game_engine_get_room_dimensions(eng, NULL, &h), NULL_POINTER);
    ck_assert_int_eq(game_engine_get_room_dimensions(eng, &w, NULL), NULL_POINTER);
}
END_TEST

START_TEST(test_game_engine_reset)
{
    ck_assert_int_eq(game_engine_reset(eng), OK);
    ck_assert_int_eq(game_engine_reset(NULL), INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_game_engine_render_current_room)
{
    char *s;

    ck_assert_int_eq(game_engine_render_current_room(eng, &s), OK);
    ck_assert_int_eq(game_engine_render_current_room(NULL, &s), INVALID_ARGUMENT);
    ck_assert_int_eq(game_engine_render_current_room(eng, NULL), NULL_POINTER);
}
END_TEST

START_TEST(test_game_engine_render_room)
{
    int *ids = NULL;
    int count = 0;

    ck_assert_int_eq(game_engine_get_room_ids(eng, &ids, &count), OK);
    ck_assert_ptr_nonnull(ids);
    ck_assert(count > 0);

    char *s = NULL;
    ck_assert_int_eq(game_engine_render_room(eng, ids[0], &s), OK);
    ck_assert_ptr_nonnull(s);

    s = NULL;
    ck_assert_int_eq(game_engine_render_room(NULL, 0, &s), INVALID_ARGUMENT);
    ck_assert_int_eq(game_engine_render_room(eng, 0, NULL), NULL_POINTER);
    ck_assert_int_eq(game_engine_render_room(eng, -1, &s), GE_NO_SUCH_ROOM);
}
END_TEST

START_TEST(test_game_engine_get_room_ids)
{
    int *ids = NULL;
    int count = 0;

    ck_assert_int_eq(game_engine_get_room_ids(eng, &ids, &count), OK);
    ck_assert_ptr_nonnull(ids);
    ck_assert(count > 0);

    for (int i = 0; i < count; i++) {
        ck_assert(ids[i] >= 0);
    }

    ck_assert_int_eq(game_engine_get_room_ids(NULL, &ids, &count), INVALID_ARGUMENT);
    ck_assert_int_eq(game_engine_get_room_ids(eng, NULL, &count), NULL_POINTER);
    ck_assert_int_eq(game_engine_get_room_ids(eng, &ids, NULL), NULL_POINTER);
}
END_TEST


/* ============================================================
 * Suite builder
 * ============================================================ */

Suite *game_engine_suite(void)
{
    Suite *s = suite_create("GameEngine");

    TCase *tc = tcase_create("Core");
    tcase_add_checked_fixture(tc, setup_engine, teardown_engine);

    tcase_add_test(tc, test_engine_create);
    tcase_add_test(tc, test_game_engine_get_player);
    tcase_add_test(tc, test_game_engine_move_player);
    tcase_add_test(tc, test_game_engine_get_room_count);
    tcase_add_test(tc, test_game_engine_get_room_dimensions);
    tcase_add_test(tc, test_game_engine_reset);
    tcase_add_test(tc, test_game_engine_render_current_room);
    tcase_add_test(tc, test_game_engine_render_room);
    tcase_add_test(tc, test_game_engine_get_room_ids);

    suite_add_tcase(s, tc);
    return s;
}

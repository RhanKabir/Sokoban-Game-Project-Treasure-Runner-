#include <check.h>
#include <stdlib.h>

Suite *room_suite(void);
Suite *game_engine_suite(void);

//more suites

int main(void)
{
    Suite *suites[] = {room_suite(),
                        game_engine_suite(),
                        NULL
        //more suites
    };

    SRunner *runner = srunner_create(suites[0]);
    for (int i = 1; suites[i] != NULL; ++i) {
        srunner_add_suite(runner, suites[i]);
    }

    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

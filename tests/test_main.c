#include <stdio.h>

extern int test_nes_state(void);
extern int test_rng_and_piece_selection(void);
extern int test_bcd_and_scoring(void);
extern int test_tetrimino_subsystem(void);
extern int test_playfield_subsystem(void);
extern int test_input_subsystem(void);
extern int test_state_subsystem(void);
extern int test_menu_subsystem(void);
extern int test_highscore_subsystem(void);
extern int test_render_subsystem(void);
extern void run_ending_subsystem_tests(void);
extern int test_differential_verification(void);

int main(void) {
    int failures = 0;
    printf("=== Running TDecomp Verification Test Suite ===\n");

    failures += test_nes_state();
    failures += test_rng_and_piece_selection();
    failures += test_bcd_and_scoring();
    failures += test_tetrimino_subsystem();
    failures += test_playfield_subsystem();
    failures += test_input_subsystem();
    failures += test_state_subsystem();
    failures += test_menu_subsystem();
    failures += test_highscore_subsystem();
    failures += test_render_subsystem();
    run_ending_subsystem_tests();

    printf("\n=== Running Reference ROM & 6502 Differential Verification ===\n");
    failures += test_differential_verification();

    if (failures == 0) {
        printf("\n=== ALL TESTS AND DIFFERENTIAL VERIFICATIONS PASSED ===\n");
    } else {
        printf("\n=== %d TEST(S) FAILED ===\n", failures);
    }

    return failures;
}

#include <stdio.h>

extern int test_nes_state(void);
extern int test_rng_and_piece_selection(void);

int main(void) {
    int failures = 0;
    printf("=== Running TDecomp Verification Test Suite ===\n");

    failures += test_nes_state();
    failures += test_rng_and_piece_selection();

    if (failures == 0) {
        printf("=== ALL TESTS PASSED SUCCESSFULLY ===\n");
    } else {
        printf("=== %d TEST(S) FAILED ===\n", failures);
    }

    return failures;
}

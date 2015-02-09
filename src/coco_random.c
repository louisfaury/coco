#include <math.h>

#include "coco.h"

#define NUMBBO_NORMAL_POLAR /* Use polar transformation method */

#define SHORT_LAG 273
#define LONG_LAG 607

struct coco_random_state {
  double x[LONG_LAG];
  size_t index;
};

/**
 * coco_random_generate(state):
 *
 * This is a lagged Fibonacci generator that is nice because it is
 * reasonably small and directly generates double values. The chosen
 * lags (607 and 273) lead to a generator with a period in excess of
 * 2^607-1.
 */
static void coco_random_generate(coco_random_state_t *state) {
  size_t i;
  for (i = 0; i < SHORT_LAG; ++i) {
    double t = state->x[i] + state->x[i + (LONG_LAG - SHORT_LAG)];
    if (t >= 1.0)
      t -= 1.0;
    state->x[i] = t;
  }
  for (i = SHORT_LAG; i < LONG_LAG; ++i) {
    double t = state->x[i] + state->x[i - SHORT_LAG];
    if (t >= 1.0)
      t -= 1.0;
    state->x[i] = t;
  }
  state->index = 0;
}

coco_random_state_t *coco_new_random(uint32_t seed) {
  coco_random_state_t *state =
      (coco_random_state_t *)coco_allocate_memory(sizeof(coco_random_state_t));
  size_t i;
  /* Expand seed to fill initial state array. */
  for (i = 0; i < LONG_LAG; ++i) {
    state->x[i] = ((double)seed) / (double)((1ULL << 32) - 1);
    /* Advance seed based on simple RNG from TAOCP */
    seed = 1812433253UL * (seed ^ (seed >> 30)) + (i + 1);
  }
  state->index = 0;
  return state;
}

void coco_free_random(coco_random_state_t *state) { coco_free_memory(state); }

double coco_uniform_random(coco_random_state_t *state) {
  /* If we have consumed all random numbers in our archive, it is
   * time to run the actual generator for one iteration to refill
   * the state with 'LONG_LAG' new values.
   */
  if (state->index >= LONG_LAG)
    coco_random_generate(state);
  return state->x[state->index++];
}

double coco_normal_random(coco_random_state_t *state) {
  double normal;
#ifdef NUMBBO_NORMAL_POLAR
  const double u1 = coco_uniform_random(state);
  const double u2 = coco_uniform_random(state);
  normal = sqrt(-2 * log(u1)) * cos(2 * coco_pi * u2);
#else
  int i;
  normal = 0.0;
  for (i = 0; i < 12; ++i) {
    normal += coco_uniform_random(state);
  }
  normal -= 6.0;
#endif
  return normal;
}

/* Be hygenic (for amalgamation) and undef lags. */
#undef SHORT_LAG
#undef LONG_LAG

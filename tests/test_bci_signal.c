// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// ISO/IEC 25000 | ISO/IEC/IEEE 15288:2023

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "eai/common.h"
#include "eai_bci/eai_bci.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;
#define TEST(name) do { tests_run++; printf("  TEST %-40s ", #name); } while(0)
#define PASS() do { tests_passed++; printf("[PASS]\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("[FAIL] %s\n", msg); } while(0)

static void test_signal_init(void)
{
    TEST(signal_init);
    eai_bci_signal_t sig;
    eai_status_t st = eai_bci_signal_init(&sig, 4, 250);
    if (st != EAI_OK) { FAIL("init failed"); return; }
    if (sig.num_channels != 4) { FAIL("wrong channel count"); return; }
    if (sig.sample_rate_hz != 250) { FAIL("wrong sample rate"); return; }
    if (eai_bci_signal_count(&sig) != 0) { FAIL("should be empty"); return; }
    PASS();
}

static void test_signal_push_and_count(void)
{
    TEST(signal_push_and_count);
    eai_bci_signal_t sig;
    eai_bci_signal_init(&sig, 2, 250);

    eai_bci_sample_t sample = {0};
    sample.channels[0] = 1.0f;
    sample.channels[1] = 2.0f;
    sample.timestamp_us = 1000;

    for (int i = 0; i < 10; i++) {
        eai_bci_signal_push(&sig, &sample);
    }

    if (eai_bci_signal_count(&sig) != 10) { FAIL("expected count 10"); return; }
    PASS();
}

static void test_signal_ring_wrap(void)
{
    TEST(signal_ring_wrap);
    eai_bci_signal_t sig;
    eai_bci_signal_init(&sig, 1, 250);

    eai_bci_sample_t sample = {0};
    for (int i = 0; i < EAI_BCI_RING_SIZE + 50; i++) {
        sample.channels[0] = (float)i;
        eai_bci_signal_push(&sig, &sample);
    }

    if (eai_bci_signal_count(&sig) != EAI_BCI_RING_SIZE) {
        FAIL("count should be capped at ring size");
        return;
    }

    eai_bci_sample_t window[1];
    int got = eai_bci_signal_get_window(&sig, window, 1);
    if (got != 1) { FAIL("expected 1 sample in window"); return; }
    if (window[0].channels[0] != (float)(EAI_BCI_RING_SIZE + 49)) {
        FAIL("latest sample mismatch");
        return;
    }
    PASS();
}

static void test_signal_get_window(void)
{
    TEST(signal_get_window);
    eai_bci_signal_t sig;
    eai_bci_signal_init(&sig, 1, 250);

    eai_bci_sample_t sample = {0};
    for (int i = 0; i < 20; i++) {
        sample.channels[0] = (float)i;
        eai_bci_signal_push(&sig, &sample);
    }

    eai_bci_sample_t window[5];
    int got = eai_bci_signal_get_window(&sig, window, 5);
    if (got != 5) { FAIL("expected 5 samples"); return; }
    if (window[0].channels[0] != 15.0f) { FAIL("window start wrong"); return; }
    if (window[4].channels[0] != 19.0f) { FAIL("window end wrong"); return; }
    PASS();
}

static void test_band_power(void)
{
    TEST(band_power);
    eai_bci_signal_t sig;
    eai_bci_signal_init(&sig, 1, 250);

    eai_bci_sample_t sample = {0};
    for (int i = 0; i < 64; i++) {
        sample.channels[0] = 3.0f;
        eai_bci_signal_push(&sig, &sample);
    }

    float power = eai_bci_band_power(&sig, 0, 64);
    if (fabsf(power - 9.0f) > 0.01f) { FAIL("expected power ~9.0"); return; }
    PASS();
}

static void test_preprocess_init(void)
{
    TEST(preprocess_init);
    eai_bci_preprocessor_t pp;
    eai_status_t st = eai_bci_preprocess_init(&pp, 4, 250.0f, 1.0f, 50.0f, 50.0f);
    if (st != EAI_OK) { FAIL("init failed"); return; }
    if (!pp.initialized) { FAIL("not marked initialized"); return; }
    if (pp.num_channels != 4) { FAIL("wrong channel count"); return; }
    PASS();
}

/* |H(e^jw)| of a biquad at freq_hz, from its stored coefficients. */
static double biquad_gain(const eai_bci_iir_filter_t *f, double freq_hz, double fs_hz)
{
    double w  = 2.0 * 3.14159265358979323846 * freq_hz / fs_hz;
    double c1 = cos(w), s1 = sin(w), c2 = cos(2.0 * w), s2 = sin(2.0 * w);
    double nr = f->b[0] + f->b[1] * c1 + f->b[2] * c2;
    double ni = -(f->b[1] * s1 + f->b[2] * s2);
    double dr = f->a[0] + f->a[1] * c1 + f->a[2] * c2;
    double di = -(f->a[1] * s1 + f->a[2] * s2);
    return sqrt((nr * nr + ni * ni) / (dr * dr + di * di));
}

static void test_bandpass_edges(void)
{
    /* The band-pass must be about -3 dB (gain 0.707) at low_hz and high_hz
     * and unity at the centre sqrt(low_hz * high_hz). */
    static const float bands[][3] = {
        /* sample rate, low, high */
        { 250.0f,  1.0f, 50.0f },   /* the eai_bci_config defaults */
        { 250.0f,  8.0f, 30.0f },   /* mu + beta */
        { 1000.0f, 8.0f, 30.0f },
    };
    int bad = 0;
    TEST(bandpass_edges);
    for (size_t i = 0; i < sizeof(bands) / sizeof(bands[0]); i++) {
        float fs = bands[i][0], lo = bands[i][1], hi = bands[i][2];
        eai_bci_preprocessor_t pp;
        if (eai_bci_preprocess_init(&pp, 1, fs, lo, hi, 50.0f) != EAI_OK) {
            FAIL("init failed");
            return;
        }
        double g_lo = biquad_gain(&pp.bandpass, lo, fs);
        double g_hi = biquad_gain(&pp.bandpass, hi, fs);
        double g_c  = biquad_gain(&pp.bandpass, sqrt((double)lo * hi), fs);
        int ok = fabs(g_lo - 0.7071) <= 0.02 && fabs(g_hi - 0.7071) <= 0.02 && fabs(g_c - 1.0) <= 0.01;
        printf("\n    fs=%g band %g-%g Hz: gain %.3f at %g Hz, %.3f at %g Hz, %.3f at centre%s",
               fs, lo, hi, g_lo, lo, g_hi, hi, g_c, ok ? "" : "  <- want 0.707, 0.707, 1.000");
        if (!ok) bad++;
    }
    printf("\n  %-46s", "");
    if (bad) { FAIL("band-pass edges are not at low_hz and high_hz"); return; }
    PASS();
}

static void test_preprocess_sample(void)
{
    TEST(preprocess_sample);
    eai_bci_preprocessor_t pp;
    eai_bci_preprocess_init(&pp, 2, 250.0f, 1.0f, 50.0f, 50.0f);

    eai_bci_sample_t sample = {0};
    sample.channels[0] = 100.0f;
    sample.channels[1] = 200.0f;

    eai_status_t st = eai_bci_preprocess_sample(&pp, &sample);
    if (st != EAI_OK) { FAIL("preprocess failed"); return; }
    /* After first sample through filter, output should be finite */
    if (isnan(sample.channels[0]) || isinf(sample.channels[0])) {
        FAIL("output is NaN/Inf");
        return;
    }
    PASS();
}

static void test_signal_clear(void)
{
    TEST(signal_clear);
    eai_bci_signal_t sig;
    eai_bci_signal_init(&sig, 1, 250);

    eai_bci_sample_t sample = {0};
    for (int i = 0; i < 10; i++) eai_bci_signal_push(&sig, &sample);

    eai_bci_signal_clear(&sig);
    if (eai_bci_signal_count(&sig) != 0) { FAIL("should be empty after clear"); return; }
    PASS();
}

int main(void)
{
    printf("=== EAI BCI Signal Tests ===\n");

    test_signal_init();
    test_signal_push_and_count();
    test_signal_ring_wrap();
    test_signal_get_window();
    test_band_power();
    test_preprocess_init();
    test_bandpass_edges();
    test_preprocess_sample();
    test_signal_clear();

    printf("\nResults: %d/%d passed, %d failed\n", tests_passed, tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}

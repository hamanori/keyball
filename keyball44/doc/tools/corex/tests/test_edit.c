#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "corex_edit.h"

static uint16_t emitted[32];
static size_t emitted_count;

void corex_edit_emit(uint16_t chord) {
    assert(emitted_count < sizeof(emitted) / sizeof(emitted[0]));
    emitted[emitted_count++] = chord;
}

static void reset(void) {
    corex_edit_cancel();
    emitted_count = 0;
}

static void expect(const uint16_t *expected, size_t count) {
    assert(emitted_count == count);
    for (size_t index = 0; index < count; index++) {
        if (emitted[index] != expected[index]) {
            fprintf(stderr, "chord %zu: expected 0x%04X, got 0x%04X\n",
                    index, expected[index], emitted[index]);
        }
        assert(emitted[index] == expected[index]);
    }
}

static void test_clipboard_history_uses_mac_chord(void) {
    reset();
    assert(corex_edit_process(24, true, true, false, 0));
    assert(emitted_count == 1);
    assert(emitted[0] == 0x0A19); /* GUI+SHIFT+V */
}

static void test_clipboard_history_uses_windows_chord_on_press_only(void) {
    reset();
    assert(corex_edit_process(24, true, false, false, 10));
    assert(corex_edit_process(24, false, false, false, 11));
    assert(emitted_count == 1);
    assert(emitted[0] == 0x0819); /* GUI+V */
}

static void test_fixed_action_sequences_match_both_hosts(void) {
    static const uint16_t replace_mac[] = {0x0C09};
    static const uint16_t replace_windows[] = {0x010B};
    static const uint16_t cut_all_mac[] = {0x0804, 0x081B};
    static const uint16_t cut_all_windows[] = {0x0104, 0x011B};
    static const uint16_t copy_all_mac[] = {0x0804, 0x0806};
    static const uint16_t copy_all_windows[] = {0x0104, 0x0106};
    static const uint16_t cut_line_mac[] = {0x0850, 0x0A4F, 0x081B};
    static const uint16_t cut_line_windows[] = {0x004A, 0x024D, 0x011B};
    static const uint16_t copy_line_mac[] = {0x0850, 0x0A4F, 0x0806};
    static const uint16_t copy_line_windows[] = {0x004A, 0x024D, 0x0106};
    static const uint16_t paragraph_mac[] = {0x0451, 0x0652};
    static const uint16_t paragraph_windows[] = {0x0151, 0x0352};
    static const uint16_t line_mac[] = {0x0850, 0x0A4F};
    static const uint16_t line_windows[] = {0x004A, 0x024D};
    static const uint16_t word_mac[] = {0x044F, 0x0650};
    static const uint16_t word_windows[] = {0x014F, 0x0350};
    static const uint16_t tabs_mac[] = {0x081E};
    static const uint16_t first_tab_windows[] = {0x011E};
    static const uint16_t last_tab_mac[] = {0x0826};
    static const uint16_t tabs_windows[] = {0x0126};

    reset();
    assert(corex_edit_process(25, true, true, false, 1));
    expect(replace_mac, 1);
    reset();
    assert(corex_edit_process(25, true, false, false, 1));
    expect(replace_windows, 1);

    reset();
    assert(corex_edit_process(52, true, true, false, 1));
    expect(cut_all_mac, 2);
    reset();
    assert(corex_edit_process(52, true, false, true, 1)); /* Shift is boundary-only. */
    expect(cut_all_windows, 2);
    reset();
    assert(corex_edit_process(53, true, true, false, 1));
    expect(copy_all_mac, 2);
    reset();
    assert(corex_edit_process(53, true, false, false, 1));
    expect(copy_all_windows, 2);

    reset();
    assert(corex_edit_process(54, true, true, false, 1));
    expect(cut_line_mac, 3);
    reset();
    assert(corex_edit_process(54, true, false, false, 1));
    expect(cut_line_windows, 3);
    reset();
    assert(corex_edit_process(55, true, true, false, 1));
    expect(copy_line_mac, 3);
    reset();
    assert(corex_edit_process(55, true, false, false, 1));
    expect(copy_line_windows, 3);

    reset();
    assert(corex_edit_process(56, true, true, false, 1));
    expect(paragraph_mac, 2);
    reset();
    assert(corex_edit_process(56, true, false, false, 1));
    expect(paragraph_windows, 2);
    reset();
    assert(corex_edit_process(57, true, true, false, 1));
    expect(line_mac, 2);
    reset();
    assert(corex_edit_process(57, true, false, false, 1));
    expect(line_windows, 2);
    reset();
    assert(corex_edit_process(58, true, true, false, 1));
    expect(word_mac, 2);
    reset();
    assert(corex_edit_process(58, true, false, false, 1));
    expect(word_windows, 2);

    reset();
    assert(corex_edit_process(62, true, true, false, 1));
    expect(tabs_mac, 1);
    reset();
    assert(corex_edit_process(62, true, false, false, 1));
    expect(first_tab_windows, 1);
    reset();
    assert(corex_edit_process(63, true, true, false, 1));
    expect(last_tab_mac, 1);
    reset();
    assert(corex_edit_process(63, true, false, false, 1));
    expect(tabs_windows, 1);
}

static void test_line_boundary_double_tap_preserves_shift(void) {
    static const uint16_t mac_head[] = {0x0A50}; /* SHIFT+GUI+LEFT */
    static const uint16_t mac_paragraph[] = {0x0A50, 0x0652}; /* head, then SHIFT+ALT+UP */
    static const uint16_t windows_end[] = {0x024D}; /* SHIFT+END */
    static const uint16_t windows_paragraph[] = {0x024D, 0x0351}; /* end, then SHIFT+CTRL+DOWN */

    reset();
    assert(corex_edit_process(59, true, true, true, 100));
    assert(corex_edit_process(59, false, true, true, 110));
    expect(mac_head, 1);
    assert(corex_edit_process(59, true, true, true, 250));
    expect(mac_paragraph, 2);

    reset();
    assert(corex_edit_process(60, true, false, true, 300));
    assert(corex_edit_process(60, false, false, true, 310));
    expect(windows_end, 1);
    assert(corex_edit_process(60, true, false, true, 450));
    expect(windows_paragraph, 2);
}

static void test_paste_waits_for_release_then_timeout(void) {
    static const uint16_t plain_mac_paste[] = {0x0819}; /* GUI+V */

    reset();
    assert(corex_edit_process(61, true, true, false, 100));
    corex_edit_tick(true, 1000); /* A held key has no timeout. */
    assert(emitted_count == 0);
    assert(corex_edit_process(61, false, true, false, 1000));
    corex_edit_tick(true, 1179);
    assert(emitted_count == 0);
    corex_edit_tick(true, 1180);
    expect(plain_mac_paste, 1);

    reset();
    assert(corex_edit_process(61, true, true, false, 65490));
    assert(corex_edit_process(61, false, true, false, 65520));
    corex_edit_tick(true, 163); /* 179 ms after release, across uint16 wrap. */
    assert(emitted_count == 0);
    corex_edit_tick(true, 164);
    expect(plain_mac_paste, 1);
}

static void test_paste_double_tap_emits_one_formatted_paste(void) {
    static const uint16_t formatted_mac[] = {0x0E19}; /* ALT+SHIFT+GUI+V */
    static const uint16_t formatted_windows[] = {0x0319}; /* CTRL+SHIFT+V */

    reset();
    assert(corex_edit_process(61, true, true, false, 10));
    assert(corex_edit_process(61, true, true, false, 11)); /* held-key repeat */
    assert(corex_edit_process(61, false, true, false, 30));
    assert(corex_edit_process(61, true, true, false, 120));
    assert(corex_edit_process(61, true, true, false, 121)); /* held-key repeat */
    assert(corex_edit_process(61, false, true, false, 130));
    corex_edit_tick(true, 400);
    expect(formatted_mac, 1);

    reset();
    assert(corex_edit_process(61, true, false, false, 500));
    assert(corex_edit_process(61, false, false, false, 510));
    assert(corex_edit_process(61, true, false, false, 600));
    assert(corex_edit_process(61, false, false, false, 610));
    expect(formatted_windows, 1);
}

static void test_paste_host_change_cancels_pending_action(void) {
    reset();
    assert(corex_edit_process(61, true, true, false, 10));
    assert(corex_edit_process(61, false, true, false, 20));
    corex_edit_tick(false, 30); /* Host switched before the delay expired. */
    corex_edit_tick(false, 1000);
    assert(emitted_count == 0);
}

static void test_interrupt_flushes_paste_and_clears_boundary_pair(void) {
    static const uint16_t plain_mac_paste[] = {0x0819};
    static const uint16_t first_line_head[] = {0x0850, 0x0850};
    static const uint16_t flush_before_action[] = {0x0819, 0x0C09};

    reset();
    assert(corex_edit_process(61, true, true, false, 10));
    assert(corex_edit_process(61, false, true, false, 20));
    assert(!corex_edit_process(0, true, true, false, 21));
    expect(plain_mac_paste, 1);

    reset();
    assert(corex_edit_process(59, true, true, false, 30));
    assert(corex_edit_process(59, false, true, false, 31));
    assert(!corex_edit_process(0, true, true, false, 32));
    assert(corex_edit_process(59, true, true, false, 33));
    expect(first_line_head, 2); /* Both presses are row-boundary actions. */

    reset();
    assert(corex_edit_process(61, true, true, false, 40));
    assert(corex_edit_process(61, false, true, false, 41));
    assert(corex_edit_process(25, true, true, false, 42));
    expect(flush_before_action, 2);
}

static void test_boundary_pair_expires_at_deadline(void) {
    static const uint16_t first_then_first_again[] = {0x0850, 0x0850};

    reset();
    assert(corex_edit_process(59, true, true, false, 10));
    assert(corex_edit_process(59, false, true, false, 20));
    corex_edit_tick(true, 200);
    assert(corex_edit_process(59, true, true, false, 201));
    expect(first_then_first_again, 2);
}

static void test_cancel_resets_pending_decisions(void) {
    static const uint16_t restarted_line_head[] = {0x0850, 0x0850};

    reset();
    assert(corex_edit_process(59, true, true, false, 10));
    assert(corex_edit_process(59, false, true, false, 11));
    corex_edit_cancel();
    assert(corex_edit_process(59, true, true, false, 12));
    expect(restarted_line_head, 2);

    reset();
    assert(corex_edit_process(61, true, true, false, 20));
    assert(corex_edit_process(61, false, true, false, 21));
    corex_edit_cancel();
    corex_edit_tick(true, 1000);
    assert(emitted_count == 0);
}

int main(void) {
    test_clipboard_history_uses_mac_chord();
    test_clipboard_history_uses_windows_chord_on_press_only();
    test_fixed_action_sequences_match_both_hosts();
    test_line_boundary_double_tap_preserves_shift();
    test_paste_waits_for_release_then_timeout();
    test_paste_double_tap_emits_one_formatted_paste();
    test_paste_host_change_cancels_pending_action();
    test_interrupt_flushes_paste_and_clears_boundary_pair();
    test_boundary_pair_expires_at_deadline();
    test_cancel_resets_pending_decisions();
    puts("10 edit behaviors passed");
    return 0;
}

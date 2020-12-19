#include <cstdio>
#include "cppmidi.h"

/* This examples program demonstrates how to write notes to a MIDI file.
 * In this particular example, a C Major chord will be written. */

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    using namespace cppmidi;

    /* create midi file object in memory */
    midi_file mf;

    /* set time division of MIDI file to 24 ticks per beat */
    mf.time_division = 24;

    /* create a new track */
    mf.midi_tracks.emplace_back();

    /* Each track contains a vector of midi_event called "midi_events".
     * We can simply attach a few notes by appending events to this vector */

    /* Create a Note On event:
     * - at time (in ticks) #0
     * - on channel #0 (cppmidi indexes MIDI channels from 0 to 15)
     * - with MIDI key #60 (C3)
     * - with note velocity #127 */
    mf[0].midi_events.emplace_back(new noteon_message_midi_event(0, 0, 60, 127));

    /* Create a Note On event:
     * - at time (in ticks) #4
     * - on channel #0 (cppmidi indexes MIDI channels from 0 to 15)
     * - with MIDI key #64 (E3)
     * - with note velocity #127 */
    mf[0].midi_events.emplace_back(new noteon_message_midi_event(4, 0, 64, 127));

    /* Create a Note On event:
     * - at time (in ticks) #8
     * - on channel #0 (cppmidi indexes MIDI channels from 0 to 15)
     * - with MIDI key #64 (G3)
     * - with note velocity #127 */
    mf[0].midi_events.emplace_back(new noteon_message_midi_event(8, 0, 64, 127));

    /* Create a Note Off event:
     * - at time (in ticks) #24
     * - on channel #0 (cppmidi indexes MIDI channels from 0 to 15)
     * - with MIDI key #60 (C3)
     * - with note velocity #127 */
    mf[0].midi_events.emplace_back(new noteoff_message_midi_event(24, 0, 60, 127));

    /* Create a Note Off event:
     * - at time (in ticks) #24
     * - on channel #0 (cppmidi indexes MIDI channels from 0 to 15)
     * - with MIDI key #64 (E3)
     * - with note velocity #127 */
    mf[0].midi_events.emplace_back(new noteoff_message_midi_event(24, 0, 64, 127));

    /* Create a Note Off event:
     * - at time (in ticks) #24
     * - on channel #0 (cppmidi indexes MIDI channels from 0 to 15)
     * - with MIDI key #67 (G3)
     * - with note velocity #127 */
    mf[0].midi_events.emplace_back(new noteoff_message_midi_event(24, 0, 67, 127));

    /* Save the file to disk with name "write_notes.mid" */
    mf.save_to_file("write_notes.mid");
}

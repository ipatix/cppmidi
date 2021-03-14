#include <iostream>
#include <algorithm>

#include "cppmidi.h"

/* This example program demonstrates how to remove all
 * sysex events from a MIDI file */

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ./delete_sysex <input.mid> <output.mid>" << std::endl;
        return 1;
    }

    try {
        /* Create midi_file object which represents a parsed file in memory. */
        cppmidi::midi_file mf;

        /* Initialize midi_file object with the contents of a standard midi file. */
        mf.load_from_file(argv[1]);

        /* iterate over all tracks */
        for (cppmidi::midi_track &mtrk : mf) {
            /* delete all events that are a class or subclass of cppmidi::sysex_event */
            mtrk.midi_events.erase(
                std::remove_if(
                    mtrk.midi_events.begin(),
                    mtrk.midi_events.end(),
                    [](const auto &ev) { return dynamic_cast<cppmidi::sysex_midi_event *>(ev.get()) != nullptr; }
                ),
                mtrk.midi_events.end()
            );
        }

        /* save midi file after we're done */
        mf.save_to_file(argv[2]);
    } catch (const cppmidi::xcept& ex) {
        /* If an error occurs, cppmidi will throw an exception of type xcept */
        std::cerr << "cppmidi lib error:" << std::endl << ex.what() << std::endl;
    }
}

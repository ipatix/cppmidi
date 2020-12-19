#include <iostream>

#include "cppmidi.h"

/* This example program demonstrates how to load a MIDI file from disk
 * and how to store it back to disk */

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ./midi_read_write <input.mid> <output.mid>" << std::endl;
        return 1;
    }

    try {
        /* Create midi_file object which represents a parsed file in memory. */
        cppmidi::midi_file mf;

        /* Initialize midi_file object with the contents of a standard midi file. */
        mf.load_from_file(argv[1]);

        /* You can now edit the midi_file object in memory.
         * In this example we'll just save the midi_file back to disk.
         * Because the midi_file format has a few redundant ways of being stored,
         * the resulting file may not be a byte exact copy, but should be functionally
         * identical. */
        mf.save_to_file(argv[2]);

    } catch (const cppmidi::xcept& ex) {
        /* If an error occurs, cppmidi will throw an exception of type xcept */
        std::cerr << "cppmidi lib error:" << std::endl << ex.what() << std::endl;
    }
}

#include <iostream>

#include "cppmidi.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./to_text <input.mid>" << std::endl;
        return 1;
    }

    try {
        /* Create midi_file object which represents a parsed file in memory. */
        cppmidi::midi_file mf;

        /* Initialize midi_file object with the contents of a standard midi file. */
        mf.load_from_file(argv[1]);

        /* print file to cout */
        std::cout << mf;
    } catch (const cppmidi::xcept& ex) {
        /* If an error occurs, cppmidi will throw an exception of type xcept */
        std::cerr << "cppmidi lib error:" << std::endl << ex.what() << std::endl;
    }
}

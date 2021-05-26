#include <vector>
#include <string>
#include <iostream>

#include "cppmidi.h"

/* This example program demonstrates how to apply an
 * instrument mapping via command line */

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: ./map_instruments <input.mid> <output.mid> [from_instr:to_instr]..." << std::endl;
        return 1;
    }

    // static to allow visitor to access the map
    static std::vector<int> inst_map(128, -1);

    for (int i = 3; i < argc; i++) {
        const std::string v(argv[i]);
        const auto seperator_pos = v.find(":");
        if (seperator_pos == std::string::npos || seperator_pos == 0 || seperator_pos == v.size() - 1) {
            std::cerr << "Warning! Ignored malformed argument: " << v << std::endl;
            continue;
        }

        int from = std::stoi(v.substr(0, seperator_pos));
        int to = std::stoi(v.substr(seperator_pos + 1));

        inst_map.at(from) = to;
    }

    try {
        /* Create midi_file object which represents a parsed file in memory. */
        cppmidi::midi_file mf;

        /* Initialize midi_file object with the contents of a standard midi file. */
        mf.load_from_file(argv[1]);

        /* create visitor that modifies the instrument according to our map */
        class instrument_mapper_visitor : public cppmidi::visitor {
        public:
            void visit(cppmidi::program_message_midi_event& ev) override {
                int mapping = inst_map.at(ev.get_program());
                if (mapping == -1)
                    return;
                ev.set_program(static_cast<uint8_t>(mapping));
            }
        } mapper;

        /* Execute the visitor on the MIDI file */
        mapper.visitor::visit(mf);

        /* save midi file after we're done */
        mf.save_to_file(argv[2]);
    } catch (const cppmidi::xcept& ex) {
        /* If an error occurs, cppmidi will throw an exception of type xcept */
        std::cerr << "cppmidi lib error:" << std::endl << ex.what() << std::endl;
    }
}

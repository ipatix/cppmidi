#include <iostream>
#include <string>
#include <algorithm>
#include <cmath>

#include "cppmidi.h"

/* This example program demonstrates the usage of a visitor to
 * iterate over all midi events. In this example, we will scale
 * all note velocities by the specified parameter. */

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: ./midi_read_write <velocity-scale> <input.mid> <output.mid>" << std::endl;
        return 1;
    }

    static float scale = std::stof(argv[1]);
    const char *input = argv[2];
    const char *output = argv[3];

    /* Create midi_file object which represents a parsed file in memory. */
    cppmidi::midi_file mf;

    /* Parse MIDI file. */
    mf.load_from_file(input);

    /* A visitor can be used to iterate over all events and execute 
     * a "handler" for a desired event type. In this example
     * we implement a visit() method for a noteon_message_midi_event
     * which will be called for every MIDI event of that type. */
    class velocity_scale_visitor : public cppmidi::visitor {
    public:
        void visit(cppmidi::noteon_message_midi_event& ev) override {
            /* This visit() method scales the note velocty by the amount
             * we specified on the command line */
            int scaled_vel = static_cast<int>(std::round(ev.get_velocity() * scale));
            ev.set_velocity(static_cast<uint8_t>(std::clamp(scaled_vel, 0, 127)));
        }
    } scaler;

    /* Execute the visitor on the MIDI file. */
    scaler.visitor::visit(mf);

    /* Save the result back to disk. */
    mf.save_to_file(output);
}

#include <algorithm>

#include "cppmidi.h"

void cppmidi::midi_file::load_from_file(const std::string& file_path) {
    // TODO
}

void cppmidi::midi_file::save_to_file(const std::string& file_path) {
    // TODO
}

void cppmidi::midi_file::sort_track_events() {
    for (cppmidi::midi_track& tr : midi_tracks) {
        auto cmp = [](const cppmidi::midi_event& a, const cppmidi::midi_event& b) const {
            return a.absolute_ticks() < b.absolute_ticks();
        }
        std::stable_sort(tr.midi_events.begin(), tr.midi_events.end(), cmp);
    }
}

//=============================================================================

std::vector<uint8_t> cppmidi::noteoff_message_midi_event::event_data() {
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0x8 << 4)), key, velocity
    };
    return retval;
}

std::vector<uint8_t> cppmidi::noteon_message_midi_event::event_data() {
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0x9 << 4)), key, velocity
    };
    return retval;
}

std::vector<uint8_t> cppmidi::noteaftertouch_message_midi_event::event_data() {
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0xA << 4)), key, value
    };
    return retval;
}

std::vector<uint8_t> cppmidi::controller_message_midi_event::event_data() {
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0xB << 4)), controller, value
    };
    return retval;
}

std::vector<uint8_t> cppmidi::program_message_midi_event::event_data() {
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0xC << 4)), program
    };
    return retval;
}

std::vector<uint8_t> cppmidi::channel_aftertouch_message_midi_event::event_data() {
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0xD << 4)), value
    };
    return retval;
}

std::vector<uint8_t> cppmidi::pitch_bend_message_midi_event::event_data() {
    uint16_t pitch_biased = static_cast<uint16_t>(pitch + 0x2000);
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0xE << 4)),
        static_cast<uint8_t>(pitch_biased & 0x7F),
        static_cast<uint8_t>((pitch_biased >> 7) & 0x7F)
    };
    return retval;
}

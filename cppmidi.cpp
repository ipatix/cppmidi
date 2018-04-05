#include <algorithm>
#include <stdexcept>
#include <fstream>

#include <cstring>
#include <cstdarg>

#include "cppmidi.h"

template<typename T, typename V>
static void throw_assert(T a, V b, const std::string& msg) {
    if (a != static_cast<T>(b))
        throw std::runtime_error(msg.c_str());
}

static void throw_xcept(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char msg[2048];
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);
    throw std::runtime_error(msg);
}

std::vector<uint8_t> cppmidi::len2vlv(uint64_t len) {
    if (len >= (1uLL << 32))
        throw std::runtime_error("Cannot construct VLV > 2^32");

    if (len >= (1uLL << 28)) {
        // 5 byte vlv
        std::vector<uint8_t> retval = {
            static_cast<uint8_t>((len >> 28) | 0x80),
            static_cast<uint8_t>(((len >> 21) & 0x7F) | 0x80),
            static_cast<uint8_t>(((len >> 14) & 0x7F) | 0x80),
            static_cast<uint8_t>(((len >> 7) & 0x7F) | 0x80),
            static_cast<uint8_t>(len & 0x7F)
        };
        return retval;
    } else if (len >= (1 << 21)) {
        // 4 byte vlv
        std::vector<uint8_t> retval = {
            static_cast<uint8_t>((len >> 21) | 0x80),
            static_cast<uint8_t>(((len >> 14) & 0x7F) | 0x80),
            static_cast<uint8_t>(((len >> 7) & 0x7F) | 0x80),
            static_cast<uint8_t>(len & 0x7F)
        };
        return retval;
    } else if (len >= (1 << 14)) {
        // 3 byte vlv
        std::vector<uint8_t> retval = {
            static_cast<uint8_t>((len >> 14) | 0x80),
            static_cast<uint8_t>(((len >> 7) & 0x7F) | 0x80),
            static_cast<uint8_t>(len & 0x7F)
        };
        return retval;
    } else if (len >= (1 << 7)) {
        // 2 byte vlv
        std::vector<uint8_t> retval = {
            static_cast<uint8_t>((len >> 7) | 0x80),
            static_cast<uint8_t>(len & 0x7F)
        };
        return retval;
    } else {
        // 1 byte vlv
        std::vector<uint8_t> retval = {
            static_cast<uint8_t>(len & 0x7F)
        };
        return retval;
    }
}

uint32_t cppmidi::vlv2len(const std::vector<uint8_t>& vlv) {
    if (vlv.size() > 5)
        throw std::runtime_error("vlv2len: vlv > 5 bytes");
    uint32_t retval = 0;
    unsigned int i = 1;
    for (uint8_t x : vlv) {
        if (i == 5 && (x & 0x70))
            throw std::runtime_error("vlv2len: resulting int > 32 bits");
        if (i == vlv.size()) {
            if (x & 0x80)
                throw std::runtime_error("vlv2len: len bit set on last byte");
        } else {
            if (!(x & 0x80))
                throw std::runtime_error("vlv2len: len bit not set on preceding bytes");
        }
        retval = static_cast<uint32_t>((x & 0x7F) | (retval << 7));
        i += 1;
    }
    return retval;
}

//=============================================================================

void load_type_zero(const std::vector<uint8_t>& midi_data, cppmidi::midi_file& mf) {
    // TODO
}

void load_type_one(const std::vector<uint8_t>& midi_data, cppmidi::midi_file& mf) {
    // TODO
}

void cppmidi::midi_file::load_from_file(const std::string& file_path) {
    // read file
    std::ifstream is(file_path, std::ios_base::binary);
    if (!is.is_open())
        throw_xcept("Error loading MIDI File: %s", strerror(errno));

    is.seekg(0, std::ios_base::end);
    long size = is.tellg();
    is.seekg(0, std::ios_base::beg);
    if (size < 0)
        throw_xcept("Failed to obtain file size: %s", strerror(errno));

    std::vector<uint8_t> midi_data(static_cast<size_t>(size));
    is.read(reinterpret_cast<char *>(midi_data.data()), midi_data.size());
    if (is.bad())
        throw std::runtime_error("std::ifstream::read bad");
    if (is.fail())
        throw std::runtime_error("std::ifstream::read fail");
    is.close();

    // check header magics
    throw_assert(midi_data.at(0), 'M', "Bad MIDI magic");
    throw_assert(midi_data.at(1), 'T', "Bad MIDI magic");
    throw_assert(midi_data.at(2), 'h', "Bad MIDI magic");
    throw_assert(midi_data.at(3), 'd', "Bad MIDI magic");

    // check header chunk len
    throw_assert(midi_data.at(4), 0, "Bad File Header chunk len");
    throw_assert(midi_data.at(5), 0, "Bad File Header chunk len");
    throw_assert(midi_data.at(6), 0, "Bad File Header chunk len");
    throw_assert(midi_data.at(7), 0, "Bad File Header chunk len");

    uint8_t midi_type = midi_data.at(9);
    if (midi_type > 2)
        throw_xcept("Illegal MIDI file type: %u", midi_type);
    if (midi_type == 2)
        throw std::runtime_error("MIDI file type 2 is not supported");

    this->time_division = static_cast<uint16_t>(
            (midi_data.at(0xC) << 8) | midi_data.at(0xD));

    if (midi_type == 0)
        load_type_zero(midi_data, *this);
    else
        load_type_one(midi_data, *this);
}

void cppmidi::midi_file::save_to_file(const std::string& file_path) const {
    // TODO
}

void cppmidi::midi_file::sort_track_events() {
    for (cppmidi::midi_track& tr : midi_tracks) {
        auto cmp = [](
                const std::unique_ptr<cppmidi::midi_event>& a,
                const std::unique_ptr<cppmidi::midi_event>& b)
        {
            return a->absolute_ticks() < b->absolute_ticks();
        };
        std::stable_sort(tr.midi_events.begin(), tr.midi_events.end(), cmp);
    }
}

//=============================================================================

std::vector<uint8_t> cppmidi::noteoff_message_midi_event::event_data() const {
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0x8 << 4)), key, velocity
    };
    return retval;
}

std::vector<uint8_t> cppmidi::noteon_message_midi_event::event_data() const {
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0x9 << 4)), key, velocity
    };
    return retval;
}

std::vector<uint8_t> cppmidi::noteaftertouch_message_midi_event::event_data() const {
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0xA << 4)), key, value
    };
    return retval;
}

std::vector<uint8_t> cppmidi::controller_message_midi_event::event_data() const {
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0xB << 4)), controller, value
    };
    return retval;
}

std::vector<uint8_t> cppmidi::program_message_midi_event::event_data() const {
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0xC << 4)), program
    };
    return retval;
}

std::vector<uint8_t> cppmidi::channel_aftertouch_message_midi_event::event_data() const {
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0xD << 4)), value
    };
    return retval;
}

std::vector<uint8_t> cppmidi::pitch_bend_message_midi_event::event_data() const {
    uint16_t pitch_biased = static_cast<uint16_t>(pitch + 0x2000);
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0xE << 4)),
        static_cast<uint8_t>(pitch_biased & 0x7F),
        static_cast<uint8_t>((pitch_biased >> 7) & 0x7F)
    };
    return retval;
}

//=============================================================================

std::vector<uint8_t> cppmidi::sequencenumber_meta_midi_event::event_data() const {
    if (empty) {
        std::vector<uint8_t> retval = {
            0xFF, 0x00, 0
        };
        return retval;
    } else {
        std::vector<uint8_t> retval = {
            0xFF, 0x00, 2,
            static_cast<uint8_t>(seq_num >> 8),
            static_cast<uint8_t>(seq_num & 0xFF)
        };
        return retval;
    }
}

std::vector<uint8_t> cppmidi::text_meta_midi_event::event_data() const {
    std::vector<uint8_t> retval = { 0xFF };
    std::vector<uint8_t> vlv = len2vlv(text.size());
    retval.insert(retval.end(), vlv.begin(), vlv.end());
    retval.insert(retval.end(),
            reinterpret_cast<const uint8_t*>(&text[0]),
            reinterpret_cast<const uint8_t*>(&text[text.size()]));
    return retval;
}

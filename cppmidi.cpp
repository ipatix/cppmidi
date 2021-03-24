#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <typeinfo>

#include <cstring>
#include <cstdarg>
#include <cassert>

#include "cppmidi.h"

// #include <cstdio>

template<typename T, typename V>
static void throw_assert(T a, V b, const std::string& msg) {
    if (a != static_cast<T>(b))
        throw cppmidi::xcept("%s", msg.c_str());
}

static std::string byte2str(uint8_t b) {
    auto val2char = [](int v) {
        if (v >= 0 && v <= 9)
            return static_cast<char>(v + '0');
        else
            return static_cast<char>(v - 10 + 'A');
    };
    std::string result;
    result += val2char(b >> 4);
    result += val2char(b & 0xF);
    return result;
}

// converts an uint to a [v]ariable [l]ength [v]alue
std::vector<uint8_t> cppmidi::len2vlv(uint64_t len) {
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

// converts a [v]ariable [l]ength [v]alue to a uint
uint32_t cppmidi::vlv2len(const std::vector<uint8_t>& vlv) {
    if (vlv.size() > 5)
        throw xcept("vlv2len: vlv > 5 bytes");
    uint32_t retval = 0;
    unsigned int i = 1;
    for (uint8_t x : vlv) {
        if (i == 5 && (x & 0x70))
            throw xcept("vlv2len: resulting int > 32 bits");
        if (i == vlv.size()) {
            if (x & 0x80)
                throw xcept("vlv2len: len bit set on last byte");
        } else {
            if (!(x & 0x80))
                throw xcept("vlv2len: len bit not set on preceding bytes");
        }
        retval = static_cast<uint32_t>((x & 0x7F) | (retval << 7));
        i += 1;
    }
    return retval;
}

uint32_t cppmidi::read_vlv(const std::vector<uint8_t>& midi_data, size_t& fpos) {
    uint32_t retval = 0;
    do {
        if (retval >= 0x10000000)
            throw xcept("Failed to read VLV (too big) at 0x%zx", fpos);
        retval = static_cast<uint32_t>((midi_data.at(fpos) & 0x7F) |
                (retval << 7));
        // range check below is not required because we already checked it
    } while (midi_data[fpos++] & 0x80);
    return retval;
}

std::unique_ptr<cppmidi::midi_event> cppmidi::read_event(
        const std::vector<uint8_t>& midi_data,
        size_t& fpos, uint8_t& current_midi_channel, running_state& current_rs,
        bool& sysex_ongoing, uint32_t current_tick) {
    // this function parses one midi event
    std::unique_ptr<midi_event> retval;
    uint8_t cmd = midi_data.at(fpos++);
    uint8_t ev_type = static_cast<uint8_t>(cmd >> 4);
    uint8_t ev_ch = static_cast<uint8_t>(cmd & 0xF);

    switch (ev_type) {
    case 0x8:
        // parse note off
        retval = std::make_unique<noteoff_message_midi_event>(
                    current_tick, ev_ch,
                    midi_data.at(fpos + 0),
                    midi_data.at(fpos + 1));
        fpos += 2;
        current_midi_channel = ev_ch;
        current_rs = running_state::NoteOff;
        break;
    case 0x9:
        // parse note on
        if (midi_data.at(fpos + 1) == 0) {
            retval = std::make_unique<noteoff_message_midi_event>(
                        current_tick, ev_ch,
                        midi_data.at(fpos + 0),
                        midi_data.at(fpos + 1));
        } else {
            retval = std::make_unique<noteon_message_midi_event>(
                        current_tick, ev_ch,
                        midi_data.at(fpos + 0),
                        midi_data.at(fpos + 1));
        }
        fpos += 2;
        current_midi_channel = ev_ch;
        current_rs = running_state::NoteOn;
        break;
    case 0xA:
        // parse note aftertouch
        retval = std::make_unique<noteaftertouch_message_midi_event>(
                    current_tick, ev_ch,
                    midi_data.at(fpos + 0),
                    midi_data.at(fpos + 1));
        fpos += 2;
        current_midi_channel = ev_ch;
        current_rs = running_state::NoteAftertouch;
        break;
    case 0xB:
        // parse controller
        retval = std::make_unique<controller_message_midi_event>(
                    current_tick, ev_ch,
                    midi_data.at(fpos),
                    midi_data.at(fpos + 1));
        fpos += 2;
        current_midi_channel = ev_ch;
        current_rs = running_state::Controller;
        break;
    case 0xC:
        // parse program change
        retval = std::make_unique<program_message_midi_event>(
                    current_tick, ev_ch,
                    midi_data.at(fpos++));
        current_midi_channel = ev_ch;
        current_rs = running_state::Program;
        break;
    case 0xD:
        // parse channel aftertouch
        retval = std::make_unique<channelaftertouch_message_midi_event>(
                    current_tick, ev_ch,
                    midi_data.at(fpos++));
        current_midi_channel = ev_ch;
        current_rs = running_state::ChannelAftertouch;
        break;
    case 0xE:
        // parse pitch bend
        {
            int pitch = (midi_data.at(fpos + 0) & 0x7F) |
                ((midi_data.at(fpos + 1) & 0x7F) << 7);
            pitch -= 0x2000;
            retval = std::make_unique<pitchbend_message_midi_event>(
                    current_tick, ev_ch, static_cast<int16_t>(pitch));
        }
        fpos += 2;
        current_midi_channel = ev_ch;
        current_rs = running_state::PitchBend;
        break;
    case 0xF:
        // parse meta or sysex
        // ev_ch isn't really a MIDI channel here
        if (ev_ch == 0xF) {
            // parse meta event
            uint8_t type = midi_data.at(fpos++);
            uint32_t len = read_vlv(midi_data, fpos);
            if (fpos + len > midi_data.size()) {
                throw xcept("MIDI parser error: Meta Event reaching over end of file "
                        "at 0x%X", fpos);
            }

            switch (type) {
            case 0x0:
                if (len == 0) {
                    retval = std::make_unique<sequencenumber_meta_midi_event>(current_tick);
                } else if (len == 2) {
                    uint16_t seq_num = static_cast<uint16_t>(
                            (midi_data[fpos + 0] << 8) |
                            midi_data[fpos + 1]);
                    fpos += 2;
                    retval = std::make_unique<sequencenumber_meta_midi_event>(current_tick, seq_num);
                } else {
                    throw xcept("MIDI parser error: Invalid sequence number format "
                            "at 0x%X", fpos);
                }
                break;
            case 0x1:
                {
                    std::string text(reinterpret_cast<const char*>(&midi_data[fpos]),
                            reinterpret_cast<const char*>(&midi_data[fpos + len]));
                    retval = std::make_unique<text_meta_midi_event>(current_tick,
                            std::move(text));
                    fpos += len;
                }
                break;
            case 0x2:
                {
                    std::string copyright(reinterpret_cast<const char*>(&midi_data[fpos]),
                            reinterpret_cast<const char*>(&midi_data[fpos + len]));
                    retval = std::make_unique<copyright_meta_midi_event>(current_tick,
                            std::move(copyright));
                    fpos += len;
                }
                break;
            case 0x3:
                {
                    std::string trackname(reinterpret_cast<const char*>(&midi_data[fpos]),
                            reinterpret_cast<const char*>(&midi_data[fpos + len]));
                    retval = std::make_unique<trackname_meta_midi_event>(current_tick,
                            std::move(trackname));
                    fpos += len;
                }
                break;
            case 0x4:
                {
                    std::string instrument(reinterpret_cast<const char*>(&midi_data[fpos]),
                            reinterpret_cast<const char*>(&midi_data[fpos + len]));
                    retval = std::make_unique<instrument_meta_midi_event>(current_tick,
                            std::move(instrument));
                    fpos += len;
                }
                break;
            case 0x5:
                {
                    std::string lyric(reinterpret_cast<const char*>(&midi_data[fpos]),
                            reinterpret_cast<const char*>(&midi_data[fpos + len]));
                    retval = std::make_unique<lyric_meta_midi_event>(current_tick,
                            std::move(lyric));
                    fpos += len;
                }
                break;
            case 0x6:
                {
                    std::string marker(reinterpret_cast<const char*>(&midi_data[fpos]),
                            reinterpret_cast<const char*>(&midi_data[fpos + len]));
                    retval = std::make_unique<marker_meta_midi_event>(current_tick,
                            std::move(marker));
                    fpos += len;
                }
                break;
            case 0x7:
                {
                    std::string cuepoint(reinterpret_cast<const char*>(&midi_data[fpos]),
                            reinterpret_cast<const char*>(&midi_data[fpos + len]));
                    retval = std::make_unique<cuepoint_meta_midi_event>(current_tick,
                            std::move(cuepoint));
                    fpos += len;
                }
                break;
            case 0x8:
                {
                    std::string programname(reinterpret_cast<const char*>(&midi_data[fpos]),
                            reinterpret_cast<const char*>(&midi_data[fpos + len]));
                    retval = std::make_unique<programname_meta_midi_event>(current_tick,
                            std::move(programname));
                    fpos += len;
                }
                break;
            case 0x9:
                {
                    std::string devicename(reinterpret_cast<const char*>(&midi_data[fpos]),
                            reinterpret_cast<const char*>(&midi_data[fpos + len]));
                    retval = std::make_unique<devicename_meta_midi_event>(current_tick,
                            std::move(devicename));
                    fpos += len;
                }
                break;
            case 0x20:
                if (len != 1) {
                    throw xcept("MIDI parser error: Invalid Channel Prefix "
                            "at 0x%X", fpos);
                }
                retval = std::make_unique<channelprefix_meta_midi_event>(current_tick,
                        midi_data[fpos++]);
                break;
            case 0x21:
                if (len != 1) {
                    throw xcept("MIDI parser error: Invalid MIDI Port "
                            "at 0x%X", fpos);
                }
                retval = std::make_unique<midiport_meta_midi_event>(current_tick,
                        midi_data[fpos++]);
                break;
            case 0x2F:
                // signal the calling function that the end of track has been reached
                // retval will keep the nullptr from initialization
                break;
            case 0x51:
                if (len != 3) {
                    throw xcept("MIDI parser error: Invalid Tempo "
                            "at 0x%X", fpos);
                } else {
                    uint32_t tempo = static_cast<uint32_t>(midi_data[fpos++] << 16);
                    tempo |= static_cast<uint32_t>(midi_data[fpos++] << 8);
                    tempo |= static_cast<uint32_t>(midi_data[fpos++]);
                    retval = std::make_unique<tempo_meta_midi_event>(current_tick, tempo);
                }
                break;
            case 0x54:
                if (len != 5) {
                    throw xcept("MIDI parser error: Invalid SMPTE Offset "
                            "at 0x%X", fpos);
                } else {
                    uint8_t frame_rate = static_cast<uint8_t>((midi_data[fpos] >> 6) & 0b11);
                    uint8_t hour = static_cast<uint8_t>(midi_data[fpos++] & 0b11111);
                    uint8_t minute = midi_data[fpos++];
                    uint8_t second = midi_data[fpos++];
                    uint8_t frames = midi_data[fpos++];
                    uint8_t frame_fractions = midi_data[fpos++];
                    retval = std::make_unique<smpteoffset_meta_midi_event>(current_tick,
                            frame_rate, hour, minute, second, frames,
                            frame_fractions);
                }
                break;
            case 0x58:
                if (len != 4) {
                    throw xcept("MIDI parser error: Invalid Time Signature "
                            "at 0x%X", fpos);
                } else {
                    uint8_t numerator = midi_data[fpos++];
                    uint8_t denominator = midi_data[fpos++];
                    uint8_t clocks = midi_data[fpos++];
                    uint8_t n32n = midi_data[fpos++];
                    retval = std::make_unique<timesignature_meta_midi_event>(current_tick,
                            numerator, denominator, clocks, n32n);
                }
                break;
            case 0x59:
                if (len != 2) {
                    throw xcept("MIDI parser error: Invalid Key Signature "
                            "at 0x%X", fpos);
                } else {
                    int8_t sharp_flats = static_cast<int8_t>(midi_data[fpos++]);
                    bool _minor = static_cast<bool>(midi_data[fpos++]);
                    retval = std::make_unique<keysignature_meta_midi_event>(current_tick,
                            sharp_flats, _minor);
                }
                break;
            case 0x7F:
                {
                    std::vector<uint8_t> data(&midi_data[fpos], &midi_data[fpos + len]);
                    fpos += len;
                    retval = std::make_unique<sequencerspecific_meta_midi_event>(
                            current_tick, std::move(data));
                }
                break;
            default:
                throw xcept("MIDI parser error: Unknown Meta Event: %02X "
                        "at 0x%X", static_cast<uint32_t>(type), fpos);
            }
        } else if (ev_ch == 0x7) {
            uint32_t len = read_vlv(midi_data, fpos);
            if (fpos + len > midi_data.size()) {
                throw xcept("MIDI parser error: SysEx/Escape Event reaching over end of file "
                        "at 0x%X", fpos);
            }
            std::vector<uint8_t> data(&midi_data[fpos], &midi_data[fpos + len]);
            fpos += len;
            if (sysex_ongoing) {
                // sysex continuation
                if (len < 1) {
                    throw xcept("MIDI parser error: Unable to Read ongoing SysEx Terminal "
                            "at 0x%X", fpos);
                }
                if (data[data.size() - 1] == 0x7F)
                    sysex_ongoing = false;
                retval = std::make_unique<sysex_midi_event>(current_tick,
                        std::move(data), false);
            } else {
                // escape sequence
                retval = std::make_unique<escape_midi_event>(current_tick, std::move(data));
            }
        } else if (ev_ch == 0x0) {
            // sysex begin
            uint32_t len = read_vlv(midi_data, fpos);
            if (fpos + len > midi_data.size()) {
                throw xcept("MIDI parser error: SysEx/Escape Event reaching over end of file "
                        "at 0x%X", fpos);
            }
            std::vector<uint8_t> data(&midi_data[fpos], &midi_data[fpos + len]);
            fpos += len;
            if (len < 1) {
                throw xcept("MIDI parser error: Unable to Read SysEx Terminal "
                        "at 0x%X", fpos);
            }
            if (data[data.size() - 1] == 0x7F)
                sysex_ongoing = false;
            else
                sysex_ongoing = true;
            retval = std::make_unique<sysex_midi_event>(current_tick, std::move(data), true);
        } else {
            throw xcept("MIDI parser error: Bad Byte 0xF%X "
                    " at 0x%X", ev_ch, fpos);
        }
        break;
    default:
        // parse dependent on running state
        switch (current_rs) {
        case running_state::NoteOff:
            retval = std::make_unique<noteoff_message_midi_event>(current_tick,
                    current_midi_channel, cmd, midi_data.at(fpos++));
            break;
        case running_state::NoteOn:
            {
                uint8_t vel = midi_data.at(fpos++);
                if (vel) {
                    retval = std::make_unique<noteon_message_midi_event>(current_tick,
                            current_midi_channel, cmd, vel);
                } else {
                    retval = std::make_unique<noteoff_message_midi_event>(current_tick,
                            current_midi_channel, cmd, vel);
                }
            }
            break;
        case running_state::NoteAftertouch:
            retval = std::make_unique<noteaftertouch_message_midi_event>(current_tick,
                    current_midi_channel, cmd, midi_data.at(fpos++));
            break;
        case running_state::Controller:
            retval = std::make_unique<controller_message_midi_event>(current_tick,
                    current_midi_channel, cmd, midi_data.at(fpos++));
            break;
        case running_state::Program:
            retval = std::make_unique<program_message_midi_event>(current_tick,
                    current_midi_channel, cmd);
            break;
        case running_state::ChannelAftertouch:
            retval = std::make_unique<channelaftertouch_message_midi_event>(current_tick,
                    current_midi_channel, cmd);
            break;
        case running_state::PitchBend:
            {
                int16_t pitch = static_cast<int16_t>(cmd | ((midi_data.at(fpos++) & 0x7F) << 7));
                // midi data is unsigned, parse as signed, subtract bias
                pitch = static_cast<int16_t>(pitch - 0x2000);
                retval = std::make_unique<pitchbend_message_midi_event>(current_tick,
                        current_midi_channel, pitch);
            }
            break;
        default:
            throw xcept("MIDI parser error: Use of running state without inital command "
                    "at 0x%X", fpos);
        }
        break;
    }
    return retval;
}


//=============================================================================

static void load_type_zero(const std::vector<uint8_t>& midi_data, cppmidi::midi_file& mf) {
    using namespace cppmidi;
    uint16_t num_tracks = static_cast<uint16_t>(
            (midi_data.at(0xA) << 8) | midi_data.at(0xB));
    if (num_tracks != 1)
        throw xcept("MIDI type 0 with more than one track");
    size_t fpos = 0xE;

    // one track for one channel
    for (int i = 0; i < 16; i++) {
        mf.midi_tracks.emplace_back();
    }

    uint32_t current_tick = 0;
    uint8_t current_midi_channel = 0;
    uint8_t current_meta_track = 0;
    running_state current_state = running_state::Undef;
    bool sysex_ongoing = false;

    throw_assert(midi_data.at(fpos++), 'M', "Bad MIDI Track Magic");
    throw_assert(midi_data.at(fpos++), 'T', "Bad MIDI Track Magic");
    throw_assert(midi_data.at(fpos++), 'r', "Bad MIDI Track Magic");
    throw_assert(midi_data.at(fpos++), 'k', "Bad MIDI Track Magic");

    uint32_t track_length = static_cast<uint32_t>(
            (midi_data.at(fpos + 0) << 24) |
            (midi_data.at(fpos + 1) << 16) |
            (midi_data.at(fpos + 2) << 8) |
            midi_data.at(fpos + 3));
    fpos += 4;
    size_t track_start = fpos;

    while (1) {
        uint64_t overflow_tick = current_tick + read_vlv(midi_data, fpos);
        if (overflow_tick >= 0x100000000)
            throw xcept("MIDI parser: Too many ticks for int32");
        current_tick = static_cast<uint32_t>(overflow_tick);
        std::unique_ptr<midi_event> ev = read_event(midi_data, fpos,
                current_midi_channel, current_state, sysex_ongoing,
                current_tick);

        if (!ev)
            break;

        /* determinate the track index on which to insert
         * the current MIDI event by examining its type */
        uint8_t insert_track = 0;
        if (typeid(*ev) == typeid(tempo_meta_midi_event) ||
                typeid(*ev) == typeid(sysex_midi_event) ||
                typeid(*ev) == typeid(escape_midi_event)) {
            insert_track = 0;
        } else if (typeid(*ev) == typeid(channelprefix_meta_midi_event)) {
            insert_track = current_meta_track = static_cast
                <cppmidi::channelprefix_meta_midi_event&>
                (*ev).get_channel() & 0xF;
        } else if (dynamic_cast<cppmidi::message_midi_event *>(ev.get())) {
            insert_track = static_cast<cppmidi::message_midi_event&>
                (*ev).channel() & 0xF;
        } else if (dynamic_cast<cppmidi::meta_midi_event *>(ev.get())) {
            insert_track = current_meta_track;
        }

        mf.midi_tracks[insert_track].midi_events.emplace_back(std::move(ev));
    }

    if (track_start + track_length != fpos) {
        throw xcept("MIDI Type 0 error: Incorrect Track Length"
                ", track data ends at 0x%X", fpos);
    }
}

static void load_type_one(const std::vector<uint8_t>& midi_data, cppmidi::midi_file& mf) {
    using namespace cppmidi;
    uint16_t num_tracks = static_cast<uint16_t>(
            (midi_data.at(0xA) << 8) | midi_data.at(0xB));
    size_t fpos = 0xE;

    for (uint16_t trk = 0; trk < num_tracks; trk++) {
        mf.midi_tracks.emplace_back();

        uint32_t current_tick = 0;
        uint8_t current_midi_channel = 0;
        cppmidi::running_state current_state = cppmidi::running_state::Undef;
        bool sysex_ongoing = false;

        throw_assert(midi_data.at(fpos++), 'M', "Bad MIDI Track Magic");
        throw_assert(midi_data.at(fpos++), 'T', "Bad MIDI Track Magic");
        throw_assert(midi_data.at(fpos++), 'r', "Bad MIDI Track Magic");
        throw_assert(midi_data.at(fpos++), 'k', "Bad MIDI Track Magic");

        uint32_t track_length = static_cast<uint32_t>(
                (midi_data.at(fpos + 0) << 24) |
                (midi_data.at(fpos + 1) << 16) |
                (midi_data.at(fpos + 2) << 8) |
                midi_data.at(fpos + 3));
        fpos += 4;
        size_t track_start = fpos;

        while (1) {
            //printf("Parsing VLV at location 0x%zX\n", fpos);
            uint64_t overflow_tick = current_tick + read_vlv(midi_data, fpos);
            if (overflow_tick >= 0x100000000)
                throw xcept("MIDI parser: Too many ticks for int32");
            current_tick = static_cast<uint32_t>(overflow_tick);
            //printf("Parsing Event at location 0x%zX\n", fpos);
            std::unique_ptr<cppmidi::midi_event> ev = read_event(midi_data, fpos,
                    current_midi_channel, current_state, sysex_ongoing,
                    current_tick);

            if (!ev)
                break;

            mf.midi_tracks[trk].midi_events.emplace_back(std::move(ev));
        }

        if (track_start + track_length != fpos) {
            throw xcept("MIDI Type 1 error: Incorrect Track Length for track %u "
                    ", track data ends at 0x%X", trk, fpos);
        }
    }
}

void cppmidi::midi_file::load_from_file(const std::filesystem::path& file_path) {
    // read file
    std::ifstream is(file_path, std::ios_base::binary);
    // reading errno here is a bit hacky, but it does kinda work
    if (!is.is_open())
        throw xcept("Error loading MIDI File: %s", strerror(errno));

    is.seekg(0, std::ios_base::end);
    std::streampos size = is.tellg();
    is.seekg(0, std::ios_base::beg);
    if (size < 0)
        throw xcept("Failed to obtain file size: %s", strerror(errno));

    std::vector<uint8_t> midi_data(static_cast<size_t>(size));
    is.read(reinterpret_cast<char *>(midi_data.data()),
            static_cast<std::streamsize>(midi_data.size()));
    if (is.bad())
        throw xcept("std::ifstream::read bad");
    if (is.fail())
        throw xcept("std::ifstream::read fail");
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
    throw_assert(midi_data.at(7), 6, "Bad File Header chunk len");

    uint16_t midi_type = static_cast<uint16_t>(
            (midi_data.at(8) << 8) | midi_data.at(9));
    if (midi_type > 2)
        throw xcept("Illegal MIDI file type: %u", midi_type);
    if (midi_type == 2)
        throw xcept("MIDI file type 2 is not supported");

    time_division = static_cast<uint16_t>(
            (midi_data.at(0xC) << 8) | midi_data.at(0xD));
    if (time_division & 0x8000)
        throw xcept("MIDI parser error: frames/second time division: unsupported");
    if (time_division == 0)
        throw xcept("MIDI parser error: time division is zero");

    if (midi_type == 0)
        load_type_zero(midi_data, *this);
    else
        load_type_one(midi_data, *this);
}

void cppmidi::midi_file::save_to_file(const std::filesystem::path& file_path) const {
    std::vector<uint8_t> data;
    // file magic
    data.push_back(static_cast<uint8_t>('M'));
    data.push_back(static_cast<uint8_t>('T'));
    data.push_back(static_cast<uint8_t>('h'));
    data.push_back(static_cast<uint8_t>('d'));

    // header chunk size
    data.push_back(0);
    data.push_back(0);
    data.push_back(0);
    data.push_back(6);

    // midi type #1
    data.push_back(0);
    data.push_back(1);

    // num tracks
    data.push_back(static_cast<uint8_t>(midi_tracks.size() >> 8));
    data.push_back(static_cast<uint8_t>(midi_tracks.size()));

    // time division
    data.push_back(static_cast<uint8_t>(time_division >> 8));
    data.push_back(static_cast<uint8_t>(time_division));

    std::vector<uint8_t> zero_vlv = len2vlv(0);
    std::vector<uint8_t> eot_data = endoftrack_meta_midi_event(0).event_data();

    for (size_t trk = 0; trk < midi_tracks.size(); trk++) {
        // track header
        data.push_back(static_cast<uint8_t>('M'));
        data.push_back(static_cast<uint8_t>('T'));
        data.push_back(static_cast<uint8_t>('r'));
        data.push_back(static_cast<uint8_t>('k'));

        // spaceholder for
        uint32_t track_len_pos = static_cast<uint32_t>(data.size());
        data.push_back(0);
        data.push_back(0);
        data.push_back(0);
        data.push_back(0);

        // event data
        uint32_t track_start_pos = static_cast<uint32_t>(data.size());
        uint32_t last_event_time = 0;
        for (const auto& ev : midi_tracks[trk]) {
            std::vector<uint8_t> ev_data = ev->event_data();
            if (typeid(*ev) == typeid(endoftrack_meta_midi_event))
                break;
            uint32_t event_time = ev->ticks;
            std::vector<uint8_t> vlv = len2vlv(event_time - last_event_time);
            last_event_time = event_time;
            data.insert(data.end(), vlv.begin(), vlv.end());
            data.insert(data.end(), ev_data.begin(), ev_data.end());
        }
        data.insert(data.end(), zero_vlv.begin(), zero_vlv.end());
        data.insert(data.end(), eot_data.begin(), eot_data.end());

        uint32_t track_end_pos = static_cast<uint32_t>(data.size());
        uint32_t track_len = track_end_pos - track_start_pos;
        data[track_len_pos + 0] = static_cast<uint8_t>(track_len >> 24);
        data[track_len_pos + 1] = static_cast<uint8_t>(track_len >> 16);
        data[track_len_pos + 2] = static_cast<uint8_t>(track_len >> 8);
        data[track_len_pos + 3] = static_cast<uint8_t>(track_len >> 0);
    }

    std::ofstream fout(file_path, std::ios::out | std::ios::binary);
    if (!fout.is_open())
        throw xcept("Error saving MIDI File: %s", strerror(errno));
    fout.write(reinterpret_cast<char*>(data.data()),
            static_cast<std::streamsize>(data.size()));
    if (fout.bad())
        throw xcept("std::ofstream::write bad");
    if (fout.fail())
        throw xcept("std::ofstream::write fail");
    fout.close();
}

void cppmidi::midi_track::print(std::ostream& os, const std::string& indent) const {
    std::string event_indent = indent + "  ";

    for (const auto& ev_ptr : midi_events) {
        ev_ptr->print(os, event_indent);
        os << std::endl;
    }
}

void cppmidi::midi_track::sort_events() {
    auto cmp = [](
            const std::unique_ptr<cppmidi::midi_event>& a,
            const std::unique_ptr<cppmidi::midi_event>& b)
    {
        return a->ticks < b->ticks;
    };
    std::stable_sort(midi_events.begin(), midi_events.end(), cmp);
}

void cppmidi::midi_file::sort_track_events() {
    for (cppmidi::midi_track& tr : midi_tracks) {
        tr.sort_events();
    }
}

void cppmidi::midi_file::convert_time_division(uint16_t time_division) {
    if (time_division & 0x8000)
        throw xcept("Cannot convert time division to frames/second: unsupported");

    // save a bit of processing time if time division is already the same
    if (time_division == this->time_division)
        return;

    for (midi_track& trk : midi_tracks) {
        for (auto& ev : trk.midi_events) {
            uint64_t ticks = ev->ticks;
            ticks *= time_division;
            ticks /= this->time_division;
            if (ticks >= 0x100000000)
                throw xcept("Cannot convert time division: int32 tick overflow");
            ev->ticks = static_cast<uint32_t>(ticks);
        }
    }
    this->time_division = time_division;
}

void cppmidi::midi_file::print(std::ostream& os, const std::string& indent) const {
    std::string track_indent = indent + "  ";

    os << indent << "File Begin:" << std::endl;

    for (size_t iTrk = 0; iTrk < midi_tracks.size(); iTrk++) {
        os << track_indent << "Track #" << iTrk << " Begin:" << std::endl;
        midi_tracks.at(iTrk).print(os, track_indent);
        os << track_indent << "Track #" << iTrk << " End" << std::endl;
    }

    os << indent << "File End" << std::endl;
}

//=============================================================================

std::vector<uint8_t> cppmidi::dummy_midi_event::event_data() const {
    throw xcept("dummy events cannot be serialized");
}

void cppmidi::dummy_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Dummy";
}

std::vector<uint8_t> cppmidi::noteoff_message_midi_event::event_data() const {
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0x8 << 4)), key, velocity
    };
    return retval;
}

void cppmidi::noteoff_message_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Chn. #" << +midi_channel <<
        ": Note Off: key=" << +key << " velocity=" << +velocity;
}

std::vector<uint8_t> cppmidi::noteon_message_midi_event::event_data() const {
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0x9 << 4)), key, velocity
    };
    return retval;
}

void cppmidi::noteon_message_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Chn. #" << +midi_channel <<
        ": Note On: key=" << +key << " velocity=" << +velocity;
}

std::vector<uint8_t> cppmidi::noteaftertouch_message_midi_event::event_data() const {
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0xA << 4)), key, value
    };
    return retval;
}

void cppmidi::noteaftertouch_message_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Chn. #" << +midi_channel <<
        ": Note Aftertouch: key=" << +key << " value=" << +value;
}

std::vector<uint8_t> cppmidi::controller_message_midi_event::event_data() const {
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0xB << 4)), controller, value
    };
    return retval;
}

void cppmidi::controller_message_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Chn. #" << +midi_channel <<
        ": Controller: cc=" << +controller << " value=" << +value;
}

std::vector<uint8_t> cppmidi::program_message_midi_event::event_data() const {
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0xC << 4)), program
    };
    return retval;
}

void cppmidi::program_message_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Chn. #" << +midi_channel <<
        ": Program: no=" << +program;
}

std::vector<uint8_t> cppmidi::channelaftertouch_message_midi_event::event_data() const {
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0xD << 4)), value
    };
    return retval;
}

void cppmidi::channelaftertouch_message_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Chn. #" << +midi_channel <<
        ": Channel Aftertouch: value=" << +value;
}

std::vector<uint8_t> cppmidi::pitchbend_message_midi_event::event_data() const {
    uint16_t pitch_biased = static_cast<uint16_t>(pitch + 0x2000);
    std::vector<uint8_t> retval = {
        static_cast<uint8_t>(midi_channel | (0xE << 4)),
        static_cast<uint8_t>(pitch_biased & 0x7F),
        static_cast<uint8_t>((pitch_biased >> 7) & 0x7F)
    };
    return retval;
}

void cppmidi::pitchbend_message_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Chn. #" << +midi_channel <<
        ": Pitch Bend: pitch=" << pitch;
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

void cppmidi::sequencenumber_meta_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Meta Sequence Number: ";
    if (empty)
        os << "empty";
    else
        os << "seq_num=" << +seq_num;
}

std::vector<uint8_t> cppmidi::text_meta_midi_event::event_data() const {
    std::vector<uint8_t> retval = { 0xFF, 0x01 };
    std::vector<uint8_t> vlv = len2vlv(text.size());
    retval.insert(retval.end(), vlv.begin(), vlv.end());
    retval.insert(retval.end(),
            reinterpret_cast<const uint8_t*>(&text[0]),
            reinterpret_cast<const uint8_t*>(&text[text.size()]));
    return retval;
}

void cppmidi::text_meta_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Meta Text: " << text;
}

std::vector<uint8_t> cppmidi::copyright_meta_midi_event::event_data() const {
    std::vector<uint8_t> retval = { 0xFF, 0x02 };
    std::vector<uint8_t> vlv = len2vlv(text.size());
    retval.insert(retval.end(), vlv.begin(), vlv.end());
    retval.insert(retval.end(),
            reinterpret_cast<const uint8_t*>(&text[0]),
            reinterpret_cast<const uint8_t*>(&text[text.size()]));
    return retval;
}

void cppmidi::copyright_meta_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Meta Copyright: " << text;
}

std::vector<uint8_t> cppmidi::trackname_meta_midi_event::event_data() const {
    std::vector<uint8_t> retval = { 0xFF, 0x03 };
    std::vector<uint8_t> vlv = len2vlv(text.size());
    retval.insert(retval.end(), vlv.begin(), vlv.end());
    retval.insert(retval.end(),
            reinterpret_cast<const uint8_t*>(&text[0]),
            reinterpret_cast<const uint8_t*>(&text[text.size()]));
    return retval;
}

void cppmidi::trackname_meta_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Meta Track Name: " << text;
}

std::vector<uint8_t> cppmidi::instrument_meta_midi_event::event_data() const {
    std::vector<uint8_t> retval = { 0xFF, 0x04 };
    std::vector<uint8_t> vlv = len2vlv(text.size());
    retval.insert(retval.end(), vlv.begin(), vlv.end());
    retval.insert(retval.end(),
            reinterpret_cast<const uint8_t*>(&text[0]),
            reinterpret_cast<const uint8_t*>(&text[text.size()]));
    return retval;
}

void cppmidi::instrument_meta_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Meta Instrument Name: " << text;
}

std::vector<uint8_t> cppmidi::lyric_meta_midi_event::event_data() const {
    std::vector<uint8_t> retval = { 0xFF, 0x05 };
    std::vector<uint8_t> vlv = len2vlv(text.size());
    retval.insert(retval.end(), vlv.begin(), vlv.end());
    retval.insert(retval.end(),
            reinterpret_cast<const uint8_t*>(&text[0]),
            reinterpret_cast<const uint8_t*>(&text[text.size()]));
    return retval;
}

void cppmidi::lyric_meta_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Meta Lyric: " << text;
}

std::vector<uint8_t> cppmidi::marker_meta_midi_event::event_data() const {
    std::vector<uint8_t> retval = { 0xFF, 0x06 };
    std::vector<uint8_t> vlv = len2vlv(text.size());
    retval.insert(retval.end(), vlv.begin(), vlv.end());
    retval.insert(retval.end(),
            reinterpret_cast<const uint8_t*>(&text[0]),
            reinterpret_cast<const uint8_t*>(&text[text.size()]));
    return retval;
}

void cppmidi::marker_meta_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Meta Marker: " << text;
}

std::vector<uint8_t> cppmidi::cuepoint_meta_midi_event::event_data() const {
    std::vector<uint8_t> retval = { 0xFF, 0x07 };
    std::vector<uint8_t> vlv = len2vlv(text.size());
    retval.insert(retval.end(), vlv.begin(), vlv.end());
    retval.insert(retval.end(),
            reinterpret_cast<const uint8_t*>(&text[0]),
            reinterpret_cast<const uint8_t*>(&text[text.size()]));
    return retval;
}

void cppmidi::cuepoint_meta_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Meta Cue Point : " << text;
}

std::vector<uint8_t> cppmidi::programname_meta_midi_event::event_data() const {
    std::vector<uint8_t> retval = { 0xFF, 0x08 };
    std::vector<uint8_t> vlv = len2vlv(text.size());
    retval.insert(retval.end(), vlv.begin(), vlv.end());
    retval.insert(retval.end(),
            reinterpret_cast<const uint8_t*>(&text[0]),
            reinterpret_cast<const uint8_t*>(&text[text.size()]));
    return retval;
}

void cppmidi::programname_meta_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Meta Program Name: " << text;
}

std::vector<uint8_t> cppmidi::devicename_meta_midi_event::event_data() const {
    std::vector<uint8_t> retval = { 0xFF, 0x09 };
    std::vector<uint8_t> vlv = len2vlv(text.size());
    retval.insert(retval.end(), vlv.begin(), vlv.end());
    retval.insert(retval.end(),
            reinterpret_cast<const uint8_t*>(&text[0]),
            reinterpret_cast<const uint8_t*>(&text[text.size()]));
    return retval;
}

void cppmidi::devicename_meta_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Meta Device Name: " << text;
}

std::vector<uint8_t> cppmidi::channelprefix_meta_midi_event::event_data() const {
    std::vector<uint8_t> retval = { 0xFF, 0x20, 1, channel };
    return retval;
}

void cppmidi::channelprefix_meta_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Meta Channel Prefix: " << +channel;
}

std::vector<uint8_t> cppmidi::midiport_meta_midi_event::event_data() const {
    std::vector<uint8_t> retval = { 0xFF, 0x21, 1, port };
    return retval;
}

void cppmidi::midiport_meta_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Meta MIDI Port: " << +port;
}

std::vector<uint8_t> cppmidi::endoftrack_meta_midi_event::event_data() const {
    std::vector<uint8_t> retval = { 0xFF, 0x2F, 0 };
    return retval;
}

void cppmidi::endoftrack_meta_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Meta End of Track";
}

std::vector<uint8_t> cppmidi::tempo_meta_midi_event::event_data() const {
    uint8_t a = static_cast<uint8_t>(us_per_beat >> 16);
    uint8_t b = static_cast<uint8_t>(us_per_beat >> 8);
    uint8_t c = static_cast<uint8_t>(us_per_beat >> 0);
    std::vector<uint8_t> retval = { 0xFF, 0x51, 3, a, b, c };
    return retval;
}

void cppmidi::tempo_meta_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Meta Tempo: " << get_bpm() << " BPM";
}

std::vector<uint8_t> cppmidi::smpteoffset_meta_midi_event::event_data() const {
    uint8_t hr = static_cast<uint8_t>((frame_rate << 6) | hour);
    std::vector<uint8_t> retval = { 0xFF, 0x54, 5, hr, minute, second, frames, frame_fractions };
    return retval;
}

void cppmidi::smpteoffset_meta_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Meta SMPTE Offset: " <<
        "frame_rate=" << +frame_rate << " hour=" << +hour <<
        " minute=" << +minute << " second=" << +second <<
        " frames=" << +frames << " frame_fractions=" << +frame_fractions;
}

void cppmidi::smpteoffset_meta_midi_event::errchk() {
    if (frame_rate > 3 || hour > 23 || minute > 59 ||
            second > 59 || frame_fractions > 99) {
        throw xcept("Invalid SMPTE offset arguments");
    }
    if ((frame_rate == 0 && frames > 23) ||
            (frame_rate == 1 && frames > 24) ||
            (frame_rate == 2 && frames > 28) ||
            (frame_rate == 3 && frames > 29)) {
        throw xcept("Invalid SMPTE offset arguments");
    }
}

std::vector<uint8_t> cppmidi::timesignature_meta_midi_event::event_data() const {
    std::vector<uint8_t> retval = { 0xFF, 0x58, 4, numerator, denominator, tick_clocks, n32n };
    return retval;
}

void cppmidi::timesignature_meta_midi_event::print(std::ostream& os, const std::string& indent) const {
    size_t den = 1;
    for (uint8_t i = 0; i < denominator; i++)
        den *= 2;
    os << indent << "t=" << ticks << ": Meta Time Signature: " <<
        +numerator << "/" << den << " ticks_per_metronome_click=" << +tick_clocks <<
        " 32nd_per_quarter=" << +n32n;
}

std::vector<uint8_t> cppmidi::keysignature_meta_midi_event::event_data() const {
    uint8_t sf = static_cast<uint8_t>(sharp_flats);
    std::vector<uint8_t> retval = { 0xFF, 0x59, 2, sf, _minor };
    return retval;
}

void cppmidi::keysignature_meta_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Meta Key Signature: ";

    if (_minor)
        os << "minor";
    else
        os << "major";

    if (sharp_flats >= 0)
        os << " num_sharps=" << +sharp_flats;
    else
        os << " num_flats=" << -sharp_flats;
}

void cppmidi::keysignature_meta_midi_event::errchk() {
    if (sharp_flats < -7 || sharp_flats > 7)
        throw xcept("Key Signature: Invalid n# of sharps");
}

std::vector<uint8_t> cppmidi::sequencerspecific_meta_midi_event::event_data() const {
    std::vector<uint8_t> retval = { 0xFF, 0x7F };
    std::vector<uint8_t> vlv = len2vlv(data.size());
    retval.insert(retval.end(), vlv.begin(), vlv.end());
    retval.insert(retval.end(), data.begin(), data.end());
    return retval;
}

void cppmidi::sequencerspecific_meta_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Meta Sequencer Specific: ";
    if (data.size() == 0) {
        os << "<empty>";
    } else {
        os << byte2str(data.at(0));
        for (size_t i = 1; i < data.size(); i++) {
            os << " " << byte2str(data.at(i));
        }
    }
}

//=============================================================================

std::vector<uint8_t> cppmidi::sysex_midi_event::event_data() const {
    std::vector<uint8_t> retval;
    if (first_chunk)
        retval.push_back(0xF0);
    else
        retval.push_back(0xF7);
    std::vector<uint8_t> vlv = len2vlv(data.size());
    retval.insert(retval.end(), vlv.begin(), vlv.end());
    retval.insert(retval.end(), data.begin(), data.end());
    return retval;
}

void cppmidi::sysex_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": SysEx: ";

    if (first_chunk) {
        os << "begin ";
    } else {
        os << "continue ";
    }

    if (data.size() == 0) {
        os << "<empty>";
    } else {
        os << byte2str(data.at(0));
        for (size_t i = 1; i < data.size(); i++) {
            os << " " << byte2str(data.at(i));
        }
    }
}

std::vector<uint8_t> cppmidi::escape_midi_event::event_data() const {
    std::vector<uint8_t> retval = { 0xF7 };
    std::vector<uint8_t> vlv = len2vlv(data.size());
    retval.insert(retval.end(), vlv.begin(), vlv.end());
    retval.insert(retval.end(), data.begin(), data.end());
    return retval;
}

void cppmidi::escape_midi_event::print(std::ostream& os, const std::string& indent) const {
    os << indent << "t=" << ticks << ": Escape: ";

    if (data.size() == 0) {
        os << "<empty>";
    } else {
        os << byte2str(data.at(0));
        for (size_t i = 1; i < data.size(); i++) {
            os << " " << byte2str(data.at(i));
        }
    }
}

//=============================================================================

cppmidi::xcept::xcept(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char msg_buf[2048];
    vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
    msg = msg_buf;
    va_end(args);
}

const char *cppmidi::xcept::what() const noexcept {
    return msg.c_str();
}

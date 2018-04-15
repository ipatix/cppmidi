#pragma once

#include <algorithm>
#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include <exception>

namespace cppmidi {
    std::vector<uint8_t> len2vlv(uint64_t len);
    uint32_t vlv2len(const std::vector<uint8_t>& vlv);

    uint32_t read_vlv(const std::vector<uint8_t>& midi_data, size_t& fpos);

    enum class running_state {
        Undef,
        NoteOff,
        NoteOn,
        NoteAftertouch,
        Controller,
        Program,
        ChannelAftertouch,
        PitchBend,
    };

    enum class ev_type {
        MsgNoteOff,
        MsgNoteOn,
        MsgNoteAftertouch,
        MsgController,
        MsgProgram,
        MsgChannelAftertouch,
        MsgPitchBend,
        MetaSequenceNumber,
        MetaText,
        MetaCopyright,
        MetaTrackName,
        MetaInstrument,
        MetaLyric,
        MetaMarker,
        MetaCuepoint,
        MetaProgramName,
        MetaDeviceName,
        MetaChannelPrefix,
        MetaMidiPort,
        MetaEndOfTrack,
        MetaTempo,
        MetaSMPTE,
        MetaTimeSignature,
        MetaKeySignature,
        MetaSequencerSpecific,
        SysEx,
        Escape,
    };

    class midi_event {
    public:
        virtual std::vector<uint8_t> event_data() const = 0;
        virtual ev_type event_type() const = 0;
        uint32_t ticks;
    protected:
        midi_event(uint32_t ticks) : ticks(ticks) {}
    };

    midi_event *read_event(const std::vector<uint8_t>& midi_data, size_t& fpos,
            uint8_t& current_midi_channel, running_state& current_rs,
            bool& sysex_ongoing, uint32_t current_tick);

    struct midi_track {
        std::vector<std::unique_ptr<midi_event>> midi_events;
    };

    struct midi_file {
        uint16_t time_division;
        std::vector<midi_track> midi_tracks;

        midi_file() : time_division(0) {}

        void load_from_file(const std::string& file_path);
        void save_to_file(const std::string& file_path) const;
        void sort_track_events();
        void convert_time_division(uint16_t time_division);
    };

    //=========================================================================

    class message_midi_event : public midi_event {
    public:
        uint8_t channel() const { return midi_channel; }
    protected:
        message_midi_event(uint32_t ticks, uint8_t midi_channel)
            : midi_event(ticks), midi_channel(midi_channel & 0xF) {}
        uint8_t midi_channel;
    };

    //=====

    class noteoff_message_midi_event : public message_midi_event {
    public:
        noteoff_message_midi_event(uint32_t ticks, uint8_t midi_channel,
                uint8_t key, uint8_t velocity)
            : message_midi_event(ticks, midi_channel),
            key(key & 0x7F), velocity(velocity & 0x7F) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        uint8_t get_key() const { return key; }
        void set_key(uint8_t key) {
            this->key = static_cast<uint8_t>(key & 0x7F);
        }
        uint8_t get_velocity() const { return velocity; }
        void set_velocity(uint8_t velocity) {
            this->velocity = static_cast<uint8_t>(velocity & 0x7F);
        }
    private:
        uint8_t key, velocity;
    };

    class noteon_message_midi_event : public message_midi_event {
    public:
        noteon_message_midi_event(uint32_t ticks, uint8_t midi_channel,
                uint8_t key, uint8_t velocity)
            : message_midi_event(ticks, midi_channel),
            key(key & 0x7F), velocity(velocity & 0x7F) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        uint8_t get_key() const { return key; }
        void set_key(uint8_t key) {
            this->key = static_cast<uint8_t>(key & 0x7F);
        }
        uint8_t get_velocity() const { return velocity; }
        void set_velocity(uint8_t velocity ) {
            this->velocity = static_cast<uint8_t>(velocity & 0x7F);
        }
    private:
        uint8_t key, velocity;
    };

    class noteaftertouch_message_midi_event : public message_midi_event {
    public:
        noteaftertouch_message_midi_event(uint32_t ticks, uint8_t midi_channel,
                uint8_t key, uint8_t value)
            : message_midi_event(ticks, midi_channel),
            key(key & 0x7F), value(value & 0x7F) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        uint8_t get_key() const { return key; }
        void set_key(uint8_t key) {
            this->key = static_cast<uint8_t>(key & 0x7F);
        }
        uint8_t get_value() const { return value; }
        void set_value(uint8_t value) {
            this->value = static_cast<uint8_t>(value & 0x7F);
        }
    private:
        uint8_t key, value;
    };

    class controller_message_midi_event : public message_midi_event {
    public:
        controller_message_midi_event(uint32_t ticks, uint8_t midi_channel,
                uint8_t controller, uint8_t value)
            : message_midi_event(ticks, midi_channel),
            controller(controller & 0x7F), value(value & 0x7F) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        uint8_t get_controller() const { return controller; }
        void set_controller(uint8_t controller) {
            this->controller = static_cast<uint8_t>(controller & 0x7F);
        }
        uint8_t get_value() const { return value; }
        void set_value(uint8_t value) {
            this->value = static_cast<uint8_t>(value & 0x7F);
        }
    private:
        uint8_t controller, value;
    };

    class program_message_midi_event : public message_midi_event {
    public:
        program_message_midi_event(uint32_t ticks, uint8_t midi_channel,
                uint8_t program)
            : message_midi_event(ticks, midi_channel),
            program(program & 0x7F) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        uint8_t get_program() const { return program; }
        void set_program(uint8_t program) {
            this->program = static_cast<uint8_t>(program & 0x7F);
        }
    private:
        uint8_t program;
    };

    class channelaftertouch_message_midi_event : public message_midi_event {
    public:
        channelaftertouch_message_midi_event(uint32_t ticks, uint8_t midi_channel,
                uint8_t value)
            : message_midi_event(ticks, midi_channel),
            value(value & 0x7F) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        uint8_t get_value() const { return value; }
        void set_value(uint8_t value) {
            this->value = static_cast<uint8_t>(value & 0x7F);
        }
    private:
        uint8_t value;
    };

    class pitchbend_message_midi_event : public message_midi_event {
    public:
        pitchbend_message_midi_event(uint32_t ticks, uint8_t midi_channel,
                int16_t pitch)
            : message_midi_event(ticks, midi_channel),
            pitch(pitch) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        int16_t get_pitch() const { return pitch; }
        void set_pitch(int16_t pitch) {
            this->pitch = std::max<int16_t>(-0x2000, std::min<int16_t>(0x1FFF, pitch));
        }
    private:
        int16_t pitch;
    };

    //=========================================================================

    class meta_midi_event : public midi_event {
    protected:
        meta_midi_event(uint32_t ticks)
            : midi_event(ticks) {}
    };

    //=====

    class sequencenumber_meta_midi_event : public meta_midi_event {
    public:
        sequencenumber_meta_midi_event(uint32_t ticks, uint16_t seq_num)
            : meta_midi_event(ticks), seq_num(seq_num), empty(false) {}
        sequencenumber_meta_midi_event(uint32_t ticks)
            : meta_midi_event(ticks), seq_num(0), empty(true) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        uint16_t get_seq_num() const { return seq_num; }
        bool get_empty() const { return empty; }
    private:
        uint16_t seq_num;
        bool empty;
    };

    class text_meta_midi_event : public meta_midi_event {
    public:
        text_meta_midi_event(uint32_t ticks, const std::string& text)
            : meta_midi_event(ticks), text(text) {}
        text_meta_midi_event(uint32_t ticks, std::string&& text)
            : meta_midi_event(ticks), text(text) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        const std::string& get_text() const { return text; }
    private:
        std::string text;
    };

    class copyright_meta_midi_event : public meta_midi_event {
    public:
        copyright_meta_midi_event(uint32_t ticks, const std::string& text)
            : meta_midi_event(ticks), text(text) {}
        copyright_meta_midi_event(uint32_t ticks, std::string&& text)
            : meta_midi_event(ticks), text(text) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        const std::string& get_text() const { return text; }
    private:
        std::string text;
    };

    class trackname_meta_midi_event : public meta_midi_event {
    public:
        trackname_meta_midi_event(uint32_t ticks, const std::string& text)
            : meta_midi_event(ticks), text(text) {}
        trackname_meta_midi_event(uint32_t ticks, std::string&& text)
            : meta_midi_event(ticks), text(text) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        const std::string& get_text() const { return text; }
    private:
        std::string text;
    };

    class instrument_meta_midi_event : public meta_midi_event {
    public:
        instrument_meta_midi_event(uint32_t ticks, const std::string& text)
            : meta_midi_event(ticks), text(text) {}
        instrument_meta_midi_event(uint32_t ticks, std::string&& text)
            : meta_midi_event(ticks), text(text) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        const std::string& get_text() const { return text; }
    private:
        std::string text;
    };

    class lyric_meta_midi_event : public meta_midi_event {
    public:
        lyric_meta_midi_event(uint32_t ticks, const std::string& text)
            : meta_midi_event(ticks), text(text) {}
        lyric_meta_midi_event(uint32_t ticks, std::string&& text)
            : meta_midi_event(ticks), text(text) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        const std::string& get_text() const { return text; }
    private:
        std::string text;
    };

    class marker_meta_midi_event : public meta_midi_event {
    public:
        marker_meta_midi_event(uint32_t ticks, const std::string& text)
            : meta_midi_event(ticks), text(text) {}
        marker_meta_midi_event(uint32_t ticks, std::string&& text)
            : meta_midi_event(ticks), text(text) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        const std::string& get_text() const { return text; }
    private:
        std::string text;
    };

    class cuepoint_meta_midi_event : public meta_midi_event {
    public:
        cuepoint_meta_midi_event(uint32_t ticks, const std::string& text)
            : meta_midi_event(ticks), text(text) {}
        cuepoint_meta_midi_event(uint32_t ticks, std::string&& text)
            : meta_midi_event(ticks), text(text) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        const std::string& get_text() const { return text; }
    private:
        std::string text;
    };

    class programname_meta_midi_event : public meta_midi_event {
    public:
        programname_meta_midi_event(uint32_t ticks, const std::string& text)
            : meta_midi_event(ticks), text(text) {}
        programname_meta_midi_event(uint32_t ticks, std::string&& text)
            : meta_midi_event(ticks), text(text) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        const std::string& get_text() const { return text; }
    private:
        std::string text;
    };

    class devicename_meta_midi_event : public meta_midi_event {
    public:
        devicename_meta_midi_event(uint32_t ticks, const std::string& text)
            : meta_midi_event(ticks), text(text) {}
        devicename_meta_midi_event(uint32_t ticks, std::string&& text)
            : meta_midi_event(ticks), text(text) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        const std::string& get_text() const { return text; }
    private:
        std::string text;
    };

    class channelprefix_meta_midi_event : public meta_midi_event {
    public:
        channelprefix_meta_midi_event(uint32_t ticks, uint8_t channel)
            : meta_midi_event(ticks), channel(channel & 0xF) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        uint8_t get_channel() const { return channel; }
    private:
        uint8_t channel;
    };

    class midiport_meta_midi_event : public meta_midi_event {
    public:
        midiport_meta_midi_event(uint32_t ticks, uint8_t port)
            : meta_midi_event(ticks), port(port & 0x7F) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        uint8_t get_port() const { return port; }
    private:
        uint8_t port;
    };

    class endoftrack_meta_midi_event : public meta_midi_event {
    public:
        endoftrack_meta_midi_event(uint32_t ticks)
            : meta_midi_event(ticks) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
    };

    class tempo_meta_midi_event : public meta_midi_event {
    public:
        tempo_meta_midi_event(uint32_t ticks, uint32_t us_per_beat)
            : meta_midi_event(ticks), us_per_beat(us_per_beat) {}
        tempo_meta_midi_event(uint32_t ticks, double bpm)
            : meta_midi_event(ticks),
            us_per_beat(static_cast<uint32_t>(1000000.0 * 60.0 / bpm)) {
                errchk();
            }
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        uint32_t get_us_per_beat() const { return us_per_beat; }
    private:
        void errchk();
        uint32_t us_per_beat;
    };

    class smpteoffset_meta_midi_event : public meta_midi_event {
    public:
        smpteoffset_meta_midi_event(uint32_t ticks, uint8_t frame_rate,
                uint8_t hour, uint8_t minute, uint8_t second,
                uint8_t frames, uint8_t frame_fractions)
            : meta_midi_event(ticks), frame_rate(frame_rate),
            hour(hour), minute(minute), second(second),
            frames(frames), frame_fractions(frame_fractions) {
                errchk();
            }
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        uint8_t get_frame_rate() const { return frame_rate; }
        uint8_t get_hour() const { return hour; }
        uint8_t get_minute() const { return minute; }
        uint8_t get_second() const { return second; }
        uint8_t get_frames() const { return frames; }
        uint8_t get_frame_fractions() const { return frame_fractions; }
    private:
        void errchk();
        uint8_t frame_rate;
        uint8_t hour, minute, second;
        uint8_t frames, frame_fractions;
    };

    class timesignature_meta_midi_event : public meta_midi_event {
    public:
        timesignature_meta_midi_event(uint32_t ticks,
                uint8_t numerator, uint8_t denominator,
                uint8_t tick_clocks, uint8_t n32n)
            : meta_midi_event(ticks),
            numerator(numerator), denominator(denominator),
            tick_clocks(tick_clocks), n32n(n32n) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        uint8_t get_numerator() const { return numerator; }
        uint8_t get_denominator() const { return denominator; }
        uint8_t get_tick_clocks() const { return tick_clocks; }
        uint8_t get_n32n() const { return n32n; }
    private:
        void errchk();
        uint8_t numerator, denominator;
        uint8_t tick_clocks, n32n;
    };

    class keysignature_meta_midi_event : public meta_midi_event {
    public:
        keysignature_meta_midi_event(uint32_t ticks, int8_t sharp_flats,
                bool _minor)
            : meta_midi_event(ticks), sharp_flats(sharp_flats), _minor(_minor) {
                errchk();
            }
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        int8_t get_sharp_flats() const { return sharp_flats; }
        bool get_minor() const { return _minor; }
    private:
        void errchk();
        int8_t sharp_flats;
        bool _minor;
    };

    class sequencerspecific_meta_midi_event : public meta_midi_event {
    public:
        sequencerspecific_meta_midi_event(uint32_t ticks,
                const std::vector<uint8_t>& data)
            : meta_midi_event(ticks), data(data) {}
        sequencerspecific_meta_midi_event(uint32_t ticks,
                std::vector<uint8_t>&& data)
            : meta_midi_event(ticks), data(data) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        const std::vector<uint8_t>& get_data() const { return data; }
    private:
        std::vector<uint8_t> data;
    };

    //=========================================================================

    class sysex_midi_event : public midi_event {
    public:
        sysex_midi_event(uint32_t ticks, const std::vector<uint8_t>& data,
                bool first_chunk)
            : midi_event(ticks), data(data), first_chunk(first_chunk) {}
        sysex_midi_event(uint32_t ticks, std::vector<uint8_t>&& data,
                bool first_chunk)
            : midi_event(ticks), data(data), first_chunk(first_chunk) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        const std::vector<uint8_t>& get_data() const { return data; }
        bool get_first_chunk() const { return first_chunk; }
    private:
        std::vector<uint8_t> data;
        bool first_chunk;
    };

    class escape_midi_event : public midi_event {
    public:
        escape_midi_event(uint32_t ticks, const std::vector<uint8_t>& data)
            : midi_event(ticks), data(data) {}
        escape_midi_event(uint32_t ticks, std::vector<uint8_t>&& data)
            : midi_event(ticks), data(data) {}
        std::vector<uint8_t> event_data() const override;
        ev_type event_type() const override;
        const std::vector<uint8_t>& get_data() const { return data; }
    private:
        std::vector<uint8_t> data;
    };

    //=========================================================================
    
    class xcept : public std::exception {
    public:
        xcept(const char *fmt, ...);
        const char *what() const noexcept override;
    private:
        std::string msg;
    };
}

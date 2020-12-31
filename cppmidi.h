#pragma once

#include <algorithm>
#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include <exception>
#include <filesystem>

#define MIDI_CC_MSB_BANK_SELECT 0
#define MIDI_CC_MSB_MOD         1
#define MIDI_CC_MSB_BREATH      2
// #3 not defined
#define MIDI_CC_MSB_FOOT        4
#define MIDI_CC_MSB_PORT_TIME   5 // portamento time
#define MIDI_CC_MSB_DATA_ENTRY  6
#define MIDI_CC_MSB_VOLUME      7
#define MIDI_CC_MSB_BALANCE     8
// #9 not defined
#define MIDI_CC_MSB_PAN         10
#define MIDI_CC_MSB_EXPRESSION  11
#define MIDI_CC_MSB_FX_CTRL_1   12
#define MIDI_CC_MSB_FX_CTRL_2   13
// #14..15 not defined
#define MIDI_CC_MSB_GP_1        16 // general purpose 1..4
#define MIDI_CC_MSB_GP_2        17
#define MIDI_CC_MSB_GP_3        18
#define MIDI_CC_MSB_GP_4        19
// #20..31 not defined
#define MIDI_CC_LSB_BANK_SELECT 32
#define MIDI_CC_LSB_MOD         33
#define MIDI_CC_LSB_BREATH      34
// #35 not defined
#define MIDI_CC_LSB_FOOT        36
#define MIDI_CC_LSB_PORT_TIME   37
#define MIDI_CC_LSB_DATA_ENTRY  38
#define MIDI_CC_LSB_VOLUME      39
#define MIDI_CC_LSB_BALANCE     40
// #41 not defined
#define MIDI_CC_LSB_PAN         42
#define MIDI_CC_LSB_EXPRESSION  43
#define MIDI_CC_LSB_FX_CTRL_1   44
#define MIDI_CC_LSB_FX_CTRL_2   45
// #46..47 not defined
#define MIDI_CC_LSB_GP_1        48
#define MIDI_CC_LSB_GP_2        49
#define MIDI_CC_LSB_GP_3        50
#define MIDI_CC_LSB_GP_4        51
// #52..63 not defined
#define MIDI_CC_SUSTAIN_PEDAL   64
#define MIDI_CC_PORT_SWITCH     65 // portamento
#define MIDI_CC_SOST_SWITCH     66 // sostenuto
#define MIDI_CC_SOFT_PEDAL      67
#define MIDI_CC_LEGATO_SWITCH   68
#define MIDI_CC_HOLD2           69
#define MIDI_CC_SND_CTRL_1      70
#define MIDI_CC_SND_CTRL_2      71
#define MIDI_CC_SND_CTRL_3      72
#define MIDI_CC_SND_CTRL_4      73
#define MIDI_CC_SND_CTRL_5      74
#define MIDI_CC_SND_CTRL_6      75
#define MIDI_CC_SND_CTRL_7      76
#define MIDI_CC_SND_CTRL_8      77
#define MIDI_CC_SND_CTRL_9      78
#define MIDI_CC_SND_CTRL_10     79
#define MIDI_CC_GP_SWITCH_1     80
#define MIDI_CC_GP_SWITCH_2     81
#define MIDI_CC_GP_SWITCH_3     82
#define MIDI_CC_GP_SWITCH_4     83
#define MIDI_CC_PORT_CTRL       84 // portamento
// #85..90 not defined
#define MIDI_CC_FX_DEPTH_1      91
#define MIDI_CC_FX_DEPTH_2      92
#define MIDI_CC_FX_DEPTH_3      93
#define MIDI_CC_FX_DEPTH_4      94
#define MIDI_CC_FX_DEPTH_5      95
#define MIDI_CC_DATA_INC        96
#define MIDI_CC_DATA_DEC        97
#define MIDI_CC_LSB_NRPN        98
#define MIDI_CC_MSB_NRPN        99
#define MIDI_CC_LSB_RPN         100
#define MIDI_CC_MSB_RPN         101
// #102..119 not defined
#define MIDI_CC_ALL_SOUND_OFF   120
#define MIDI_CC_ALL_CTRL_RESET  121
#define MIDI_CC_LOCAL_SWITCH    122
#define MIDI_CC_ALL_NOTES_OFF   123
#define MIDI_CC_OMNI_MODE_OFF   124
#define MIDI_CC_OMNI_MODE_ON    125
#define MIDI_CC_MONO_MODE       126
#define MIDI_CC_POLY_MODE       127

namespace cppmidi {
    //=========================================================================

    class visitor;
    class dummy_midi_event;
    class noteoff_message_midi_event;
    class noteon_message_midi_event;
    class noteaftertouch_message_midi_event;
    class controller_message_midi_event;
    class program_message_midi_event;
    class channelaftertouch_message_midi_event;
    class pitchbend_message_midi_event;
    class sequencenumber_meta_midi_event;
    class text_meta_midi_event;
    class copyright_meta_midi_event;
    class trackname_meta_midi_event;
    class instrument_meta_midi_event;
    class lyric_meta_midi_event;
    class marker_meta_midi_event;
    class cuepoint_meta_midi_event;
    class programname_meta_midi_event;
    class devicename_meta_midi_event;
    class channelprefix_meta_midi_event;
    class midiport_meta_midi_event;
    class endoftrack_meta_midi_event;
    class tempo_meta_midi_event;
    class smpteoffset_meta_midi_event;
    class timesignature_meta_midi_event;
    class keysignature_meta_midi_event;
    class sequencerspecific_meta_midi_event;
    class sysex_midi_event;
    class escape_midi_event;

    //=========================================================================
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

    class midi_event {
    public:
        virtual ~midi_event() = default;
        virtual std::vector<uint8_t> event_data() const = 0;
        uint32_t ticks;

        virtual void accept(visitor& v) = 0;
    protected:
        midi_event(uint32_t ticks) : ticks(ticks) {}
    };

    std::unique_ptr<midi_event> read_event(const std::vector<uint8_t>& midi_data,
            size_t& fpos, uint8_t& current_midi_channel, running_state& current_rs,
            bool& sysex_ongoing, uint32_t current_tick);

    struct midi_track {
        std::vector<std::unique_ptr<midi_event>> midi_events;

        const std::unique_ptr<midi_event>& operator[](size_t i) const {
            return midi_events[i];
        }
        std::unique_ptr<midi_event>& operator[](size_t i) {
            return midi_events[i];
        }
        auto begin() { return midi_events.begin(); }
        auto begin() const { return midi_events.begin(); }
        auto end() { return midi_events.end(); }
        auto end() const { return midi_events.end(); }

        void sort_events();
    };

    struct midi_file {
        uint16_t time_division;
        std::vector<midi_track> midi_tracks;

        const midi_track& operator[](size_t i) const {
            return midi_tracks[i];
        }
        midi_track& operator[](size_t i) {
            return midi_tracks[i];
        }
        auto begin() { return midi_tracks.begin(); }
        auto begin() const { return midi_tracks.begin(); }
        auto end() { return midi_tracks.end(); }
        auto end() const { return midi_tracks.end(); }

        midi_file() : time_division(48) {}

        void load_from_file(const std::filesystem::path& file_path);
        void save_to_file(const std::filesystem::path& file_path) const;
        void sort_track_events();
        void convert_time_division(uint16_t time_division);
    };

    //=========================================================================

    class visitor {
    public:
        void visit(midi_track& mtrk) {
            for (auto& mevt : mtrk.midi_events) {
                mevt->accept(*this);
            }
        }
        void visit(midi_file& mf) {
            for (auto& mtrk : mf.midi_tracks) {
                visit(mtrk);
            }
        }
        virtual void visit(dummy_midi_event&) {}
        virtual void visit(noteoff_message_midi_event&) {}
        virtual void visit(noteon_message_midi_event&) {}
        virtual void visit(noteaftertouch_message_midi_event&) {}
        virtual void visit(controller_message_midi_event&) {}
        virtual void visit(program_message_midi_event&) {}
        virtual void visit(channelaftertouch_message_midi_event&) {}
        virtual void visit(pitchbend_message_midi_event&) {}
        virtual void visit(sequencenumber_meta_midi_event&) {}
        virtual void visit(text_meta_midi_event&) {}
        virtual void visit(copyright_meta_midi_event&) {}
        virtual void visit(trackname_meta_midi_event&) {}
        virtual void visit(instrument_meta_midi_event&) {}
        virtual void visit(lyric_meta_midi_event&) {}
        virtual void visit(marker_meta_midi_event&) {}
        virtual void visit(cuepoint_meta_midi_event&) {}
        virtual void visit(programname_meta_midi_event&) {}
        virtual void visit(devicename_meta_midi_event&) {}
        virtual void visit(channelprefix_meta_midi_event&) {}
        virtual void visit(midiport_meta_midi_event&) {}
        virtual void visit(endoftrack_meta_midi_event&) {}
        virtual void visit(tempo_meta_midi_event&) {}
        virtual void visit(smpteoffset_meta_midi_event&) {}
        virtual void visit(timesignature_meta_midi_event&) {}
        virtual void visit(keysignature_meta_midi_event&) {}
        virtual void visit(sequencerspecific_meta_midi_event&) {}
        virtual void visit(sysex_midi_event&) {}
        virtual void visit(escape_midi_event&) {}
    protected:
        visitor() = default;
    };

    //=========================================================================

    // The dummy event is nothing specified by midi.
    // It's intended to be used by the user program only
    class dummy_midi_event : public midi_event {
    public:
        dummy_midi_event(uint32_t ticks) : midi_event(ticks) {}
        std::vector<uint8_t> event_data() const override;
        void accept(visitor& v) override { v.visit(*this); }
    };

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
        uint8_t get_key() const { return key; }
        void set_key(uint8_t key) {
            this->key = static_cast<uint8_t>(key & 0x7F);
        }
        uint8_t get_velocity() const { return velocity; }
        void set_velocity(uint8_t velocity) {
            this->velocity = static_cast<uint8_t>(velocity & 0x7F);
        }
        void accept(visitor& v) override { v.visit(*this); }
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
        uint8_t get_key() const { return key; }
        void set_key(uint8_t key) {
            this->key = static_cast<uint8_t>(key & 0x7F);
        }
        uint8_t get_velocity() const { return velocity; }
        void set_velocity(uint8_t velocity ) {
            this->velocity = static_cast<uint8_t>(velocity & 0x7F);
        }
        void accept(visitor& v) override { v.visit(*this); }
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
        uint8_t get_key() const { return key; }
        void set_key(uint8_t key) {
            this->key = static_cast<uint8_t>(key & 0x7F);
        }
        uint8_t get_value() const { return value; }
        void set_value(uint8_t value) {
            this->value = static_cast<uint8_t>(value & 0x7F);
        }
        void accept(visitor& v) override { v.visit(*this); }
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
        uint8_t get_controller() const { return controller; }
        void set_controller(uint8_t controller) {
            this->controller = static_cast<uint8_t>(controller & 0x7F);
        }
        uint8_t get_value() const { return value; }
        void set_value(uint8_t value) {
            this->value = static_cast<uint8_t>(value & 0x7F);
        }
        void accept(visitor& v) override { v.visit(*this); }
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
        uint8_t get_program() const { return program; }
        void set_program(uint8_t program) {
            this->program = static_cast<uint8_t>(program & 0x7F);
        }
        void accept(visitor& v) override { v.visit(*this); }
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
        uint8_t get_value() const { return value; }
        void set_value(uint8_t value) {
            this->value = static_cast<uint8_t>(value & 0x7F);
        }
        void accept(visitor& v) override { v.visit(*this); }
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
        int16_t get_pitch() const { return pitch; }
        void set_pitch(int16_t pitch) {
            this->pitch = std::clamp<int16_t>(pitch, -0x2000, 0x1FFF);
        }
        void accept(visitor& v) override { v.visit(*this); }
    private:
        int16_t pitch;
    };

    //=========================================================================

    class meta_midi_event : public midi_event {
    public:
        virtual ~meta_midi_event() = default;
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
        uint16_t get_seq_num() const { return seq_num; }
        bool get_empty() const { return empty; }
        void accept(visitor& v) override { v.visit(*this); }
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
        const std::string& get_text() const { return text; }
        void accept(visitor& v) override { v.visit(*this); }
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
        const std::string& get_text() const { return text; }
        void accept(visitor& v) override { v.visit(*this); }
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
        const std::string& get_text() const { return text; }
        void accept(visitor& v) override { v.visit(*this); }
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
        const std::string& get_text() const { return text; }
        void accept(visitor& v) override { v.visit(*this); }
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
        const std::string& get_text() const { return text; }
        void accept(visitor& v) override { v.visit(*this); }
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
        const std::string& get_text() const { return text; }
        void accept(visitor& v) override { v.visit(*this); }
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
        const std::string& get_text() const { return text; }
        void accept(visitor& v) override { v.visit(*this); }
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
        const std::string& get_text() const { return text; }
        void accept(visitor& v) override { v.visit(*this); }
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
        const std::string& get_text() const { return text; }
        void accept(visitor& v) override { v.visit(*this); }
    private:
        std::string text;
    };

    class channelprefix_meta_midi_event : public meta_midi_event {
    public:
        channelprefix_meta_midi_event(uint32_t ticks, uint8_t channel)
            : meta_midi_event(ticks), channel(channel & 0xF) {}
        std::vector<uint8_t> event_data() const override;
        uint8_t get_channel() const { return channel; }
        void accept(visitor& v) override { v.visit(*this); }
    private:
        uint8_t channel;
    };

    class midiport_meta_midi_event : public meta_midi_event {
    public:
        midiport_meta_midi_event(uint32_t ticks, uint8_t port)
            : meta_midi_event(ticks), port(port & 0x7F) {}
        std::vector<uint8_t> event_data() const override;
        uint8_t get_port() const { return port; }
        void accept(visitor& v) override { v.visit(*this); }
    private:
        uint8_t port;
    };

    class endoftrack_meta_midi_event : public meta_midi_event {
    public:
        endoftrack_meta_midi_event(uint32_t ticks)
            : meta_midi_event(ticks) {}
        std::vector<uint8_t> event_data() const override;
        void accept(visitor& v) override { v.visit(*this); }
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
        uint32_t get_us_per_beat() const { return us_per_beat; }
        double get_bpm() const { return 1000000.0 * 60.0 / us_per_beat; }
        void accept(visitor& v) override { v.visit(*this); }
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
        uint8_t get_frame_rate() const { return frame_rate; }
        uint8_t get_hour() const { return hour; }
        uint8_t get_minute() const { return minute; }
        uint8_t get_second() const { return second; }
        uint8_t get_frames() const { return frames; }
        uint8_t get_frame_fractions() const { return frame_fractions; }
        void accept(visitor& v) override { v.visit(*this); }
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
        uint8_t get_numerator() const { return numerator; }
        uint8_t get_denominator() const { return denominator; }
        uint8_t get_tick_clocks() const { return tick_clocks; }
        uint8_t get_n32n() const { return n32n; }
        void accept(visitor& v) override { v.visit(*this); }
    private:
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
        int8_t get_sharp_flats() const { return sharp_flats; }
        bool get_minor() const { return _minor; }
        void accept(visitor& v) override { v.visit(*this); }
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
        const std::vector<uint8_t>& get_data() const { return data; }
        void accept(visitor& v) override { v.visit(*this); }
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
        const std::vector<uint8_t>& get_data() const { return data; }
        bool get_first_chunk() const { return first_chunk; }
        void accept(visitor& v) override { v.visit(*this); }
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
        const std::vector<uint8_t>& get_data() const { return data; }
        void accept(visitor& v) override { v.visit(*this); }
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

#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <memory>

namespace cppmidi {
    std::vector<uint8_t> len2vlv(uint64_t len);
    uint32_t vlv2len(const std::vector<uint8_t>& vlv);

    class midi_event {
        public:
            uint64_t absolute_ticks() const {
                return ticks;
            }
            virtual std::vector<uint8_t> event_data() const = 0;
        protected:
            midi_event(uint64_t ticks) : ticks(ticks) {}
            uint64_t ticks;
    };

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
    };

    //=========================================================================

    class message_midi_event : public midi_event {
        protected:
            message_midi_event(uint64_t ticks, uint8_t midi_channel)
                : midi_event(ticks), midi_channel(midi_channel) {}
            uint8_t midi_channel;
    };

    //=====

    class noteoff_message_midi_event : public message_midi_event {
        public:
            noteoff_message_midi_event(uint64_t ticks, uint8_t midi_channel,
                    uint8_t key, uint8_t velocity)
                : message_midi_event(ticks, midi_channel),
                key(key), velocity(velocity) {}
            std::vector<uint8_t> event_data() const override;
        private:
            uint8_t key, velocity;
    };

    class noteon_message_midi_event : public message_midi_event {
        public:
            noteon_message_midi_event(uint64_t ticks, uint8_t midi_channel,
                    uint8_t key, uint8_t velocity)
                : message_midi_event(ticks, midi_channel),
                key(key), velocity(velocity) {}
            std::vector<uint8_t> event_data() const override;
        private:
            uint8_t key, velocity;
    };

    class noteaftertouch_message_midi_event : public message_midi_event {
        public:
            noteaftertouch_message_midi_event(uint64_t ticks, uint8_t midi_channel,
                    uint8_t key, uint8_t value)
                : message_midi_event(ticks, midi_channel),
                key(key), value(value) {}
            std::vector<uint8_t> event_data() const override;
        private:
            uint8_t key, value;
    };

    class controller_message_midi_event : public message_midi_event {
        public:
            controller_message_midi_event(uint64_t ticks, uint8_t midi_channel,
                    uint8_t controller, uint8_t value)
                : message_midi_event(ticks, midi_channel),
                controller(controller), value(value) {}
            std::vector<uint8_t> event_data() const override;
        private:
            uint8_t controller, value;
    };

    class program_message_midi_event : public message_midi_event {
        public:
            program_message_midi_event(uint64_t ticks, uint8_t midi_channel,
                    uint8_t program)
                : message_midi_event(ticks, midi_channel),
                program(program) {}
            std::vector<uint8_t> event_data() const override;
        private:
            uint8_t program;
    };

    class channel_aftertouch_message_midi_event : public message_midi_event {
        public:
            channel_aftertouch_message_midi_event(uint64_t ticks, uint8_t midi_channel,
                    uint8_t value)
                : message_midi_event(ticks, midi_channel),
                value(value) {}
            std::vector<uint8_t> event_data() const override;
        private:
            uint8_t value;
    };

    class pitch_bend_message_midi_event : public message_midi_event {
        public:
            pitch_bend_message_midi_event(uint64_t ticks, uint8_t midi_channel,
                    int16_t pitch)
                : message_midi_event(ticks, midi_channel),
                pitch(pitch) {}
            std::vector<uint8_t> event_data() const override;
        private:
            int16_t pitch;
    };

    //=========================================================================

    class meta_midi_event : public midi_event {
        protected:
            meta_midi_event(uint64_t ticks)
                : midi_event(ticks) {}
    };

    //=====

    class sequencenumber_meta_midi_event : public meta_midi_event {
        public:
            sequencenumber_meta_midi_event(uint64_t ticks, uint16_t seq_num)
                : meta_midi_event(ticks), seq_num(seq_num), empty(false) {}
            sequencenumber_meta_midi_event(uint64_t ticks)
                : meta_midi_event(ticks), seq_num(0), empty(true) {}
            std::vector<uint8_t> event_data() const override;
        private:
            uint16_t seq_num;
            bool empty;
    };

    class text_meta_midi_event : public meta_midi_event {
        public:
            text_meta_midi_event(uint64_t ticks, const std::string& text)
                : meta_midi_event(ticks), text(text) {}
            std::vector<uint8_t> event_data() const override;
        private:
            std::string text;
    };

    class copyright_meta_midi_event : public meta_midi_event {
        public:
            copyright_meta_midi_event(uint64_t ticks, const std::string& text)
                : meta_midi_event(ticks), text(text) {}
            std::vector<uint8_t> event_data() const override;
        private:
            std::string text;
    };

    class trackname_meta_midi_event : public meta_midi_event {
        public:
            trackname_meta_midi_event(uint64_t ticks, const std::string& text)
                : meta_midi_event(ticks), text(text) {}
            std::vector<uint8_t> event_data() const override;
        private:
            std::string text;
    };

    class instrument_meta_midi_event : public meta_midi_event {
        public:
            instrument_meta_midi_event(uint64_t ticks, const std::string& text)
                : meta_midi_event(ticks), text(text) {}
            std::vector<uint8_t> event_data() const override;
        private:
            std::string text;
    };

    class lyric_meta_midi_event : public meta_midi_event {
        public:
            lyric_meta_midi_event(uint64_t ticks, const std::string& text)
                : meta_midi_event(ticks), text(text) {}
            std::vector<uint8_t> event_data() const override;
        private:
            std::string text;
    };

    class marker_meta_midi_event : public meta_midi_event {
        public:
            marker_meta_midi_event(uint64_t ticks, const std::string& text)
                : meta_midi_event(ticks), text(text) {}
            std::vector<uint8_t> event_data() const override;
        private:
            std::string text;
    };

    class cuepoint_meta_midi_event : public meta_midi_event {
        public:
            cuepoint_meta_midi_event(uint64_t ticks, const std::string& text)
                : meta_midi_event(ticks), text(text) {}
            std::vector<uint8_t> event_data() const override;
        private:
            std::string text;
    };

    class channelprefix_meta_midi_event : public meta_midi_event {
        public:
            channelprefix_meta_midi_event(uint64_t ticks, uint8_t channel)
                : meta_midi_event(ticks), channel(channel) {}
            std::vector<uint8_t> event_data() const override;
        private:
            uint8_t channel;
    };

    class endoftrack_meta_midi_event : public meta_midi_event {
        public:
            endoftrack_meta_midi_event(uint64_t ticks)
                : meta_midi_event(ticks) {}
            std::vector<uint8_t> event_data() const override;
    };

    class tempo_meta_midi_event : public meta_midi_event {
        public:
            tempo_meta_midi_event(uint64_t ticks, uint32_t us_per_beat)
                : meta_midi_event(ticks), us_per_beat(us_per_beat) {}
            tempo_meta_midi_event(uint64_t ticks, double bpm)
                : meta_midi_event(ticks),
                us_per_beat(static_cast<uint64_t>(1000000.0 * 60.0 / bpm)) {}
            std::vector<uint8_t> event_data() const override;
        private:
            uint32_t us_per_beat;
    };

    class smpteoffset_meta_midi_event : public meta_midi_event {
        public:
            smpteoffset_meta_midi_event(uint64_t ticks, uint8_t frame_rate,
                    uint8_t hour, uint8_t minute, uint8_t second,
                    uint8_t frames, uint8_t frame_fractions)
                : meta_midi_event(ticks), frame_rate(frame_rate),
                hour(hour), minute(minute), second(second),
                frames(frames), frame_fractions(frame_fractions) {}
            std::vector<uint8_t> event_data() const override;
        private:
            uint8_t frame_rate;
            uint8_t hour, minute, second;
            uint8_t frames, frame_fractions;
    };

    class timesignature_meta_midi_event : public meta_midi_event {
        public:
            timesignature_meta_midi_event(uint64_t ticks,
                    uint8_t numerator, uint8_t denominator,
                    uint8_t tick_clocks, uint8_t n32n)
                : meta_midi_event(ticks),
                numerator(numerator), denominator(denominator),
                tick_clocks(tick_clocks), n32n(n32n) {}
            std::vector<uint8_t> event_data() const override;
        private:
            uint8_t numerator, denominator;
            uint8_t tick_clocks, n32n;
    };

    class keysignature_meta_midi_event : public meta_midi_event {
        public:
            keysignature_meta_midi_event(uint64_t ticks, int8_t sharp_flats,
                    bool _minor)
                : meta_midi_event(ticks), sharp_flats(sharp_flats), _minor(_minor) {}
            std::vector<uint8_t> event_data() const override;
        private:
            int8_t sharp_flats;
            bool _minor;
    };

    class sequencerspecific_meta_midi_event : public meta_midi_event {
        public:
            sequencerspecific_meta_midi_event(uint64_t ticks,
                    const std::vector<uint8_t>& data)
                : meta_midi_event(ticks), data(data) {}
            std::vector<uint8_t> event_data() const override;
        private:
            std::vector<uint8_t> data;
    };

    //=========================================================================

    class sysex_midi_event : public midi_event {
        public:
            sysex_midi_event(uint64_t ticks, const std::vector<uint8_t>& data,
                    bool first_chunk, bool last_chunk)
                : midi_event(ticks), data(data), first_chunk(first_chunk),
                last_chunk(last_chunk) {}
            std::vector<uint8_t> event_data() const override;
        private:
            std::vector<uint8_t> data;
            bool first_chunk, last_chunk;
    };

    class escape_midi_event : public midi_event {
        public:
            escape_midi_event(uint64_t ticks, const std::vector<uint8_t>& data)
                : midi_event(ticks), data(data) {}
            std::vector<uint8_t> event_data() const override;
        private:
            std::vector<uint8_t> data;
    };
}

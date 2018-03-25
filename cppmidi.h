#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace cppmidi {
    struct midi_file {
        uint16_t time_division;
        std::vector<midi_track> midi_tracks;

        midi_file() : time_division(0) {}

        void load_from_file(std::string file_path);
        void save_to_file(std::string file_path);
        void sort_track_events();
    };

    struct midi_track {
        std::vector<midi_event &> midi_events;
    };

    class midi_event {
        public:
            uint64_t absolute_ticks() {
                return ticks;
            }
            std::vector<uint8_t> event_data() = 0;
        protected:
            midi_event(uint64_t ticks) : ticks(ticks) {}

            uint64_t ticks;
    };

    class message_midi_event : public midi_event {
        protected:
            message_midi_event(uint8_t midi_channel) : midi_channel(midi_channel) {}

            uint8_t midi_channel;
    };

    class noteoff_message_midi_event : public message_midi_event {
        public:
            noteoff_message_midi_event(uint64_t ticks, uint8_t midi_channel,
                    uint8_t key, uint8_t velocity);
            std::vector<uint8_t event_data();
        private:
            uint8_t key, velocity;
    };

    class noteon_message_midi_event : public message_midi_event {
        public:
            noteon_message_midi_event(uint64_t ticks, uint8_t midi_channel,
                    uint8_t key, uint8_t velocity);
            std::vector<uint8_t> event_data();
        private:
            uint8_t key, velocity;
    };

    class noteaftertouch_message_midi_event : public message_midi_event {
        public:
            noteaftertouch_message_midi_event(uint64_t ticks, uint8_t midi_channel,
                    uint8_t key, uint8_t value);
            std::vector<uint8_t> event_data();
        private:
            uint8_t key, value;
    };
}

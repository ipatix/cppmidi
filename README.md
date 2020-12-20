# cppmidi
## Overview
This is a library to parse and save MIDI files. It's supposed to be a lightweight straight forward and easy to use library.
The goal is oo provide a very basic MIDI parsing/saving library without a lot of features with the intention to make it very easy to use
and without having to dig through pages of documentation.

You can find example programs in `examples` which may be helpful if you have not used this library before.

THIS LIBRARY CANNOT DO MIDI PORT I/O. It's useful only for parsing and saving MIDI files.

## Features

- Loading Type 0 and 1 files (type 2 not supported)
- Saving Type 1 files
- Polymorphic representation of all valid event types (`cppmidi::midi_event`)
- Quickly iterating over specific event types with the `cppmidi::visitor`

## How to compile / integrate

The library consists of only two files: `cppmidi.cpp` and `cppmidi.h`. You can simply copy those files into your project to use them.
For now, I do not provide a Makefile or script to compile a shared library, but in the case you need it, it should be easy to do.

c++17 or higher is required to use this library. If you absolutely need something older, you may have to replace `std::filesystem::path` with `std::string`.

If you want to update the library, it's probably the easiest to integrate this repository as submodule into your main repository.
That way you can always update to a newer commit hash, should you need latest bug fixes and features.

## How to use

This library is written to be easy to use. MIDI files are represented in memory using the class `cppmidi::midi_file`.
Instantiating one is as easy as this:

```cpp
#include "cppmidi.h"

int main() {
    /* All types are contained in the namespace 'cppmidi' */
    cppmidi::midi_file mf;
}
```

Okay, let's do something more sophisticated. A `cppmidi::midi_file` holds a `std::vector<cppmidi::midi_track>` called `midi_tracks`.
Let's add two tracks to our MIDI file:

```cpp
int main() {
    cppmidi::midi_file mf;

    /* Add track #0 */
    mf.midi_tracks.emplace_back();

    /* Add track #1 */
    mf.midi_tracks.emplace_back();
}
```

Before we add any events we should make sure to use the correct time division. The default is 48 ticks per quarter note.
In this example we'll set it to 96. It does not actually matter at which point you set the time division, however, it should
be consistent with the events you add to the MIDI file.

```cpp
int main() {
    cppmidi::midi_file mf;

    mf.midi_tracks.emplace_back();
    mf.midi_tracks.emplace_back();

    /* You can set the time divison before or after adding tracks. */
    mf.time_division = 96;
}
```

A `cppmidi::midi_track` holds a `std::vector<std::unique_ptr<cppmidi::midi_event>>` called `midi_events`.
Okay, pretty boring so far, let's play some notes. MIDI notes have a key and a velocity value ranging from 0 to 127.
The middle C on the keyboard has key 60. Timing in a MIDI file is specified in ticks. Let's play a C Major chord on track #0:

```cpp
int main() {
    cppmidi::midi_file mf;

    mf.time_division = 96;

    mf.midi_tracks.emplace_back();
    mf.midi_tracks.emplace_back();

    /* We can either use `new` or `std::make_unique` to add events */
    mf.midi_tracks[0].midi_events.emplace_back(
        new cppmidi::noteon_message_midi_event(
            0,      // time: place note at tick #0
            0,      // MIDI-channel: play on channel #0
            60,     // MIDI-key: play a C
            127     // velocity: play at max velocity
        )
    );

    /* Play E */
    mf.midi_tracks[0].midi_events.emplace_back(
        new cppmidi::noteon_message_midi_event(0, 0, 64, 127)
    );

    /* Play G */
    mf.midi_tracks[0].midi_events.emplace_back(
        new cppmidi::noteon_message_midi_event(0, 0, 67, 127)
    );

    /* Let's turn off notes after 1 half note (192 ticks) */

    /* Stop C */
    mf.midi_tracks[0].midi_events.emplace_back(
        new cppmidi::noteoff_message_midi_event(
            192,    // time: place note at tick #192
            0,      // MIDI-channel: stop on channel #0
            60,     // MIDI-key: stop C
            127     // velocity: release the key at fastest speed
        )
    );

    /* Stop E */
    mf.midi_tracks[0].midi_events.emplace_back(
        new cppmidi::noteoff_message_midi_event(192, 0, 64, 127)
    );

    /* Stop G */
    mf.midi_tracks[0].midi_events.emplace_back(
        new cppmidi::noteoff_message_midi_event(192, 0, 67, 127)
    );
}
```

So far, we've only considered objects in memory. We can write the object to disk by calling `cppmidi::midi_file::save_to_file()`:

```cpp
int main() {
    cppmidi::midi_file mf;

    mf.time_division = 96;

    mf.midi_tracks.emplace_back();
    mf.midi_tracks.emplace_back();

    /* ... edit tracks like above ... */

    /* Now save the generated MIDI to disk */
    mf.save_to_file("my_file_name.mid");
}
```

You may have noticed that events can be inserted into the event list indepently from the tick you have specified when creating the event.
Before saving a file, you should make sure that events are sorted by time from beginning to end (ascending).
This is not a concern when events are guaranteed to be in the correct order, however, should that not be the case you should call
`cppmidi::midi_file::sort_track_events()` (stable sort is performed):

```cpp
int main() {
    cppmidi::midi_file mf;

    mf.time_division = 96;

    mf.midi_tracks.emplace_back();
    mf.midi_tracks.emplace_back();

    /* This event is placed at tick #192 */
    mf.midi_tracks[0].midi_events.emplace_back(
        new cppmidi::noteon_message_midi_event(192, 0, 60, 127)
    );

    /* This event is placed at tick #0 */
    mf.midi_tracks[0].midi_events.emplace_back(
        new cppmidi::noteoff_message_midi_event(0, 0, 60, 127)
    );

    /* We did not place events in order, so sort them before saving to disk */
    mf.sort_track_events();

    mf.save_to_file("my_file_name.mid");
}
```

Obviously, files can also be loaded, edited/analyzed and be written back to disk:

```cpp
int main() {
    cppmidi::midi_file mf;

    mf.load_from_file("my_file_name.mid");

    std::cout << "Number of events on track #0: " << mf[0].midi_events.size() << std::endl;
}
```

This is a full list of events available events:

```
dummy_midi_event
noteoff_message_midi_event
noteon_message_midi_event
noteaftertouch_message_midi_event
controller_message_midi_event
program_message_midi_event
channelaftertouch_message_midi_event
pitchbend_message_midi_event
sequencenumber_meta_midi_event
text_meta_midi_event
copyright_meta_midi_event
trackname_meta_midi_event
instrument_meta_midi_event
lyric_meta_midi_event
marker_meta_midi_event
cuepoint_meta_midi_event
programname_meta_midi_event
devicename_meta_midi_event
channelprefix_meta_midi_event
midiport_meta_midi_event
endoftrack_meta_midi_event
tempo_meta_midi_event
smpteoffset_meta_midi_event
timesignature_meta_midi_event
keysignature_meta_midi_event
sequencerspecific_meta_midi_event
sysex_midi_event
escape_midi_event
```

Check out `cppmidi.h` to get the signatured of the constructors and the available members.

## License

This program is licensed under the MIT license. See LICENSE for more information.

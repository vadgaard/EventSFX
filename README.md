# EventSFX

EventSFX is a BakkesMod plugin that plays configurable sound effects on
specific events. Individual events can be toggled and adjusted for volume
and delay. Further, you can set it to use any output device, which might
be nice for streamers or people who want to route the audio to e.g. Discord.

## Supported Events

Currently, the plugin allows for custom sounds on the following events:
* Bumps (3D)
* Demolitions (3D)
* Crossbar/goal post hits (3D)
* Goals
* Saves
* Assists
* Team goals
* Team wins
* Team losses
* Concedes

## Spatial Audio

Bumps, demolitionss, and crossbar/goal post hits will play the given sound
spatially (i.e. in 3D) relative to the camera. For the immersion. Further,
there's an option to have to volume of the crossbar/goal post hit to match
the speed at which the ball hits.

## Console Commands

The plugin provides a bunch of console commands. Some interesting ones are:
* `eventsfx_play <filename> [volume]`: Plays the provided sound file.
* `eventsfx_play_<event> [volume]`: Plays the sound associated with the given event.
* `eventsfx_set_volume <volume>`: Sets the plugin master volume.
* `eventsfx_set_volume_<event> <volume>`: Sets the submix volume for the given event.
* Etc.

These console commands can be bound to any key (or combination of keys, if
you're using [Custom Bindings](https://bakkesplugins.com/plugins/view/228)).
As such, EventSFX can be used as a soundboard if so desired.

See the guide in the plugin settings in Rocket League for more in-depth information.

# ArduinoEurorack
Modules and schematics for modular synthesizers using low voltage electronics and the ATMEGA328P

MIDI2VC is a controller arduino file for a MIDI to CV and CV to MIDI module. it's got three ins and three outs for pitch and gate, one TRS jack for MIDI in and another for MIDI out.

CS1Ther is a theremin style CV source, the arduino library CapacitiveSensor is used to sense proximity to an antenna. That technique has not great resolution or stability, so it's a wonky little sensor. Output a PWM through a RC LPF to make CV. 

TM2 is a sequencer like the Music Thing Modular Turing Machine or the 2hp's TM.  It's a randomizer implementing two channels of triggered CV. A mode switch flips between a static memory table and the turing mode lets the user have constant sequences or evolution. 

EuclidSeq is a pattern generator taking triggers and outputting triggers on rythmic beats. Implements two channels with Length, Fill, and Rotate knobs, each with a dual output. 


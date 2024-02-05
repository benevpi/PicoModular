import array
import time
import math

import digitalio

import board
import usb_midi
import adafruit_midi
from adafruit_midi.midi_message     import note_parser
from adafruit_midi.note_on          import NoteOn
from adafruit_midi.note_off         import NoteOff
from adafruit_midi.control_change   import ControlChange
from adafruit_midi.pitch_bend       import PitchBend

import busio
import adafruit_mcp4725

gate = digitalio.DigitalInOut(board.GP15)
gate.direction = digitalio.Direction.OUTPUT
gate.value = False

i2c = busio.I2C(board.GP1, board.GP0)
dac = adafruit_mcp4725.MCP4725(i2c)
dac.value = 32767

midi_channel = 1
midi = adafruit_midi.MIDI(midi_in=usb_midi.ports[0],
                          in_channel=midi_channel-1)

#note, 12 bit DAC
#one volt per octave? that makes this bit pretty simple.
#one volt is 12 bits / 3.3
#is it better to 'tune' this? Just have a simple thing that gradually cranks up the voltage and outputs the response. I can then adjust this along with the VCO.
# let's do this.

basenote = 60 #g1 -- it's about this naturally, but I'll need to tune the VCO
max_val = 65535 #the module takes a 16 bit number even though it's a 12 bit dac


octave_size = int(65535/3.3)
notes = []

current_val = 0
inc = int(octave_size/12)
for i in range(int (3.3*12)):
    notes.append(int(i*inc))

def note_to_dac_val(note):
    if note < basenote: return 0
    if note >= basenote+(int(3.3*12)): return max_val
    return notes[note-basenote]
    
last_note = 0

#has to include pitch bends, but really, need to tune it for this to make sense.
while True:
    msg = midi.receive()
    if isinstance(msg, NoteOn) and msg.velocity != 0:
        print("on: ", msg.note)
        print("value: ", note_to_dac_val(msg.note))
        dac.value = note_to_dac_val(msg.note)
        gate.value = True
        last_note = msg.note

    elif (isinstance(msg, NoteOff) or
          isinstance(msg, NoteOn) and msg.velocity == 0):
        if(msg.note == last_note):
            gate.value = False
            print( "off: ",  msg.note)
        else:
            print("not last note off")
    
    elif (isinstance(msg, PitchBend)):
        #Not -- this is not correct, needs tuning
        dac.value = note_to_dac_val(last_note-basenote) + int((msg.pitch_bend/8192)*(octave_size/12))
        print("bending")
        
    elif (msg is not None):
        print(msg)
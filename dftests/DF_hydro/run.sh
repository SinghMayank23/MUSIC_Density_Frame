#!/bin/bash

cp ../../build/src/MUSIChydro .
cp -r ../../EOS .
cp ../../example_inputfiles/IPGlasma_2D/input/epsilon-u-Hydro-t0.6-0.dat .

./MUSIChydro music_input_mode_2
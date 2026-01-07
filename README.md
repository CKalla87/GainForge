# GAINFORGE

A Mesa Boogie Triple Rectifier guitar amp emulator plugin built with JUCE.

## Description

GAINFORGE is an authentic emulation of the Mesa Boogie Triple Rectifier, one of the most iconic high-gain tube amplifiers for metal. It features:

- **Multiple Cascading Gain Stages**: Four preamp stages for authentic high-gain saturation
- **Rectifier Modes**: Switch between Silicon Diode (tighter, more immediate) and Tube Rectifier (saggy, compressed)
- **Authentic Tone Stack**: Bass (80Hz), Mid (750Hz), Treble (2500Hz), and Presence (5500Hz) controls matching the Triple Rectifier
- **Drive Control**: Additional saturation control for fine-tuning the distortion character
- **Master Volume**: Output level control

## Features

- **Mesa Boogie Triple Rectifier Emulation**: Authentic modeling of the iconic high-gain amp
- **Multiple Preamp Stages**: Four cascading gain stages for realistic harmonic content
- **Rectifier Sag Simulation**: Tube rectifier mode includes voltage sag for authentic feel
- **Accurate Tone Stack**: Frequencies and Q values matched to the Triple Rectifier
- **Asymmetric Saturation**: Tube-style asymmetric clipping for authentic character
- **Smooth Parameter Interpolation**: Artifact-free sound with smooth parameter changes
- **Metal-themed User Interface**: Industrial design matching the amp's aesthetic
- **Stereo Processing**: Full stereo support

## Building

1. Open `GAINFORGE.jucer` in Projucer
2. Configure your JUCE modules path (currently pointing to `../NebulaEQ/JUCE/modules`)
3. Export to your preferred IDE (Xcode, Visual Studio, etc.)
4. Build the project

## Parameters

- **Gain**: 0-100% - Controls the preamp gain (0.2x to 15x range)
- **Bass**: 0-100% - Low frequency control (80Hz low shelf, 0.2x to 2.5x)
- **Mid**: 0-100% - Mid frequency control (750Hz peak, 0.15x to 2.2x, wider Q for scooped mids)
- **Treble**: 0-100% - High frequency control (2500Hz high shelf, 0.2x to 2.5x)
- **Presence**: 0-100% - Ultra-high frequency control (5500Hz high shelf, affects articulation)
- **Master**: 0-100% - Output volume control (0.15x to 12x)
- **Drive**: 0-100% - Additional saturation control
- **Rectifier Mode**: Toggle between Silicon Diode (tighter) and Tube Rectifier (saggy)

## Recommended Settings for Metal

Based on Mesa Boogie Triple Rectifier settings:

- **Gain**: 60-80% (for high-gain metal tones)
- **Bass**: 65% (tight low end)
- **Mid**: 25% (scooped mids for modern metal)
- **Treble**: 60% (bright articulation)
- **Presence**: 60% (high-end clarity)
- **Master**: 40% (adjust to taste)
- **Drive**: 30-50% (additional saturation)
- **Rectifier Mode**: Silicon (for tight, modern metal) or Tube (for vintage sag)

## License

Copyright 2025 CK Audio Design


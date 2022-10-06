# Overview

```{tableofcontents}
```

```{raw} html
:file: ../images/design_schematic.svg
```

The design goal for pmacFilterControl is to automatically adjust the attenuation of a
filter set based on the exposure on a detector. This is done in four parts:

1. Detector DAQ Pipeline calculates 4-bin histogram and publishes results over ZeroMQ

2. pmacFilterControl app on PowerBrick receives these events and determines whether to
adjust the filter set. If so, it sets the new demands and runs the motion program.

3. Motion program performs two sequential moves to postitions set by pmacFilterControl

4. Motion PLC monitors state through P variables and provides GPIO signals to display
safety of system.

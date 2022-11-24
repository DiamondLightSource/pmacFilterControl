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


## Modes of Operation

The system can be operated in various modes for different use cases.

### 1. Automatic Attenuation

The primary mode of operation that the system was designed for. The data stream should
be monitored and the motor positions updated continuously to best effort. If no messages
are received on the data stream to notify of the required attenuation level, the maximum
attenuation level should be set and a timeout error will be raised.

```{raw} html
:file: ../images/pfc-state-flow-diagram-continuous-mode.svg
```

### 2. Single-shot & Reset

In this mode the system will start at max attenuation and then lower the attenuation
until it stablises and then hold this level. In this case, the stream of data messages
will stop and maximum attenuation level should not be set. This is to allow software
time for higher level software to perform step scans, which may take a few seconds.
Once the scan has been completed, the system can be told to start another Single-shot
run. If no request to start a new Single-shot run in recieved in time, a timeout error
will be raised and max attenuation will be set.

```{raw} html
:file: ../images/pfc-state-flow-diagram-singleshot-mode.svg
```

### 3. Manual Mode

Automatic adjustment of filters is completely disabled allowing manual control of each
filter. The system must not try to change the filter positions based on the data stream
and it should not set max attenuation after a timeout.

```{raw} html
:file: ../images/pfc-state-flow-diagram-manual-mode.svg
```

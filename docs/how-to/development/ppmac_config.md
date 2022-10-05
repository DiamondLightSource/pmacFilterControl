# PowerPMAC Configuration

## Motion Parameters

The following parameters should be set for the motors to move quickly:

- `Motor[x].JogTa = 1` (ms)
- `Motor[x].JogTs = 0` (ms)
- `Motor[x].JogSpeed = 80` (cts/ms)

TBC if we use different variables to avoid conflicts with the EPICS IOC overwriting them.

# Software Interface

This page describes the software interface of the pmacFilterControl application.

## Control Channel

A ZMQ control channel is exposed on the port given in the CLI parameters. The requests
that can be sent to this channel are detailed below.

### Commands

All messages must have a `command` key. Valid commands are:

|       Command | Description                                                                                         |
| ------------: | :-------------------------------------------------------------------------------------------------- |
|      shutdown | Shutdown the application                                                                            |
|        status | Request a status message                                                                            |
|     configure | Configure parameters                                                                                |
|         reset | Reset the `last_*_frame` counters - this is required to begin processing frame numbers from 0 again |
| clear_timeout | Clear the `TIMEOUT` state and change into the `WAITING` state                                       |
|    singleshot | Request a singleshot run to start - must be in `SINGLESHOT` mode and `WAITING` state                |

For example:

```json
{"command": "shutdown"}
```

### Config

The configuration requests are exposed via the `config` command request. Config
requests must contain a `params` key and its value must be a dictionary with at least
one of the following config options:

|                 Config | Description                                                                                                                            |
| ---------------------: | :------------------------------------------------------------------------------------------------------------------------------------- |
|                   mode | Set the operational mode - 0: Disable, 1: Continuous, 2: Singleshot                                                                    |
|           in_positions | In positions (counts) for each filter - allowed keys: `"filter{1,2,3,4}"`                                                              |
|          out_positions | Out positions (counts) for each filter - allowed keys: `"filter{1,2,3,4}"`                                                             |
| pixel_count_thresholds | Thresholds for each histogram bin above which attenuation should be changed - allowed keys: threshold names, e.g.: `"low1"`, `"high2"` |

For example:

```json
{"command": "configure", "params": {"mode": 0}}
```

### Status

A dictionary of the current status can be requested with the `status` command. The
following status items are included in response:

|                  Status | Description                                                                                       |
| ----------------------: | :------------------------------------------------------------------------------------------------ |
|                 version | The version number of the application                                                             |
|        process_duration | The duration of the most recent process that changed the attenuation (us)                         |
|          process_period | The elapsed time between the most recent process and the preceding one (us)                       |
|     last_received_frame | The frame number of the most recently received data message                                       |
|    last_processed_frame | The frame number of the most recently received data message that caused the attenuation to change |
| time_since_last_message | The elapsed time since a message was last received                                                |
|     current_attenuation | The currently demanded attenuation level                                                          |
|                   state | The current state - 0: Idle, 1: Waiting, 2: Active, 3: Timeout, 4: Singleshot Complete            |

For example:

```json
{"command": "status"}
```

Readbacks for [config items](#config) are included in the response with the same keys as
in the config command request.

### Timeout State

If frames are not received for 3 seconds while in the `ACTIVE` state then the `TIMEOUT`
state is triggered. When this happens maximum attenuation is set and any further data
messages are ignored. The `clear_timeout` command must be sent, which will clear the
error and change the state to `WAITING`.

This logic is a failsafe to minimise the time that attenuation is kept below the maximum
without receiving data messages to continually confirm that the attenuation is safe.

### Singleshot Mode

If the system is put into `SINGLESHOT` mode, maximum attenuation is set and the
`WAITING` state is entered until data messages are received on the subscribe channel.
Once data messages start, the system adjusts attenuation as normal until a message is
received that does not cause an adjustment. At this point the attenuation level is
considered stable and the `SINGLESHOT_COMPELETE` state is entered, which pauses the
adjustment at the current attenuation level without timing out.

This mode allows higher level software to use the automatic attenuation to optimise the
attenuation level and then capture a single optimal image at that attenuation.

## Data Channel

The application will subscribe on the endpoints given in the CLI parameters. Messages of
the following form are expected on these channels:

```json
{
    "frame_number": 0,
    "parameters": {
        "low2": 4,
        "low1": 10,
        "high1": 17,
        "high2": 3,
        "high3": 1
    }
}
```

The `pixel_count_thresholds` for each of these bins determine the action taken for
received data messages. The low thresholds are triggered if there are not enough pixels
above the given value, while the high thresholds are triggered if there are too many.

Any messages with a `frame_number` less than `last_processed_frame` will be ignored -
because it is too late to correct for them - as will frames equal to
`last_processed_frame + 1` - because the change from the previous message will not have
taken effect yet and adjusting for the subsequent message would be correcting a second
time for the same event.

## Event Stream Channel

For each successfully processed data message, an event message will be published on the
event stream channel. The channel will be bound to the publish port given in the CLI
parameters. The messages will be of the form (where frame 6 caused an adjustment of -1):

```json
{
    "frame_number": 6,
    "adjustment": 0,
    "attenuation": 5
}
{
    "frame_number": 7,
    "adjustment": -1,
    "attenuation": 4
}
```

It is intended for a higher-level application to subscribe to these events in order to
record the the active attenuation level for each frame and whether the adjustment
triggered. The `adjustment` and `attenuation` of frame `N+1` will be that which
resulted from processing frame `N`. A non-zero adjustment for frame `N+1` means the
attenuation was not optimal for frame `N` and will be changed during the exposure of
frame `N+1`, which means data analysis needs to ignore the data from that frame because
it is undefined what the exposure was.

## Motion Program

The motion program simply commands two moves in quick succession, with a timer to
estimate how long the moves took. The first move will demand the filters 1, 2, 3, 4 to
the positions stored in `P407{1,2,3,4}` and for the second move `P408{1,2,3,4}`. These
values are written to by the pmacFilterControl application before running the program.
The time elapsed is stored in `P4085`.

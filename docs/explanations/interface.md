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
|    singleshot | Request a singlshot run to start - must be in `SINGLESHOT` mode and `WAITING` state                 |

For example:

```json
{"command": "shutdown"}
```

### Config

The configuration requests are exposed via the `config` command request. Config
requests must contain a `params` key and its value must be a dictionary with at least
one ofwith with at least one config options:

|                 Config | Description                                                                 |
| ---------------------: | :-------------------------------------------------------------------------- |
|                   mode | Set the operational mode - 0: Disable, 1: Continuous, 2: Singleshot         |
|           in_positions | In positions (counts) for each filter - allowed keys: `"filter{1,2,3,4}"`   |
| pixel_count_thresholds | Thresholds for each histogram bin above which attenuation should be changed |

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

Readbacks for [config items](#config) are included in the same response with the same
keys as in the config command request.

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
        "high2": 3
    }
}
```

Any messages with a `frame_number` less than `last_processed_frame` will be ignored -
because it is too late to correct for them - as will frames equal to
`last_processed_frame + 1` - because the change from the previous message will not have
taken effect yet and adjusting for the subsequent message would be correcting a second
time for the same event.

## Motion Program

The motion program simply commands two moves in quick succession, with a timer to
estimate how long the moves took. The first move will demand the filters 1, 2, 3, 4 to
the positions stored in `P407{1,2,3,4}` and for the second move `P408{1,2,3,4}`. These
values are written to by the pmacFilterControl application before running the program.
The time elapsed is stored in `P4085`.

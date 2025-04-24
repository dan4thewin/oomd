# Changes to the fork branch

This branch adds new plugins and makes small changes to the oomd engine
to complement them.  The new plugins are: sleep, always_reclaim, interdict,
and run_command.  The engine now supports an exit registry where plugins
can add functions to run at oomd exit, and an always-continue flag to
always run a ruleset's actions which may behave differently when detectors
fail to match.

# Plugins

## sleep

### Arguments

    duration (in seconds)

### Description

This plugin allows an action to occur periodically at an interval greater
than the duration of the event loop, especially for use with `always_reclaim`.

CONTINUE if more than `duration` seconds have elapsed since the last
CONTINUE, otherwise STOP.

## always_reclaim

### Arguments

    cgroup
    reclaim_bytes

This plugin launches a reclamation of `reclaim_bytes` for the cgroup(s)
specified in `cgroup`.  Reclamation consists of a write to the
`memory.reclaim` file of the cgroup.  The plugin always returns CONTINUE.

### Description

## interdict

### Arguments

    cgroup
    memhigh_pct (from 1 to 99)

### Description

This plugin toggles a cgroup's `memory.high` limit between its current
value and a reduced value.  The intent is, under sufficient memory
contention, to encumber low-priority cgroups enough that the kernel
throttles further memory allocations.  Once the contention subsides,
the cgroup is unencumbered.  NB. the ruleset must specify
`"always-continue": true` for the plugin to operate correctly.

When all detectors return CONTINUE, this plugin writes a value to
the cgroup `memory.high` file that is `memhigh_pct` percentage of the
value in `memory.current` and saves the prior value from `memory.high`.
If a detector returns STOP, the plugin writes the saved value back to
`memory.high`.  The plugin always returns CONTINUE.  At oomd exit,
all saved values are written back.

## run_command

### Arguments

    command
    argument
    use_exit_value
    cache_sec
    timeout_msec

### Description

This plugin runs `command` with optional `argument` in which multiple
arguments may be specified by separating them with a tab (`\t`).
If `use_exit_value` is true, then the plugin returns CONTINUE for a
0 exit value and STOP otherwise.  If `use_exit_value` is false, the
default, then the plugin returns CONTINUE.

If `cache_sec`, default 0, is greater than 0 then the plugin returns
the value from the last run for the duration specified.  If `timeout_msec`,
default 500 milliseconds, is greater than 0, then a run exceeding the
timeout is killed.  With `use_exit_value` true, a killed run returns STOP.

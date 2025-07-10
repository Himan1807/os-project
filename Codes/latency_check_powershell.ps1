# Measure 10-pipe operation latency
Measure-Command {
    "test" | ForEach-Object { $_ } | ForEach-Object { $_ } | ForEach-Object { $_ } |
    ForEach-Object { $_ } | ForEach-Object { $_ } | ForEach-Object { $_ } |
    ForEach-Object { $_ } | ForEach-Object { $_ } | ForEach-Object { $_ } |
    ForEach-Object { $_ } > $null
}
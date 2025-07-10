# Math benchmark (2^30 repeated 1000 times)
$mathTime = Measure-Command { 
    for ($i=0; $i -lt 1000; $i++) { [math]::Pow(2, 30) } 
}
Write-Host "PowerShell math: $($mathTime.TotalMilliseconds) ms"

# Command overhead (equivalent to /bin/true)
$cmdTime = Measure-Command { 
    for ($i=0; $i -lt 1000; $i++) { cmd /c "echo > nul" } 
}
Write-Host "PowerShell command overhead: $($cmdTime.TotalSeconds) s"
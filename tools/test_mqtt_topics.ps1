param(
    [string]$BrokerHost = "127.0.0.1",
    [int]$Port = 1884
)

$mosquittoDir = "D:\Mosquitto"
$pub = Join-Path $mosquittoDir "mosquitto_pub.exe"
$sub = Join-Path $mosquittoDir "mosquitto_sub.exe"

if (-not (Test-Path -LiteralPath $pub) -or -not (Test-Path -LiteralPath $sub)) {
    throw "Mosquitto clients were not found in D:\Mosquitto"
}

$root = "smarthome/device01"

# 1. retained state should be delivered immediately to a new subscriber.
& $pub -h $BrokerHost -p $Port -t "$root/state" -r -m '{"mode":0,"light_on":1,"door_open":0}'
$state = & $sub -h $BrokerHost -p $Port -t "$root/state" -C 1 -W 2
if ($LASTEXITCODE -ne 0 -or $state -notmatch '"light_on":1') {
    throw "Retained state test failed"
}

# 2. a command published without retain must not remain for future subscribers.
& $pub -h $BrokerHost -p $Port -t "$root/command" -n -r
& $pub -h $BrokerHost -p $Port -t "$root/command" -m '{"device":1,"value":1}'
$command = & $sub -h $BrokerHost -p $Port -t "$root/command" -C 1 -W 1 2>$null
if ($LASTEXITCODE -eq 0 -or $command) {
    throw "Command was unexpectedly retained"
}

Write-Output "PASS: retained state is available and command is not retained."

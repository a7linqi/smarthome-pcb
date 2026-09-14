param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("auto", "manual", "pump-on", "pump-off", "temp", "soil")]
    [string]$Command,

    [int]$Value = 0,
    [string]$BrokerHost = "broker.emqx.io",
    [int]$Port = 1883
)

$mosquittoPub = "D:\Mosquitto\mosquitto_pub.exe"
$configPath = Join-Path $PSScriptRoot "..\firmware\esp8266\ESP8266_MQTT_Bridge\bridge_config.h"

if (-not (Test-Path -LiteralPath $mosquittoPub)) {
    throw "Mosquitto publisher was not found at D:\Mosquitto"
}
if (-not (Test-Path -LiteralPath $configPath)) {
    throw "Create bridge_config.h from bridge_config.example.h first"
}

$config = Get-Content -LiteralPath $configPath -Raw
$match = [regex]::Match($config, 'BRIDGE_TOPIC_NAMESPACE\s+"([^"]+)"')
if (-not $match.Success) {
    throw "BRIDGE_TOPIC_NAMESPACE is missing"
}

$message = switch ($Command) {
    "auto"     { "SET_MODE:ZD" }
    "manual"   { "SET_MODE:SD" }
    "pump-on"  { "Water_pump_ON" }
    "pump-off" { "Water_pump_OFF" }
    "temp"     { "SET_T_H:$Value" }
    "soil"     { "SET_H_L:$Value" }
}

$topic = "jiaohua/$($match.Groups[1].Value)/control"
& $mosquittoPub -h $BrokerHost -p $Port -t $topic -m $message
if ($LASTEXITCODE -ne 0) {
    throw "MQTT publish failed"
}

Write-Host "Published '$message' to $topic"

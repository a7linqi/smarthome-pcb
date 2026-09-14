param(
    [string]$BrokerHost = "broker.emqx.io",
    [int]$Port = 1883
)

$mosquittoSub = "D:\Mosquitto\mosquitto_sub.exe"
$configPath = Join-Path $PSScriptRoot "..\firmware\esp8266\ESP8266_MQTT_Bridge\bridge_config.h"

if (-not (Test-Path -LiteralPath $mosquittoSub)) {
    throw "Mosquitto subscriber was not found at D:\Mosquitto"
}
if (-not (Test-Path -LiteralPath $configPath)) {
    throw "Create bridge_config.h from bridge_config.example.h first"
}

$config = Get-Content -LiteralPath $configPath -Raw
$match = [regex]::Match($config, 'BRIDGE_TOPIC_NAMESPACE\s+"([^"]+)"')
if (-not $match.Success) {
    throw "BRIDGE_TOPIC_NAMESPACE is missing"
}

$topic = "jiaohua/$($match.Groups[1].Value)/#"
Write-Host "Monitoring $topic. Press Ctrl+C to stop."
& $mosquittoSub -h $BrokerHost -p $Port -t $topic -v

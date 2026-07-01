$body = '{"ssid":"HomeWiFi","password":"pass123","tenant_id":"sim_tenant"}'
$headers = @{"Content-Type" = "application/json"}

for ($i = 1; $i -le 5; $i++) {
    $port = 9000 + $i
    Write-Host "Provisioning sim_dev_000$i on port $port..."
    try {
        $res = Invoke-RestMethod -Uri "http://localhost:$port/provision" -Method POST -Body $body -Headers $headers -TimeoutSec 5
        Write-Host "  -> OK: $res"
    } catch {
        Write-Host "  -> FAILED: $_"
    }
}

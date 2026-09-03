async function refreshStatus() {
    try {
        const s = await api('/api/status');
        document.querySelector('#wifi-state').innerHTML = s.sta_connected
            ? '<span class="badge ok">Conectado</span>'
            : '<span class="badge off">Sin conexión</span>';
        document.querySelector('#sta-ssid').textContent = s.sta_ssid || '-';
        document.querySelector('#sta-ip').textContent = s.sta_ip;
        document.querySelector('#rssi').textContent = s.sta_connected ? `${s.rssi} dBm` : '-';
        document.querySelector('#ap-ip').textContent = s.ap_ip;
        document.querySelector('#uptime').textContent = formatUptime(s.uptime_s);
        document.querySelector('#heap').textContent = formatBytes(s.free_heap);
        document.querySelector('#flash').textContent = formatBytes(s.flash_bytes);
        document.querySelector('#psram').textContent = formatBytes(s.psram_bytes);
    } catch (error) {
        console.error(error);
    }
}

refreshStatus();
setInterval(refreshStatus, 5000);

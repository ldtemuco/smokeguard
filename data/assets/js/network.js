const scanButton = document.querySelector('#scan');
const connectForm = document.querySelector('#wifi-form');
const ssidSelect = document.querySelector('#ssid');
const scanMessage = document.querySelector('#scan-message');
const connectMessage = document.querySelector('#connect-message');

async function scanNetworks() {
    scanButton.disabled = true;
    scanMessage.textContent = 'Buscando redes...';
    ssidSelect.innerHTML = '<option value="">Buscando...</option>';

    try {
        const data = await api('/api/networks');
        ssidSelect.innerHTML = '<option value="">Seleccione una red</option>';

        const seen = new Set();
        data.networks
            .sort((a, b) => b.rssi - a.rssi)
            .forEach((network) => {
                if (!network.ssid || seen.has(network.ssid)) return;
                seen.add(network.ssid);
                const option = document.createElement('option');
                option.value = network.ssid;
                option.textContent = `${network.ssid} (${network.rssi} dBm)${network.secure ? ' 🔒' : ''}`;
                ssidSelect.appendChild(option);
            });

        scanMessage.textContent = `${seen.size} red(es) encontrada(s).`;
    } catch (error) {
        scanMessage.textContent = error.message;
    } finally {
        scanButton.disabled = false;
    }
}

connectForm.addEventListener('submit', async (event) => {
    event.preventDefault();
    connectMessage.className = 'muted';
    connectMessage.textContent = 'Intentando conexión...';
    const button = connectForm.querySelector('button[type="submit"]');
    button.disabled = true;

    try {
        const body = new URLSearchParams(new FormData(connectForm));
        const data = await api('/api/wifi', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body
        });
        connectMessage.className = 'success';
        connectMessage.textContent = `Conectado correctamente. IP: ${data.ip}`;
    } catch (error) {
        connectMessage.className = 'error';
        connectMessage.textContent = error.message;
    } finally {
        button.disabled = false;
    }
});

scanButton.addEventListener('click', scanNetworks);
scanNetworks();

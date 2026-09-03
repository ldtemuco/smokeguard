const passwordForm = document.querySelector('#password-form');
const passwordMessage = document.querySelector('#password-message');

passwordForm.addEventListener('submit', async (event) => {
    event.preventDefault();
    passwordMessage.textContent = '';

    const newPassword = passwordForm.new_password.value;
    const repeatPassword = passwordForm.repeat_password.value;
    if (newPassword !== repeatPassword) {
        passwordMessage.className = 'error';
        passwordMessage.textContent = 'Las nuevas contraseñas no coinciden.';
        return;
    }

    try {
        const body = new URLSearchParams();
        body.set('current_password', passwordForm.current_password.value);
        body.set('new_password', newPassword);
        await api('/api/password', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body
        });
        passwordMessage.className = 'success';
        passwordMessage.textContent = 'Contraseña actualizada.';
        passwordForm.reset();
    } catch (error) {
        passwordMessage.className = 'error';
        passwordMessage.textContent = error.message;
    }
});

async function postAction(url, question) {
    if (!confirm(question)) return;
    try {
        await api(url, { method: 'POST' });
        document.body.style.opacity = '.55';
    } catch (error) {
        alert(error.message);
    }
}

document.querySelector('#restart').addEventListener('click', () =>
    postAction('/api/restart', '¿Reiniciar el ESP32-S3?')
);

document.querySelector('#factory-reset').addEventListener('click', () =>
    postAction('/api/factory-reset', 'Esto eliminará Wi-Fi y contraseña del panel. ¿Continuar?')
);

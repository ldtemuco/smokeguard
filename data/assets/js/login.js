const form = document.querySelector('#login-form');
const message = document.querySelector('#message');

form.addEventListener('submit', async (event) => {
    event.preventDefault();
    message.textContent = '';

    const button = form.querySelector('button');
    button.disabled = true;

    try {
        const body = new URLSearchParams(new FormData(form));
        const response = await fetch('/api/login', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body
        });
        const data = await response.json();

        if (!response.ok) throw new Error(data.error || 'No fue posible iniciar sesión');
        window.location.href = '/dashboard';
    } catch (error) {
        message.textContent = error.message;
    } finally {
        button.disabled = false;
    }
});

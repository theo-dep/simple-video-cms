import { useEffect } from 'preact/hooks';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { user } from '../store/auth.js';

export default function Logout() {
  useTitle('Logout');

  useEffect(() => {
    async function cleanup() {
      if ('serviceWorker' in navigator) {
        const regs = await navigator.serviceWorker.getRegistrations();
        for (const reg of regs) await reg.unregister();
      }

      await api.logout();

      user.videos.value = [];
      user.isLogged.value = false;
      user.isAdmin.value = false;
      user.name.value = '';
      user.id.value = null;

      window.location.href = '/';
    }
    cleanup();
  }, []);

  return null;
}

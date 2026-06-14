import { useEffect } from 'preact/hooks';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { user } from '../store/auth.js';
import clearSW from '../clearsw.js';

export default function Logout() {
  useTitle('Logout');

  useEffect(() => {
    async function cleanup() {
      await clearSW();

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

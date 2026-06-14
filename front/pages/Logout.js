import { useEffect } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { user, refreshAuth } from '../store/auth.js';
import clearSW from '../clearsw.js';

export default function Logout() {
  const { route } = useLocation();

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

      await refreshAuth();
      route('/');
    }
    cleanup();
  }, []);

  return null;
}

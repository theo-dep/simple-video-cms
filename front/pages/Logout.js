import { useEffect } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { user, refreshRequested } from '../store/auth.js';
import { swReady, postToServiceWorker } from '../store/sw.js';

export default function Logout() {
  const { route } = useLocation();

  useTitle('Logout');

  useEffect(() => {
    api.logout().then((_response) => {
      user.videos.value = [];
      user.isLogged.value = false;
      user.isAdmin.value = false;
      user.name.value = '';
      user.id.value = null;

      if (swReady.value) {
        postToServiceWorker('disableVideoCaching');
      }

      refreshRequested.value = true; // update the user
      route('/');
    });
  }, []);

  return null;
}

import { signal, effect } from '@preact/signals';
import { api } from '../api.js';
import { apiOffline } from './offline.js';

const userId = signal(null);
const userName = signal('');
const isLogged = signal(false);
const isAdmin = signal(false);
const videos = signal([]);

export const refreshRequested = signal(true);
export const firstRefreshed = signal(false);
export const refreshed = signal(false);

export const user = {
  id: userId,
  name: userName,
  isLogged,
  isAdmin,
  videos,
};

async function refreshAuth() {
  refreshed.value = false;

  try {
    const { json } = await api.refresh();
    userId.value = json?.id ?? null;
    userName.value = json?.name ?? '';
    isLogged.value = !!json?.id;
    isAdmin.value = !!json?.isAdmin;
    videos.value = json?.videos ?? [];
  } catch {
    userId.value = null;
    userName.value = '';
    isLogged.value = false;
    isAdmin.value = false;
    videos.value = [];
  } finally {
    refreshRequested.value = false;
    firstRefreshed.value = true;
    refreshed.value = true;
  }
}

effect(() => {
  if (apiOffline.value) return;
  if (refreshRequested.value) refreshAuth();
});

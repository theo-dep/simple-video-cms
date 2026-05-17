import { signal } from '@preact/signals';
import { api } from '../api.js';

const userId = signal(null);
const userName = signal('');
const isLogged = signal(false);
const isAdmin = signal(false);
const videos = signal([]);

export const user = {
  id: userId,
  name: userName,
  isLogged,
  isAdmin,
  videos,
};

export async function refreshAuth() {
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
  }
}

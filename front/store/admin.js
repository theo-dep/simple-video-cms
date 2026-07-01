import { signal } from '@preact/signals';
import { navigate } from '../store/router.js';
import { api } from '../api.js';

export const stats = signal(null);

export const admins = signal(null);
export const groups = signal(null);
export const users = signal(null);
export const videos = signal(null);

export const selectedAdmin = signal(null);
export const selectedGroup = signal(null);
export const selectedUser = signal(null);
export const selectedVideo = signal(null);

const inFlightRequests = new Map();
const loadTokens = new Map();

async function load(target, apiCall, ...apiParam) {
  const key = `${apiCall.name}:${apiParam.join(',')}`;

  let request = inFlightRequests.get(key);
  if (!request) {
    request = apiCall(...apiParam);
    inFlightRequests.set(key, request);
    request.finally(() => {
      if (inFlightRequests.get(key) === request) inFlightRequests.delete(key);
    });
  }

  const token = (loadTokens.get(target) ?? 0) + 1;
  loadTokens.set(target, token);

  try {
    const r = await request;
    if (loadTokens.get(target) === token) target.value = r.json ?? r;
  } catch (err) {
    if (loadTokens.get(target) !== token) return; // superseded by a newer request
    if (err.status === 401 || err.status === 403) {
      navigate.value ? navigate.value('/403') : (window.location.href = '/403');
    } else console.error(err);
  }
}

export const loadStats = () => load(stats, api.adminStats);

export const loadAdmins = () => load(admins, api.adminAdminList);
export const loadGroups = () => load(groups, api.adminGroupList);
export const loadUsers = () => load(users, api.adminUserList);
export const loadVideos = () => load(videos, api.adminVideoList);

export const loadAdmin = (adminId) => load(selectedAdmin, api.adminAdmin, adminId);
export const loadGroup = (groupId) => load(selectedGroup, api.adminGroup, groupId);
export const loadUser = (userId) => load(selectedUser, api.adminUser, userId);
export const loadVideo = (videoId) => load(selectedVideo, api.adminVideo, videoId);

function invalidate(list) {
  list.value = null;
  stats.value = null;
}

export const invalidateAdmins = () => invalidate(admins);
export const invalidateGroups = () => invalidate(groups);
export const invalidateUsers = () => invalidate(users);
export const invalidateVideos = () => invalidate(videos);

import { signal } from '@preact/signals';
import { navigate } from '../store/router.js';
import { api } from '../api.js';

export const stats = signal(null);

export const admins = signal(null);
export const groups = signal(null);
export const users = signal(null);
export const videos = signal(null);

export const admin = signal(null);
export const group = signal(null);
export const user = signal(null);
export const video = signal(null);

async function load(target, apiCall, ...apiParam) {
  try {
    const r = await apiCall(...apiParam);
    target.value = r.json ?? r;
  } catch {
    navigate.value ? navigate.value('/403') : (window.location.href = '/403');
  }
}

export const loadStats = () => load(stats, api.adminStats);

export const loadAdmins = () => load(admins, api.adminAdminList);
export const loadGroups = () => load(groups, api.adminGroupList);
export const loadUsers = () => load(users, api.adminUserList);
export const loadVideos = () => load(videos, api.adminVideoList);

export const loadAdmin = (adminId) => load(admin, api.adminAdmin, adminId);
export const loadGroup = (groupId) => load(group, api.adminGroup, groupId);
export const loadUser = (userId) => load(user, api.adminUser, userId);
export const loadVideo = (videoId) => load(video, api.adminVideo, videoId);

export const invalidateAdmins = () => {
  admins.value = null;
  stats.value = null;
};
export const invalidateGroups = () => {
  groups.value = null;
  stats.value = null;
};
export const invalidateUsers = () => {
  users.value = null;
  stats.value = null;
};
export const invalidateVideos = () => {
  videos.value = null;
  stats.value = null;
};

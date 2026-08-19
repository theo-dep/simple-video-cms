import { signal } from '@preact/signals';
import { api } from '../api.js';

export const stats = signal(null);

export const admins = signal(null);
export const groups = signal(null);
export const users = signal(null);
export const videos = signal(null);

export const locations = signal(null);
export const authors = signal(null);
export const tags = signal(null);

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
    console.error(err);
  }
}

export const loadStats = () => load(stats, api.adminStats);

export const loadAdmins = () => load(admins, api.adminAdminList);
export const loadGroups = () => load(groups, api.adminGroupList);
export const loadUsers = () => load(users, api.adminUserList);
export const loadVideos = () => load(videos, api.adminVideoList);

export const loadLocations = () => load(locations, api.adminLocationList);
export const loadAuthors = () => load(authors, api.adminAuthorList);
export const loadTags = () => load(tags, api.adminTagList);

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

export function invalidateAdminLists() {
  users.value = null;
  groups.value = null;
  videos.value = null;
}

async function addValue(list, apiCall, value) {
  const { json } = await apiCall(value);
  if (!json) return null;
  const { id } = json;
  list.value = [...list.value, { id, name: value }];
  return id;
}

async function deleteValue(list, apiCall, id) {
  await apiCall(id);
  list.value = list.value.filter((l) => String(l.id) !== id);
}

export async function onAddedLocation(value) {
  return await addValue(locations, api.adminAddLocation, value);
}

export async function onDeletedLocation(id) {
  await deleteValue(locations, api.adminDeleteLocation, id);
}

export async function onAddedAuthor(value) {
  return await addValue(authors, api.adminAddAuthor, value);
}

export async function onDeletedAuthor(id) {
  await deleteValue(authors, api.adminDeleteAuthor, id);
}

export async function onAddedTag(value) {
  return await addValue(tags, api.adminAddTag, value);
}

export async function onDeletedTag(id) {
  await deleteValue(tags, api.adminDeleteTag, id);
}

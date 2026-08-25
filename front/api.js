const BASE = '/api';

function isNetworkError(err) {
  return err instanceof TypeError || !navigator.onLine;
}

async function fetchApiResponse(method, path, body = undefined) {
  let res;
  try {
    res = await fetch(BASE + path, {
      method,
      credentials: 'same-origin',
      ...(body !== undefined && { body: body }),
    });
  } catch (err) {
    if (isNetworkError(err)) {
      dispatchEvent(new CustomEvent('api-offline'));
    }
    throw err;
  }

  if (!res.ok) {
    const msg = await res.text().catch(() => res.statusText);
    const err = new Error(msg || `HTTP ${res.status}`);
    err.status = res.status;
    throw err;
  }
  return res;
}

async function fetchApi(method, path, body = undefined) {
  const res = await fetchApiResponse(method, path, body);
  const text = await res.text();
  return {
    status: res.status,
    json: text ? JSON.parse(text) : null,
  };
}

export const api = {
  logsPath: () => BASE + `/logs`,
  logs: (logData) => fetchApi('POST', '/logs', JSON.stringify(logData)),

  login: (username, password) =>
    fetchApi(
      'POST',
      '/login',
      new URLSearchParams({
        username: username,
        password: password,
      })
    ),
  refresh: () => fetchApi('GET', '/refresh'),
  logout: () => fetchApi('POST', '/logout'),

  addPassword: (username, password, confirmPassword) =>
    fetchApi(
      'POST',
      '/add-password',
      new URLSearchParams({
        username: username,
        password: password,
        confirmPassword: confirmPassword,
      })
    ),

  updateUsername: (username, password) =>
    fetchApi(
      'POST',
      '/update-username',
      new URLSearchParams({
        username: username,
        password: password,
      })
    ),
  updatePassword: (oldPassword, newPassword, confirmPassword) =>
    fetchApi(
      'POST',
      '/update-password',
      new URLSearchParams({
        oldPassword: oldPassword,
        newPassword: newPassword,
        confirmPassword: confirmPassword,
      })
    ),

  videoPlaylistPath: (videoId) => BASE + `/video/${videoId}/playlist`,
  adminDownloadVideoPath: (videoId) => BASE + `/admin/download-video/${videoId}`,
  thumbnailPath: (videoId) => BASE + `/thumbnail/${videoId}`,

  bookmark: (videoId, bookmarked) => fetchApi('POST', `/bookmark/${videoId}`, new URLSearchParams({ bookmarked })),

  addVideoSession: (videoId) => fetchApi('POST', `/add-video-session/${videoId}`),
  startVideoSession: (videoId) => fetchApi('POST', `/start-video-session/${videoId}`),
  resetVideoSession: (videoId) => fetchApi('POST', `/reset-video-session/${videoId}`),

  adminStats: () => fetchApi('GET', '/admin/stats'),

  adminVideoList: () => fetchApi('GET', '/admin/video-list'),
  adminVideo: (videoId) => fetchApi('GET', `/admin/video/${videoId}`),
  adminAddVideo: (video, title, date, locationId, authorIds, tagIds, groupIds, userIds) => {
    const form = new FormData();
    form.append('title', title);
    form.append('video', video);
    form.append('date', date);
    form.append('locationId', locationId);
    form.append('authorIds', authorIds);
    form.append('tagIds', tagIds);
    form.append('groupIds', groupIds);
    form.append('userIds', userIds);
    return fetchApi('POST', '/admin/add-video', form);
  },
  adminUpdateVideo: (videoId, title, date, locationId, authorIds, tagIds, groupIds, userIds) =>
    fetchApi(
      'POST',
      `/admin/update-video/${videoId}`,
      new URLSearchParams({
        title: title,
        date: date,
        locationId: locationId,
        authorIds: authorIds,
        tagIds: tagIds,
        groupIds: groupIds,
        userIds: userIds,
      })
    ),
  adminDeleteVideo: (videoId) => fetchApi('POST', `/admin/delete-video/${videoId}`),

  adminLocationList: () => fetchApi('GET', '/admin/location-list'),
  adminAddLocation: (name) =>
    fetchApi(
      'POST',
      '/admin/add-location',
      new URLSearchParams({
        name: name,
      })
    ),
  adminUpdateLocation: (locationId, name) =>
    fetchApi(
      'POST',
      `/admin/update-location/${locationId}`,
      new URLSearchParams({
        name: name,
      })
    ),
  adminDeleteLocation: (locationId) => fetchApi('POST', `/admin/delete-location/${locationId}`),

  adminAuthorList: () => fetchApi('GET', '/admin/author-list'),
  adminAddAuthor: (name) =>
    fetchApi(
      'POST',
      '/admin/add-author',
      new URLSearchParams({
        name: name,
      })
    ),
  adminUpdateAuthor: (authorId, name) =>
    fetchApi(
      'POST',
      `/admin/update-author/${authorId}`,
      new URLSearchParams({
        name: name,
      })
    ),
  adminDeleteAuthor: (authorId) => fetchApi('POST', `/admin/delete-author/${authorId}`),

  adminTagList: () => fetchApi('GET', '/admin/tag-list'),
  adminAddTag: (name) =>
    fetchApi(
      'POST',
      '/admin/add-tag',
      new URLSearchParams({
        name: name,
      })
    ),
  adminUpdateTag: (tagId, name) =>
    fetchApi(
      'POST',
      `/admin/update-tag/${tagId}`,
      new URLSearchParams({
        name: name,
      })
    ),
  adminDeleteTag: (tagId) => fetchApi('POST', `/admin/delete-tag/${tagId}`),

  adminAdminList: () => fetchApi('GET', '/admin/admin-list'),
  adminAdmin: (adminId) => fetchApi('GET', `/admin/admin/${adminId}`),
  adminAddAdmin: (username) =>
    fetchApi(
      'POST',
      '/admin/add-admin',
      new URLSearchParams({
        username: username,
      })
    ),
  adminUpdateAdmin: (adminId, username) =>
    fetchApi(
      'POST',
      `/admin/update-admin/${adminId}`,
      new URLSearchParams({
        username: username,
      })
    ),

  adminUserList: () => fetchApi('GET', '/admin/user-list'),
  adminUser: (userId) => fetchApi('GET', `/admin/user/${userId}`),
  adminAddUser: (username, groupIds, videoIds) =>
    fetchApi(
      'POST',
      '/admin/add-user',
      new URLSearchParams({
        username: username,
        groupIds: groupIds,
        videoIds: videoIds,
      })
    ),

  adminUpdateUser: (userId, username, groupIds, videoIds) =>
    fetchApi(
      'POST',
      `/admin/update-user/${userId}`,
      new URLSearchParams({
        username: username,
        groupIds: groupIds,
        videoIds: videoIds,
      })
    ),
  adminDeactivateUser: (userId, userDeactivated) =>
    fetchApi('POST', `/admin/deactivate-user/${userId}`, new URLSearchParams({ deactivated: userDeactivated })),
  adminResetUserPassword: (userId) => fetchApi('POST', `/admin/reset-user-password/${userId}`),
  adminDeleteUser: (userId) => fetchApi('POST', `/admin/delete-user/${userId}`),

  adminGroupList: () => fetchApi('GET', '/admin/group-list'),
  adminGroup: (groupId) => fetchApi('GET', `/admin/group/${groupId}`),
  adminAddGroup: (name, userIds, videoIds) =>
    fetchApi(
      'POST',
      '/admin/add-group',
      new URLSearchParams({
        name: name,
        userIds: userIds,
        videoIds: videoIds,
      })
    ),
  adminUpdateGroup: (groupId, name, userIds, videoIds) =>
    fetchApi(
      'POST',
      `/admin/update-group/${groupId}`,
      new URLSearchParams({
        name: name,
        userIds: userIds,
        videoIds: videoIds,
      })
    ),
  adminDeleteGroup: (groupId) => fetchApi('POST', `/admin/delete-group/${groupId}`),
};

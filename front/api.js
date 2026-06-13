const BASE = '/api';

async function fetchApi(method, path, body = undefined) {
  const res = await fetch(BASE + path, {
    method,
    credentials: 'same-origin',
    ...(body !== undefined && { body: body }),
  });
  if (!res.ok) {
    const msg = await res.text().catch(() => res.statusText);
    const err = new Error(msg || `HTTP ${res.status}`);
    err.status = res.status;
    throw err;
  }
  const text = await res.text();
  return {
    status: res.status,
    json: text ? JSON.parse(text) : null,
  };
}

export const api = {
  logs: (logData) => fetchApi('POST', '/logs', JSON.stringify(logData)),

  login: (username, password) => {
    return fetchApi(
      'POST',
      '/login',
      new URLSearchParams({
        username: username,
        password: password,
      })
    );
  },
  refresh: () => fetchApi('GET', '/refresh'),
  logout: () => fetchApi('POST', '/logout'),

  addPassword: (username, password, confirmPassword) => {
    return fetchApi(
      'POST',
      '/add-password',
      new URLSearchParams({
        username: username,
        password: password,
        confirmPassword: confirmPassword,
      })
    );
  },

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
  thumbnailPath: (videoId) => BASE + `/thumbnail/${videoId}`,
  adminDownloadVideoPath: (videoId) => BASE + `/admin/download-video/${videoId}`,

  addVideoSession: (videoId) => fetchApi('POST', `/add-video-session/${videoId}`),
  startVideoSession: (videoId) => fetchApi('POST', `/start-video-session/${videoId}`),
  resetVideoSession: (videoId) => fetchApi('POST', `/reset-video-session/${videoId}`),

  adminStats: () => fetchApi('GET', '/admin/stats'),

  adminVideoList: () => fetchApi('GET', '/admin/video-list'),
  adminVideo: (videoId) => fetchApi('GET', `/admin/video/${videoId}`),
  adminAddVideo: (video, title, groupIds, userIds) => {
    const form = new FormData();
    form.append('title', title);
    form.append('video', video);
    form.append('groupIds', groupIds);
    form.append('userIds', userIds);
    return fetchApi('POST', '/admin/add-video', form);
  },
  adminUpdateVideo: (videoId, title, groupIds, userIds) =>
    fetchApi(
      'POST',
      `/admin/update-video/${videoId}`,
      new URLSearchParams({
        title: title,
        groupIds: groupIds,
        userIds: userIds,
      })
    ),
  adminDeleteVideo: (videoId) => fetchApi('POST', `/admin/delete-video/${videoId}`),

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

  adminUserList: () => fetchApi('GET', '/admin/user-list'),
  adminUser: (userId) => fetchApi('GET', `/admin/user/${userId}`),
  adminAddUser: (username, groupIds) =>
    fetchApi(
      'POST',
      '/admin/add-user',
      new URLSearchParams({
        username: username,
        groupIds: groupIds,
      })
    ),

  adminUpdateUser: (userId, username, groupIds) =>
    fetchApi(
      'POST',
      `/admin/update-user/${userId}`,
      new URLSearchParams({
        username: username,
        groupIds: groupIds,
      })
    ),
  adminResetUserPassword: (userId) => fetchApi('POST', `/admin/reset-user-password/${userId}`),
  adminDeleteUser: (userId) => fetchApi('POST', `/admin/delete-user/${userId}`),

  adminGroupList: () => fetchApi('GET', '/admin/group-list'),
  adminGroup: (groupId) => fetchApi('GET', `/admin/group/${groupId}`),
  adminAddGroup: (name, userIds) =>
    fetchApi(
      'POST',
      '/admin/add-group',
      new URLSearchParams({
        name: name,
        userIds: userIds,
      })
    ),
  adminUpdateGroup: (groupId, name, userIds) =>
    fetchApi(
      'POST',
      `/admin/update-group/${groupId}`,
      new URLSearchParams({
        name: name,
        userIds: userIds,
      })
    ),
  adminDeleteGroup: (groupId) => fetchApi('POST', `/admin/delete-group/${groupId}`),
};

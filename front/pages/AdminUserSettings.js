import { html } from 'htm/preact';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import {
  selectedAdmin,
  selectedUser,
  admins,
  groups,
  videos,
  loadAdmin,
  loadUser,
  loadGroups,
  loadVideos,
  invalidateAdminLists,
} from '../store/admin.js';
import { AdminNav } from '../component/UserNav.js';
import { Form, useMultiSelect } from '../component/Form.js';
import { Loader } from '../component/Loader.js';
import { confirm } from '../component/ConfirmDialog.js';
import { validateField } from '../utils/validation.js';

function AdminUserSettingsBase({ userId, isAdmin }) {
  userId = Number(userId);
  const { route } = useLocation();
  const currentUser = isAdmin ? selectedAdmin : selectedUser;
  const selectGroupRef = useMultiSelect([currentUser.value, groups.value, videos.value]);
  const selectVideoRef = useMultiSelect([currentUser.value, groups.value, videos.value]);
  const loadUserCall = isAdmin ? loadAdmin : loadUser;
  const { isLoading: isUserLoading } = useLoader(() => loadUserCall(userId), currentUser.value?.id === userId, [isAdmin, userId]);
  const { isLoading: isGroupsLoading } = useLoader(loadGroups, isAdmin || Array.isArray(groups.value), [isAdmin]);
  const { isLoading: isVideosLoading } = useLoader(loadVideos, isAdmin || Array.isArray(videos.value), [isAdmin]);

  useTitle(`${currentUser.value?.name || (isAdmin ? 'Admin' : 'User')} Settings`);

  function isGroupSelected(groupId) {
    return currentUser.value?.groups?.find((g) => g.id === groupId);
  }

  function isVideoSelected(videoId) {
    return currentUser.value?.videos?.find((v) => v.id === videoId);
  }

  async function onUserSubmit(e) {
    const form = e.target;
    const username = form.elements['username'].value.trim();
    validateField(username);

    const oldName = currentUser.value?.name ?? 'this ' + (isAdmin ? 'admin' : 'user');
    const confirmMessage = username === oldName ? `Update ${username}?` : `Update ${oldName} to ${username}?`;
    if (!(await confirm(confirmMessage))) return;

    const groupSelect = form.elements['group-ids'];
    const groupIds = groupSelect ? Array.from(groupSelect.selectedOptions).map((o) => o.value) : [];

    const videoSelect = form.elements['video-ids'];
    const videoIds = videoSelect ? Array.from(videoSelect.selectedOptions).map((o) => o.value) : [];

    await api.adminUpdateUser(userId, username, groupIds, videoIds);
    if (isAdmin) {
      admins.value = null; // force to refresh the admin list
      route('/admin/admin-list');
    } else {
      invalidateAdminLists(); // force to refresh the user, group and video lists
      route('/admin/user-list');
    }
  }

  const title = isAdmin ? 'Change admin name' : 'Change username and groups';

  return html`
    <${AdminNav} />

    ${
      isUserLoading || (!isAdmin && (isGroupsLoading || isVideosLoading))
        ? html`<${Loader} />`
        : html`
            <${Form} title="${title}" buttonTitle="Update" onSubmitAction=${onUserSubmit}>
              <div class="pure-control-group">
                <input class="pure-input-1" type="text" name="username" placeholder="username" value=${currentUser.value.name} required />
              </div>
              ${
                !isAdmin &&
                !!groups.value.length &&
                html`
                  <div class="pure-control-group">
                    <select
                      ref=${selectGroupRef}
                      name="group-ids"
                      class="pure-input-1"
                      data-placeholder="Add groups to user (optional)"
                      multiple
                      data-multi-select
                    >
                      ${groups.value.map((g) => html`<option key=${g.id} value=${g.id} selected=${isGroupSelected(g.id)}>${g.name}</option>`)}
                    </select>
                  </div>
                `
              }
              ${
                !isAdmin &&
                !!videos.value.length &&
                html`
                  <div class="pure-control-group">
                    <select
                      ref=${selectVideoRef}
                      name="video-ids"
                      class="pure-input-1"
                      data-placeholder="Add videos to user (optional)"
                      multiple
                      data-multi-select
                    >
                      ${videos.value.map((v) => html`<option key=${v.id} value=${v.id} selected=${isVideoSelected(v.id)}>${v.title}</option>`)}
                    </select>
                  </div>
                `
              }
            <//>
          `
    }
  `;
}

export function AdminUserSettings({ userId }) {
  return html`<${AdminUserSettingsBase} userId=${userId} isAdmin=${false} />`;
}

export function AdminAdminSettings({ adminId }) {
  return html`<${AdminUserSettingsBase} userId=${adminId} isAdmin=${true} />`;
}

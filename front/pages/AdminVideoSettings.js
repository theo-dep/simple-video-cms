import { html } from 'htm/preact';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { refreshRequested } from '../store/auth.js';
import { selectedVideo, groups, users, videos, loadVideo, loadGroups, loadUsers } from '../store/admin.js';
import { AdminNav } from '../component/UserNav.js';
import { Form, useMultiSelect } from '../component/Form.js';
import { Loader } from '../component/Loader.js';
import { confirm } from '../component/ConfirmDialog.js';
import { validateField } from '../utils/validation.js';

export default function AdminUpdateVideo({ videoId }) {
  videoId = Number(videoId);
  const { route } = useLocation();
  const selectGroupsRef = useMultiSelect([selectedVideo.value, users.value, groups.value]);
  const selectUsersRef = useMultiSelect([selectedVideo.value, users.value, groups.value]);
  const { isLoading: isVideoLoading } = useLoader(() => loadVideo(videoId), selectedVideo.value?.id === videoId, [videoId]);
  const { isLoading: isGroupsLoading } = useLoader(loadGroups, Array.isArray(groups.value));
  const { isLoading: isUsersLoading } = useLoader(loadUsers, Array.isArray(users.value));

  useTitle(`${selectedVideo.value?.title || 'Video'} Settings`);

  function isGroupSelected(groupId) {
    return selectedVideo.value?.groups?.find((g) => g.id === groupId);
  }

  function isUserSelected(userId) {
    return selectedVideo.value?.users?.find((u) => u.id === userId);
  }

  async function onVideoSubmit(e) {
    const form = e.target;
    const title = form.elements['title'].value.trim();
    validateField(title);

    const oldTitle = selectedVideo.value?.title ?? 'this video';
    const confirmMessage = title === oldTitle ? `Update ${title} video?` : `Update ${oldTitle} video name to ${title}?`;
    if (!(await confirm(confirmMessage))) return;
    const groupSelect = form.elements['group-ids'];
    const userSelect = form.elements['user-ids'];
    const groupIds = groupSelect ? Array.from(groupSelect.selectedOptions).map((o) => o.value) : [];
    const userIds = userSelect ? Array.from(userSelect.selectedOptions).map((o) => o.value) : [];

    await api.adminUpdateVideo(videoId, title, groupIds, userIds);
    videos.value = null; // force to refresh the video list
    refreshRequested.value = true; // update this video to the admin video list (current user)
    route('/admin/video-list');
  }

  return html`
    <${AdminNav} />

    ${
      isVideoLoading || isGroupsLoading || isUsersLoading
        ? html`<${Loader} />`
        : html`
            <${Form} title="Change video title, group and user rights" buttonTitle="Update" onSubmitAction=${onVideoSubmit}>
              <div class="pure-control-group">
                <input class="pure-input-1" type="text" name="title" value=${selectedVideo.value.title} placeholder="Video title" required />
              </div>
              ${
                !!groups.value.length &&
                html`
                  <div class="pure-control-group">
                    <select
                      ref=${selectGroupsRef}
                      name="group-ids"
                      class="pure-input-1"
                      data-placeholder="Select groups (optional)"
                      multiple
                      data-multi-select
                    >
                      ${groups.value.map((g) => html`<option key=${g.id} value=${g.id} selected=${isGroupSelected(g.id)}>${g.name}</option>`)}
                    </select>
                  </div>
                `
              }
              ${
                !!users.value.length &&
                html`
                  <div class="pure-control-group">
                    <select
                      ref=${selectUsersRef}
                      name="user-ids"
                      class="pure-input-1"
                      data-placeholder="Select users (optional)"
                      multiple
                      data-multi-select
                    >
                      ${users.value.map((u) => html`<option key=${u.id} value=${u.id} selected=${isUserSelected(u.id)}>${u.name}</option>`)}
                    </select>
                  </div>
                `
              }
            <//>
          `
    }
  `;
}

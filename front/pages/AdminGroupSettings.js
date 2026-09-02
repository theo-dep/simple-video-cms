import { html } from 'htm/preact';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { selectedGroup, users, videos, loadGroup, loadUsers, loadVideos, invalidateAdminLists } from '../store/admin.js';
import { AdminNav } from '../component/HeaderNav.js';
import { Form } from '../component/Form.js';
import { MultiSelectDropDown } from '../component/SelectDropDown.js';
import { Loader } from '../component/Loader.js';
import { confirm } from '../component/ConfirmDialog.js';
import { RestrictedInput } from '../component/RestrictedInput.js';
import { validateText, TEXT_VALIDATION_TOOLTIP } from '../utils/validation.js';

export default function AdminGroupSettings({ groupId }) {
  groupId = Number(groupId);
  const { route } = useLocation();
  const { isLoading: isGroupLoading } = useLoader(() => loadGroup(groupId), selectedGroup.value?.id === groupId, [groupId]);
  const { isLoading: isUsersLoading } = useLoader(loadUsers, Array.isArray(users.value));
  const { isLoading: isVideosLoading } = useLoader(loadVideos, Array.isArray(videos.value));

  useTitle(`${selectedGroup.value?.name || 'Group'} Settings`);

  function isUserSelected(userId) {
    return selectedGroup.value?.users?.find((u) => u.id === userId);
  }

  function isVideoSelected(videoId) {
    return selectedGroup.value?.videos?.find((v) => v.id === videoId);
  }

  async function onGroupSubmit(e) {
    const form = e.target;
    const name = form.elements['name'].value.trim();
    validateText(name);

    const oldName = selectedGroup.value?.name ?? 'this group';
    const confirmMessage = name === oldName ? `Update ${name} group?` : `Update ${oldName} group name to ${name}?`;
    if (!(await confirm(confirmMessage))) return;

    const userSelect = form.elements['user-ids'];
    const userIds = userSelect ? Array.from(userSelect.selectedOptions).map((o) => o.value) : [];

    const videoSelect = form.elements['video-ids'];
    const videoIds = videoSelect ? Array.from(videoSelect.selectedOptions).map((o) => o.value) : [];

    await api.adminUpdateGroup(groupId, name, userIds, videoIds);
    invalidateAdminLists(); // force to refresh the group, user and video lists
    route('/admin/group-list');
  }

  return html`
    <${AdminNav} />

    ${
      isGroupLoading || isUsersLoading || isVideosLoading
        ? html`<${Loader} />`
        : html`
            <${Form} title="Change group name and users" buttonTitle="Update" onSubmitAction=${onGroupSubmit}>
              <div class="form-control-group">
                <${RestrictedInput} name="name" value=${selectedGroup.value.name} placeholder="Name" tooltip=${TEXT_VALIDATION_TOOLTIP} required />
              </div>
              ${
                !!users.value.length &&
                html`
                  <div class="form-control-group">
                    <${MultiSelectDropDown} name="user-ids" placeholder="Add users to group (optional)">
                      ${users.value.map((u) => html`<option key=${u.id} value=${u.id} selected=${isUserSelected(u.id)}>${u.name}</option>`)}
                    <//>
                  </div>
                `
              }
              ${
                !!videos.value.length &&
                html`
                  <div class="form-control-group">
                    <${MultiSelectDropDown} name="video-ids" placeholder="Add videos to group (optional)">
                      ${videos.value.map((v) => html`<option key=${v.id} value=${v.id} selected=${isVideoSelected(v.id)}>${v.title}</option>`)}
                    <//>
                  </div>
                `
              }
            <//>
          `
    }
  `;
}

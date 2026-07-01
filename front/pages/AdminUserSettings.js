import { html } from 'htm/preact';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { selectedAdmin, selectedUser, admins, groups, users, loadAdmin, loadUser, loadGroups } from '../store/admin.js';
import { AdminNav } from '../component/UserNav.js';
import { Form, useMultiSelect } from '../component/Form.js';
import { Loader } from '../component/Loader.js';
import { confirm } from '../component/ConfirmDialog.js';

function AdminUserSettingsBase({ userId, isAdmin }) {
  userId = Number(userId);
  const { route } = useLocation();
  const currentUser = isAdmin ? selectedAdmin : selectedUser;
  const selectRef = useMultiSelect([currentUser.value, groups.value]);
  const { isLoading: isUserLoading } = useLoader(isAdmin ? loadAdmin : loadUser, currentUser.value?.id === userId, [isAdmin, userId]);
  const { isLoading: isGroupsLoading } = useLoader(loadGroups, isAdmin || Array.isArray(groups.value), [isAdmin]);

  useTitle(`${currentUser.value?.name || (isAdmin ? 'Admin' : 'User')} Settings`);

  function isSelected(groupId) {
    return currentUser.value?.groups?.find((g) => g.id === groupId);
  }

  async function onUserSubmit(e) {
    const form = e.target;
    const username = form.elements['username'].value.trim();
    const oldName = currentUser.value?.name ?? 'this ' + (isAdmin ? 'admin' : 'user');
    const confirmMessage = username === oldName ? `Update ${username}?` : `Update ${oldName} to ${username}?`;
    if (!(await confirm(confirmMessage))) return;
    const groupSelect = form.elements['group-ids'];
    const groupIds = groupSelect ? Array.from(groupSelect.selectedOptions).map((o) => o.value) : [];

    await api.adminUpdateUser(userId, username, groupIds);
    if (isAdmin) {
      admins.value = null; // force to refresh the admin list
      route('/admin/admin-list');
    } else {
      users.value = null; // force to refresh the user list
      route('/admin/user-list');
    }
  }

  const title = isAdmin ? 'Change admin name' : 'Change username and groups';

  return html`
    <${AdminNav} />

    ${isUserLoading || (!isAdmin && isGroupsLoading)
      ? html`<${Loader} />`
      : html`
          <${Form} title="${title}" buttonTitle="Update" onSubmitAction=${onUserSubmit}>
            <div class="pure-control-group">
              <input class="pure-input-1" type="text" name="username" placeholder="username" value=${currentUser.value.name} required />
            </div>
            ${!isAdmin &&
            !!groups.value.length &&
            html`
              <div class="pure-control-group">
                <select ref=${selectRef} name="group-ids" class="pure-input-1" data-placeholder="Select groups (optional)" multiple data-multi-select>
                  ${groups.value.map((g) => html`<option key=${g.id} value=${g.id} selected=${isSelected(g.id)}>${g.name}</option>`)}
                </select>
              </div>
            `}
          <//>
        `}
  `;
}

export function AdminUserSettings({ userId }) {
  return html`<${AdminUserSettingsBase} userId=${userId} isAdmin=${false} />`;
}

export function AdminAdminSettings({ adminId }) {
  return html`<${AdminUserSettingsBase} userId=${adminId} isAdmin=${true} />`;
}

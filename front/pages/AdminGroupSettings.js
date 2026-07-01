import { html } from 'htm/preact';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { selectedGroup, groups, users, loadGroup, loadUsers } from '../store/admin.js';
import { AdminNav } from '../component/UserNav.js';
import { Form, useMultiSelect } from '../component/Form.js';
import { Loader } from '../component/Loader.js';
import { confirm } from '../component/ConfirmDialog.js';

export default function AdminGroupSettings({ groupId }) {
  groupId = Number(groupId);
  const { route } = useLocation();
  const selectRef = useMultiSelect([selectedGroup.value, users.value]);
  const { isLoading: isGroupLoading } = useLoader(loadGroup, selectedGroup.value?.id === groupId, [groupId]);
  const { isLoading: isUsersLoading } = useLoader(loadUsers, Array.isArray(users.value));

  useTitle(`${selectedGroup.value?.name || 'Group'} Settings`);

  function isSelected(userId) {
    return selectedGroup.value?.users?.find((u) => u.id === userId);
  }

  async function onGroupSubmit(e) {
    const form = e.target;
    const name = form.elements['name'].value.trim();
    const oldName = selectedGroup.value?.name ?? 'this group';
    const confirmMessage = name === oldName ? `Update ${name} group?` : `Update ${oldName} group name to ${name}?`;
    if (!(await confirm(confirmMessage))) return;
    const userSelect = form.elements['user-ids'];
    const userIds = userSelect ? Array.from(userSelect.selectedOptions).map((o) => o.value) : [];

    await api.adminUpdateGroup(groupId, name, userIds);
    groups.value = null; // force to refresh the group list
    route('/admin/group-list');
  }

  return html`
    <${AdminNav} />

    ${isGroupLoading || isUsersLoading
      ? html`<${Loader} />`
      : html`
          <${Form} title="Change group name and users" buttonTitle="Update" onSubmitAction=${onGroupSubmit}>
            <div class="pure-control-group">
              <input class="pure-input-1" type="text" name="name" value=${selectedGroup.value.name} placeholder="name" required />
            </div>
            ${!!users.value.length &&
            html`
              <div class="pure-control-group">
                <select
                  ref=${selectRef}
                  name="user-ids"
                  class="pure-input-1"
                  data-placeholder="Add users to group (optional)"
                  multiple
                  data-multi-select
                >
                  ${users.value.map((u) => html`<option key=${u.id} value=${u.id} selected=${isSelected(u.id)}>${u.name}</option>`)}
                </select>
              </div>
            `}
          <//>
        `}
  `;
}

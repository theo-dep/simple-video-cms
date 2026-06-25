import { html } from 'htm/preact';
import { useState } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { selectedItem } from '../store/selection.js';
import { AdminNav } from '../component/UserNav.js';
import { Form, useMultiSelect } from '../component/Form.js';
import { Loader } from '../component/Loader.js';

export default function AdminGroupSettings({ groupId }) {
  groupId = Number(groupId);
  const { route } = useLocation();
  const [group, setGroup] = useState(selectedItem.value);
  const [users, setUsers] = useState(null);
  const selectRef = useMultiSelect([group, users]);
  const { isLoading: isGroupLoading } = useLoader(loadGroup, group !== null && group.id === groupId, [groupId]);
  const { isLoading: isUsersLoading } = useLoader(loadUsers, Array.isArray(users));

  useTitle(group ? `${group.name} Settings` : 'Group Settings');

  async function loadGroup() {
    try {
      const r = await api.adminGroup(groupId);
      setGroup(r.json ?? r);
    } catch {
      route('/403');
    }
  }

  async function loadUsers() {
    try {
      const r = await api.adminUserList();
      setUsers(r.json ?? r);
    } catch {
      route('/403');
    }
  }

  function isSelected(userId) {
    return group.users.find((u) => u.id === userId);
  }

  async function onGroupSubmit(e) {
    if (!confirm('Update this group?')) return;

    const form = e.target;
    const name = form.elements['name'].value.trim();
    const userSelect = form.elements['user-ids'];
    const userIds = userSelect ? Array.from(userSelect.selectedOptions).map((o) => o.value) : [];

    await api.adminUpdateGroup(groupId, name, userIds);
    route('/admin/group-list');
  }

  return html`
    <${AdminNav} />

    ${isGroupLoading || isUsersLoading
      ? html`<${Loader} />`
      : html`
          <${Form} title="Change group name and users" buttonTitle="Update" onSubmitAction=${onGroupSubmit}>
            <div class="pure-control-group">
              <input class="pure-input-1" type="text" name="name" value=${group.name} placeholder="name" required />
            </div>
            ${!!users.length &&
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
                  ${users.map((u) => html`<option key=${u.id} value=${u.id} selected=${isSelected(u.id)}>${u.name}</option>`)}
                </select>
              </div>
            `}
          <//>
        `}
  `;
}

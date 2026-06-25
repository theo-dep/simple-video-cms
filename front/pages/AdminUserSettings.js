import { html } from 'htm/preact';
import { useState, useEffect } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { selectedItem } from '../store/selection.js';
import { AdminNav } from '../component/UserNav.js';
import { Form, useMultiSelect } from '../component/Form.js';

function AdminUserSettingsBase({ userId, isAdmin }) {
  userId = Number(userId);
  const { route } = useLocation();
  const [user, setUser] = useState(selectedItem.value);
  const [groups, setGroups] = useState(null);
  const selectRef = useMultiSelect([user, groups]);

  useTitle(user ? `${user.name} Settings` : 'User Settings');

  useEffect(() => {
    if (user === null || user.id !== userId) {
      const fetchUser = isAdmin ? api.adminAdmin(userId) : api.adminUser(userId);
      fetchUser.then((r) => setUser(r.json ?? r)).catch(() => route('/403'));
    }

    if (isAdmin || Array.isArray(groups)) return;
    api
      .adminGroupList()
      .then((r) => setGroups(r.json ?? r))
      .catch(() => route('/403'));
  }, [userId, isAdmin]);

  function isSelected(groupId) {
    return user.groups.find((g) => g.id === groupId);
  }

  async function onUserSubmit(e) {
    if (!confirm(`Update this ${isAdmin ? 'admin' : 'user'}?`)) return;

    const form = e.target;
    const username = form.elements['username'].value.trim();
    const groupSelect = form.elements['group-ids'];
    const groupIds = groupSelect ? Array.from(groupSelect.selectedOptions).map((o) => o.value) : [];

    await api.adminUpdateUser(userId, username, groupIds);
    route(isAdmin ? '/admin/admin-list' : '/admin/user-list');
  }

  return html`
    <${AdminNav} />

    ${user !== null &&
    (isAdmin || Array.isArray(groups)) &&
    html`
      <${Form} title="Change username${!isAdmin ? ' and groups' : ''}" buttonTitle="Update" onSubmitAction=${onUserSubmit}>
        <div class="pure-control-group">
          <input class="pure-input-1" type="text" name="username" placeholder="username" value=${user.name} required />
        </div>
        ${!isAdmin &&
        !!groups?.length &&
        html`
          <div class="pure-control-group">
            <select ref=${selectRef} name="group-ids" class="pure-input-1" data-placeholder="Select groups (optional)" multiple data-multi-select>
              ${groups.map((g) => html`<option key=${g.id} value=${g.id} selected=${isSelected(g.id)}>${g.name}</option>`)}
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

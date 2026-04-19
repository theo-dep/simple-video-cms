import { html } from 'htm/preact';
import { useState, useEffect } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { selectedItem } from '../store/selection.js';
import { AdminNav } from '../component/UserNav.js';
import { Form, useMultiSelect } from '../component/Form.js';

export default function AdminUpdateUser({ userId }) {
  userId = Number(userId);
  const { query, route } = useLocation();
  const isAdmin = query.isAdmin === 'true';
  const [user, setUser] = useState(selectedItem.value);
  const [groups, setGroups] = useState([]);
  const selectRef = useMultiSelect([user, groups]);

  useEffect(() => {
    if (!user || user.id !== userId) {
      api
        .adminUserList()
        .then((allUsers) => {
          const au = allUsers.json ?? allUsers;
          const user = au.find((u) => u.id === userId) ?? { name: '', groups: [] };
          setUser(user);
        })
        .catch(() => route('/403'));
    }

    if (!isAdmin) {
      api
        .adminGroupList()
        .then((r) => setGroups(r.json ?? r))
        .catch(() => route('/403'));
    }
  }, [isAdmin, userId]);

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

    ${user &&
    groups &&
    html`
      <${Form} title="Change username${!isAdmin ? ' and groups' : ''}" buttonTitle="Update" onSubmitAction=${onUserSubmit}>
        <div class="pure-control-group">
          <input class="pure-input-1" type="text" name="username" placeholder="username" value=${user.name} required />
        </div>
        ${!isAdmin &&
        !!groups.length &&
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

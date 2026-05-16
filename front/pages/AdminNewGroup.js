import { html } from 'htm/preact';
import { useState, useEffect } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { AdminNav } from '../component/UserNav.js';
import { Form, useMultiSelect } from '../component/Form.js';

export default function AdminNewGroup() {
  const { route } = useLocation();
  const [users, setUsers] = useState([]);
  const selectRef = useMultiSelect([users]);

  useTitle('New Group');

  useEffect(() => {
    api
      .adminUserList()
      .then((r) => setUsers(r.json ?? r))
      .catch(() => route('/403'));
  }, []);

  async function onGroupSubmit(e) {
    const form = e.target;
    const name = form.elements['name'].value.trim();
    const userSelect = form.elements['user-ids'];
    const userIds = userSelect ? Array.from(userSelect.selectedOptions).map((o) => o.value) : [];

    await api.adminAddGroup(name, userIds);
    route('/admin/group-list');
  }

  return html`
    <${AdminNav} />

    ${users &&
    html`
      <${Form} title="Add a new group" buttonTitle="Create" onSubmitAction=${onGroupSubmit}>
        <div class="pure-control-group">
          <input class="pure-input-1" type="text" name="name" placeholder="name" required autofocus />
        </div>
        ${!!users.length &&
        html`
          <div class="pure-control-group">
            <select ref=${selectRef} name="user-ids" class="pure-input-1" data-placeholder="Add users to group (optional)" multiple data-multi-select>
              ${users.map((u) => html`<option key=${u.id} value=${u.id}>${u.name}</option>`)}
            </select>
          </div>
        `}
      <//>
    `}
  `;
}

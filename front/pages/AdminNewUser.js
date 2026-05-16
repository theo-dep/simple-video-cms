import { html } from 'htm/preact';
import { useState, useEffect } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { AdminNav } from '../component/UserNav.js';
import { Form, useMultiSelect } from '../component/Form.js';

export default function AdminNewUser() {
  const { query, route } = useLocation();
  const isAdmin = query.isAdmin === 'true';
  const [groups, setGroups] = useState([]);
  const selectRef = useMultiSelect([groups]);

  useTitle('New User');

  useEffect(() => {
    if (!isAdmin) {
      api
        .adminGroupList()
        .then((r) => setGroups(r.json ?? r))
        .catch(() => route('/403'));
    }
  }, [isAdmin]);

  async function onUserSubmit(e) {
    const form = e.target;
    const username = form.elements['username'].value.trim();
    const groupSelect = form.elements['group-ids'];
    const groupIds = groupSelect ? Array.from(groupSelect.selectedOptions).map((o) => o.value) : [];

    if (isAdmin) {
      await api.adminAddAdmin(username);
      route('/admin/admin-list');
    } else {
      await api.adminAddUser(username, groupIds);
      route('/admin/user-list');
    }
  }

  return html`
    <${AdminNav} />

    ${groups &&
    html`
      <${Form} title="Add a new ${isAdmin ? 'admin' : 'user'}" buttonTitle="Create" onSubmitAction=${onUserSubmit}>
        <div class="pure-control-group">
          <input class="pure-input-1" type="text" name="username" placeholder="username" required autofocus />
        </div>
        ${!!groups.length &&
        html`
          <div class="pure-control-group">
            <select ref=${selectRef} name="group-ids" data-placeholder="Select groups (optional)" multiple data-multi-select>
              ${groups.map((g) => html`<option key=${g.id} value=${g.id}>${g.name}</option>`)}
            </select>
          </div>
        `}
      <//>
    `}
  `;
}

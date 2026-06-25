import { html } from 'htm/preact';
import { useState } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { AdminNav } from '../component/UserNav.js';
import { Form, useMultiSelect } from '../component/Form.js';
import { Loader } from '../component/Loader.js';

function AdminNewUserBase({ isAdmin }) {
  const { route } = useLocation();
  const [groups, setGroups] = useState(null);
  const selectRef = useMultiSelect([groups]);
  const { isLoading } = useLoader(load, isAdmin || Array.isArray(groups), [isAdmin]);

  useTitle('New User');

  async function load() {
    try {
      const r = await api.adminGroupList();
      setGroups(r.json ?? r);
    } catch {
      route('/403');
    }
  }

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

    ${isLoading
      ? html`<${Loader} />`
      : html`
          <${Form} title="Add a new ${isAdmin ? 'admin' : 'user'}" buttonTitle="Create" onSubmitAction=${onUserSubmit}>
            <div class="pure-control-group">
              <input class="pure-input-1" type="text" name="username" placeholder="username" required autofocus />
            </div>
            ${!isAdmin &&
            !!groups?.length &&
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

export function AdminNewUser() {
  return html`<${AdminNewUserBase} isAdmin=${false} />`;
}

export function AdminNewAdmin() {
  return html`<${AdminNewUserBase} isAdmin=${true} />`;
}

import { html } from 'htm/preact';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { groups, loadGroups, invalidateUsers, invalidateAdmins } from '../store/admin.js';
import { AdminNav } from '../component/UserNav.js';
import { Form, useMultiSelect } from '../component/Form.js';
import { Loader } from '../component/Loader.js';
import { validateField } from '../utils/validation.js';

function AdminNewUserBase({ isAdmin }) {
  const { route } = useLocation();
  const selectRef = useMultiSelect([groups.value]);
  const { isLoading } = useLoader(loadGroups, isAdmin || Array.isArray(groups.value), [isAdmin]);

  useTitle(`New ${isAdmin ? 'Admin' : 'User'}`);

  async function onUserSubmit(e) {
    const form = e.target;
    const username = form.elements['username'].value.trim();
    validateField(username);

    const groupSelect = form.elements['group-ids'];
    const groupIds = groupSelect ? Array.from(groupSelect.selectedOptions).map((o) => o.value) : [];

    if (isAdmin) {
      await api.adminAddAdmin(username);
      invalidateAdmins(); // force to refresh the admin list and stats
      route('/admin/admin-list');
    } else {
      await api.adminAddUser(username, groupIds);
      invalidateUsers(); // force to refresh the user list and stats
      route('/admin/user-list');
    }
  }

  const title = isAdmin ? 'Add a new admin' : 'Add a new user';

  return html`
    <${AdminNav} />

    ${!isAdmin && isLoading
      ? html`<${Loader} />`
      : html`
          <${Form} title="${title}" buttonTitle="Create" onSubmitAction=${onUserSubmit}>
            <div class="pure-control-group">
              <input class="pure-input-1" type="text" name="username" placeholder="username" required autofocus />
            </div>
            ${!isAdmin &&
            !!groups.value.length &&
            html`
              <div class="pure-control-group">
                <select ref=${selectRef} name="group-ids" data-placeholder="Select groups (optional)" multiple data-multi-select>
                  ${groups.value.map((g) => html`<option key=${g.id} value=${g.id}>${g.name}</option>`)}
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

import { html } from 'htm/preact';
import { useState } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { selectedItem } from '../store/selection.js';
import { Content } from '../component/Content.js';
import { AdminNav } from '../component/UserNav.js';
import { ListTable } from '../component/ListTable.js';
import { Drawer } from '../component/Drawer.js';
import { Loader } from '../component/Loader.js';
import { PersonAddIcon } from '../svg/PersonAddIcon.js';

export default function AdminUserList() {
  const { route } = useLocation();
  const [users, setUsers] = useState(null);
  const { isLoading } = useLoader(load, Array.isArray(users));

  useTitle('User List');

  async function load() {
    try {
      const r = await api.adminUserList();
      setUsers(r.json ?? r);
    } catch {
      route('/403');
    }
  }

  function updateUser(user) {
    selectedItem.value = user;
    route('/admin/user-settings/' + user.id);
  }

  async function resetUser(id) {
    if (!confirm('Reset this user?')) return;
    await api.adminResetUserPassword(id);
    await load();
  }

  async function deleteUser(id) {
    if (!confirm('Delete this user?')) return;
    await api.adminDeleteUser(id);
    await load();
  }

  return html`
    <${AdminNav} />

    <${Content}>
      ${isLoading
        ? html`<${Loader} />`
        : html`<${ListTable}
            title="List of users"
            icon="${html`<title>Add user</title> <${PersonAddIcon} />`}"
            addLink="/admin/new-user"
            columns="${['Username']}"
            items="${users}"
            renderRow="${(u) => html`
              <tr key=${u.id}>
                <td>${u.name}</td>
                <td>
                  <div class="pure-g">
                    <div class="pure-u-1 pure-u-sm-1-4">
                      ${!!u.groups?.length &&
                      html`<${Drawer} label="Groups" items=${[{ label: 'User Groups', elements: u.groups.map((g) => g.name) }]} />`}
                    </div>
                    <div class="pure-u-1 pure-u-sm-1-4">
                      <a onClick=${() => updateUser(u)} style="cursor:pointer">Update</a>
                    </div>
                    <div class="pure-u-1 pure-u-sm-1-4">
                      ${u.isLoggedOnce && html` <a onClick=${() => resetUser(u.id)} style="cursor:pointer">Reset password</a>`}
                    </div>
                    <div class="pure-u-1 pure-u-sm-1-4">
                      <a onClick=${() => deleteUser(u.id)} style="cursor:pointer">Delete</a>
                    </div>
                  </div>
                </td>
              </tr>
            `}"
          /> `}
    <//>
  `;
}

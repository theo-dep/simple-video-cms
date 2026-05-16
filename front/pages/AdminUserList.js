import { html } from 'htm/preact';
import { useState, useEffect } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { selectedItem } from '../store/selection.js';
import { Content } from '../component/Content.js';
import { AdminNav } from '../component/UserNav.js';
import { Drawer } from '../component/Drawer.js';
import { PersonAddIcon } from '../svg/PersonAddIcon.js';

export default function AdminUserList() {
  const { route } = useLocation();
  const [users, setUsers] = useState(null);

  useTitle('User List');

  function load() {
    api
      .adminUserList()
      .then((r) => setUsers(r.json ?? r))
      .catch(() => route('/403'));
  }

  useEffect(() => {
    load();
  }, []);

  function updateUser(user) {
    selectedItem.value = user;
    route('/admin/user-settings/' + user.id + '?isAdmin=false');
  }

  async function resetUser(id) {
    if (!confirm('Reset this user?')) return;
    await api.adminResetUserPassword(id);
    load();
  }

  async function deleteUser(id) {
    if (!confirm('Delete this user?')) return;
    await api.adminDeleteUser(id);
    load();
  }

  return html`
    <${AdminNav} />

    <${Content}>
      <h2>List of users</h2>
      ${users &&
      html`
        <table id="table" class="table pure-table pure-table-horizontal">
          <thead>
            <tr>
              <th>Username</th>
              <th class="add-user">
                <a href="/admin/new-user?isAdmin=false">
                  <svg class="svg-button">
                    <title>Add user</title>
                    <${PersonAddIcon} />
                  </svg>
                </a>
              </th>
            </tr>
          </thead>
          <tbody>
            ${users.map(
              (u) => html`
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
              `
            )}
          </tbody>
        </table>
      `}
    <//>
  `;
}

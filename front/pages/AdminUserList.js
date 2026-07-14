import { html } from 'htm/preact';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { selectedUser, users, loadUsers, invalidateUsers } from '../store/admin.js';
import { Content } from '../component/Content.js';
import { AdminNav } from '../component/UserNav.js';
import { ListTable } from '../component/ListTable.js';
import { Drawer } from '../component/Drawer.js';
import { Loader } from '../component/Loader.js';
import { PersonAddIcon } from '../svg/PersonAddIcon.js';
import { confirm } from '../component/ConfirmDialog.js';

export default function AdminUserList() {
  const { route } = useLocation();
  const { isLoading } = useLoader(loadUsers, Array.isArray(users.value));

  useTitle('User List');

  function updateUser(user) {
    selectedUser.value = user;
    route('/admin/user-settings/' + selectedUser.value.id);
  }

  async function resetUser(id) {
    const name = users.value?.find((u) => u.id === id)?.name ?? 'this user';
    if (!(await confirm(`Reset ${name} password?`))) return;
    await api.adminResetUserPassword(id);
    await loadUsers();
  }

  async function deleteUser(id) {
    const name = users.value?.find((u) => u.id === id)?.name ?? 'this user';
    if (!(await confirm(`Delete ${name}?`))) return;
    await api.adminDeleteUser(id);
    invalidateUsers(); // reset stats
    await loadUsers();
  }

  return html`
    <${AdminNav} />

    <${Content}>
      ${
        isLoading
          ? html`<${Loader} />`
          : html`<${ListTable}
              title="List of users"
              icon="${html`<title>Add user</title> <${PersonAddIcon} />`}"
              addLink="/admin/new-user"
              columns="${['Username']}"
              items="${users.value}"
              renderRow="${(u) => html`
                <tr key=${u.id}>
                  <td>${u.name}</td>
                  <td>
                    <div class="pure-g">
                      <div class="pure-u-1 pure-u-sm-1-4">
                        ${
                          !!u.groups?.length &&
                          html`<${Drawer} label="Groups" items=${[{ label: 'User Groups', elements: u.groups.map((g) => g.name) }]} />`
                        }
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
            /> `
      }
    <//>
  `;
}

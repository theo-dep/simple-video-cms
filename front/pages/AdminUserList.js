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

  async function deactivateUser(id, deactivated) {
    const name = users.value?.find((u) => u.id === id)?.name ?? 'this user';
    if (!(await confirm(`${deactivated ? 'Deactivate' : 'Activate'} ${name}?`))) return;
    await api.adminDeactivateUser(id, deactivated);
    await loadUsers();
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
              columns="${[{ label: 'Username', key: 'name' }]}"
              items="${users.value}"
              searchKeys="${['name']}"
              renderRow="${(u) => html`
                <tr key=${u.id}>
                  <td>${u.name}</td>
                  <td>
                    <div class="pure-g">
                      <div class="table-button pure-u-1 pure-u-sm-1-3 pure-u-md-1-5">
                        ${
                          (!!u.groups?.length || !!u.videos?.length) &&
                          html`<${Drawer}
                            label="Groups and Rights"
                            items=${[
                              { label: 'User Groups', elements: u.groups.map((g) => g.name) ?? [] },
                              { label: 'Video Rights', elements: u.videos.map((v) => v.title) ?? [] },
                            ]}
                          />`
                        }
                      </div>
                      <div class="table-button pure-u-1 pure-u-sm-1-3 pure-u-md-1-5">
                        <a onClick=${() => updateUser(u)}>Update</a>
                      </div>
                      <div class="table-button pure-u-1 pure-u-sm-1-3 pure-u-md-1-5">
                        <a onClick=${() => deactivateUser(u.id, !u.isDeactivated)}> ${u.isDeactivated ? 'Activate' : 'Deactivate'} </a>
                      </div>
                      <div class="table-button pure-u-1 pure-u-sm-1-3 pure-u-md-1-5">
                        ${u.isLoggedOnce && html` <a onClick=${() => resetUser(u.id)}>Reset password</a>`}
                      </div>
                      <div class="table-button pure-u-1 pure-u-sm-1-3 pure-u-md-1-5">
                        <a onClick=${() => deleteUser(u.id)}>Delete</a>
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

import { html } from 'htm/preact';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { selectedUser, users, loadUsers, invalidateUsers } from '../store/admin.js';
import { Content } from '../component/Content.js';
import { AdminNav } from '../component/UserNav.js';
import { ListTable } from '../component/ListTable.js';
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
              addContent="${html`<svg class="svg-button"><${PersonAddIcon} /></svg> New user`}"
              addLink="/admin/new-user"
              columns="${[{ key: 'name', label: 'Username', sortValue: (u) => u.name }]}"
              items="${users.value}"
              searchKeys="${['name']}"
              renderExpanded="${(u) => html`
                <div class="list-expanded-actions">
                  <a onClick=${() => updateUser(u)}>Update</a>
                  <a onClick=${() => deactivateUser(u.id, !u.isDeactivated)}>${u.isDeactivated ? 'Activate' : 'Deactivate'}</a>
                  ${u.isLoggedOnce && html`<a onClick=${() => resetUser(u.id)}>Reset password</a>`}
                  <a onClick=${() => deleteUser(u.id)}>Delete</a>
                </div>
                ${
                  (!!u.groups?.length || !!u.videos?.length) &&
                  html`
                    <div class="list-expanded-info">
                      ${
                        !!u.groups?.length &&
                        html`
                          <div class="list-expanded-info-row">
                            <h4>Groups</h4>
                            ${u.groups?.map((g) => html`<p>${g.name}</p>`)}
                          </div>
                        `
                      }
                      ${
                        !!u.videos?.length &&
                        html`
                          <div class="list-expanded-info-row">
                            <h4>Video Rights</h4>
                            ${u.videos?.map((v) => html`<p>${v.title}</p>`)}
                          </div>
                        `
                      }
                    </div>
                  `
                }
              `}"
            />`
      }
    <//>
  `;
}

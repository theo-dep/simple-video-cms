import { html } from 'htm/preact';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { selectedAdmin, admins, loadAdmins, invalidateAdmins } from '../store/admin.js';
import { Content } from '../component/Content.js';
import { AdminNav } from '../component/HeaderNav.js';
import { ListTable } from '../component/ListTable.js';
import { Loader } from '../component/Loader.js';
import { Icon } from '../component/Icon.js';
import { confirm } from '../component/ConfirmDialog.js';

export default function AdminAdminList() {
  const { route } = useLocation();
  const { isLoading } = useLoader(loadAdmins, Array.isArray(admins.value));

  useTitle('Admin List');

  function updateAdmin(admin) {
    selectedAdmin.value = admin;
    route('/admin/admin-settings/' + selectedAdmin.value.id);
  }

  async function deactivateUser(id, deactivated) {
    const name = admins.value?.find((u) => u.id === id)?.name ?? 'this admin';
    if (!(await confirm(`${deactivated ? 'Deactivate' : 'Activate'} ${name}?`))) return;
    await api.adminDeactivateUser(id, deactivated);
    await loadAdmins();
  }

  async function resetUser(id) {
    const name = admins.value?.find((a) => a.id === id)?.name ?? 'this admin';
    if (!(await confirm(`Reset ${name} password?`))) return;
    await api.adminResetUserPassword(id);
    await loadAdmins();
  }

  async function deleteUser(id) {
    const name = admins.value?.find((a) => a.id === id)?.name ?? 'this admin';
    if (!(await confirm(`Delete ${name}?`))) return;
    await api.adminDeleteUser(id);
    invalidateAdmins(); // reset stats
    await loadAdmins();
  }

  return html`
    <${AdminNav} />

    <${Content}>
      ${
        isLoading
          ? html`<${Loader} />`
          : html`<${ListTable}
              title="List of administrators"
              addContent="${html`<${Icon} name="person-add" class="svg-button" /> New admin`}"
              addLink="/admin/new-admin"
              columns="${[{ key: 'name', label: 'Username', sortValue: (a) => a.name }]}"
              items="${admins.value}"
              searchKeys="${['name']}"
              renderExpanded="${(a) =>
                !a.isSuperAdmin &&
                html`
                  <div class="list-expanded-actions">
                    <a onClick=${() => updateAdmin(a)}>Update</a>
                    <a onClick=${() => deactivateUser(a.id, !a.isDeactivated)}>${a.isDeactivated ? 'Activate' : 'Deactivate'}</a>
                    ${a.isLoggedOnce && html`<a onClick=${() => resetUser(a.id)}>Reset password</a>`}
                    <a onClick=${() => deleteUser(a.id)}>Delete</a>
                  </div>
                `}"
            />`
      }
    <//>
  `;
}

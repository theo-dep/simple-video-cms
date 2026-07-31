import { html } from 'htm/preact';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { selectedAdmin, admins, invalidateAdmins, loadAdmins } from '../store/admin.js';
import { Content } from '../component/Content.js';
import { AdminNav } from '../component/UserNav.js';
import { ListTable } from '../component/ListTable.js';
import { Loader } from '../component/Loader.js';
import { PersonAddIcon } from '../svg/PersonAddIcon.js';
import { confirm } from '../component/ConfirmDialog.js';

export default function AdminAdminList() {
  const { route } = useLocation();
  const { isLoading } = useLoader(loadAdmins, Array.isArray(admins.value));

  useTitle('Admin List');

  function updateAdmin(admin) {
    selectedAdmin.value = admin;
    route('/admin/admin-settings/' + selectedAdmin.value.id);
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
              icon="${html`<title>Add admin</title> <${PersonAddIcon} />`}"
              addLink="/admin/new-admin"
              columns="${['Username']}"
              items="${admins.value}"
              searchKeys="${['name']}"
              renderRow="${(a) => html`
                <tr key=${a.id}>
                  <td>${a.name}</td>
                  <td>
                    ${
                      !a.isSuperAdmin &&
                      html`
                        <div class="pure-g">
                          <div class="pure-u-1 pure-u-sm-1-3">
                            <a onClick=${() => updateAdmin(a)} style="cursor:pointer">Update</a>
                          </div>
                          <div class="pure-u-1 pure-u-sm-1-3">
                            ${a.isLoggedOnce && html` <a onClick=${() => resetUser(a.id)} style="cursor:pointer">Reset password</a>`}
                          </div>
                          <div class="pure-u-1 pure-u-sm-1-3">
                            <a onClick=${() => deleteUser(a.id)} style="cursor:pointer">Delete</a>
                          </div>
                        </div>
                      `
                    }
                  </td>
                </tr>
              `}"
            />`
      }
    <//>
  `;
}

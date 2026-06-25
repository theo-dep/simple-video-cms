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
import { Loader } from '../component/Loader.js';
import { PersonAddIcon } from '../svg/PersonAddIcon.js';

export default function AdminAdminList() {
  const { route } = useLocation();
  const [admins, setAdmins] = useState(null);
  const { isLoading } = useLoader(load, Array.isArray(admins));

  useTitle('Admin List');

  async function load() {
    try {
      const r = await api.adminAdminList();
      setAdmins(r.json ?? r);
    } catch {
      route('/403');
    }
  }

  function updateAdmin(admin) {
    selectedItem.value = admin;
    route('/admin/admin-settings/' + admin.id);
  }

  async function resetUser(id) {
    if (!confirm('Reset this admin?')) return;
    await api.adminResetUserPassword(id);
    await load();
  }

  async function deleteUser(id) {
    if (!confirm('Delete this admin?')) return;
    await api.adminDeleteUser(id);
    await load();
  }

  return html`
    <${AdminNav} />

    <${Content}>
      ${isLoading
        ? html`<${Loader} />`
        : html`<${ListTable}
            title="List of administrators"
            icon="${html`<title>Add admin</title> <${PersonAddIcon} />`}"
            addLink="/admin/new-admin"
            columns="${['Username']}"
            items="${admins}"
            renderRow="${(a) => html`
              <tr key=${a.id}>
                <td>${a.name}</td>
                <td>
                  ${!a.isSuperAdmin &&
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
                  `}
                </td>
              </tr>
            `}"
          />`}
    <//>
  `;
}

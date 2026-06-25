import { html } from 'htm/preact';
import { useState, useEffect } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { selectedItem } from '../store/selection.js';
import { Content } from '../component/Content.js';
import { AdminNav } from '../component/UserNav.js';
import { PersonAddIcon } from '../svg/PersonAddIcon.js';

export default function AdminAdminList() {
  const { route } = useLocation();
  const [admins, setAdmins] = useState(null);

  useTitle('Admin List');

  function load() {
    api
      .adminAdminList()
      .then((r) => setAdmins(r.json ?? r))
      .catch(() => route('/403'));
  }

  useEffect(() => {
    if (Array.isArray(admins)) return;
    load();
  }, []);

  function updateAdmin(admin) {
    selectedItem.value = admin;
    route('/admin/admin-settings/' + admin.id);
  }

  async function resetUser(id) {
    if (!confirm('Reset this admin?')) return;
    await api.adminResetUserPassword(id);
    load();
  }

  async function deleteUser(id) {
    if (!confirm('Delete this admin?')) return;
    await api.adminDeleteUser(id);
    load();
  }

  return html`
    <${AdminNav} />

    <${Content}>
      <h2>List of administrators</h2>
      ${Array.isArray(admins) &&
      html`
        <table id="table" class="table pure-table pure-table-horizontal">
          <thead>
            <tr>
              <th>Username</th>
              <th class="add-user">
                <a href="/admin/new-admin">
                  <svg class="svg-button">
                    <title>Add admin</title>
                    <${PersonAddIcon} />
                  </svg>
                </a>
              </th>
            </tr>
          </thead>
          <tbody>
            ${admins.map(
              (a) => html`
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
              `
            )}
          </tbody>
        </table>
      `}
    <//>
  `;
}

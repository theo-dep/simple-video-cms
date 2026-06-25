import { html } from 'htm/preact';
import { useState, useEffect } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { selectedItem } from '../store/selection.js';
import { Content } from '../component/Content.js';
import { AdminNav } from '../component/UserNav.js';
import { Drawer } from '../component/Drawer.js';
import { GroupAddIcon } from '../svg/GroupAddIcon.js';

export default function AdminGroupList() {
  const { route } = useLocation();
  const [groups, setGroups] = useState(null);

  useTitle('Group List');

  function load() {
    api
      .adminGroupList()
      .then((r) => setGroups(r.json ?? r))
      .catch(() => route('/403'));
  }

  useEffect(() => {
    if (Array.isArray(groups)) return;
    load();
  }, []);

  function updateGroup(group) {
    selectedItem.value = group;
    route('/admin/group-settings/' + group.id);
  }

  async function deleteGroup(id) {
    if (!confirm('Delete this group?')) return;
    await api.adminDeleteGroup(id);
    load();
  }

  return html`
    <${AdminNav} />

    <${Content}>
      <h2>List of groups</h2>
      ${Array.isArray(groups) &&
      html`
        <table id="table" class="table pure-table pure-table-horizontal">
          <thead>
            <tr>
              <th>Name</th>
              <th class="add-user">
                <a href="/admin/new-group">
                  <svg class="svg-button">
                    <title>Add group</title>
                    <${GroupAddIcon} />
                  </svg>
                </a>
              </th>
            </tr>
          </thead>
          <tbody>
            ${groups.map(
              (g) => html`
                <tr key=${g.id}>
                  <td>${g.name}</td>
                  <td>
                    <div class="pure-g">
                      <div class="pure-u-1 pure-u-sm-1-3">
                        ${!!g.users?.length &&
                        html`<${Drawer} label="Users" items=${[{ label: 'Group Users', elements: g.users.map((u) => u.name) }]} />`}
                      </div>
                      <div class="pure-u-1 pure-u-sm-1-3">
                        <a onClick=${() => updateGroup(g)} style="cursor:pointer">Update</a>
                      </div>
                      <div class="pure-u-1 pure-u-sm-1-3">
                        <a onClick=${() => deleteGroup(g.id)} style="cursor:pointer">Delete</a>
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

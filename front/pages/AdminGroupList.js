import { html } from 'htm/preact';
import { useState } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { selectedItem } from '../store/selection.js';
import { Content } from '../component/Content.js';
import { AdminNav } from '../component/UserNav.js';
import { Drawer } from '../component/Drawer.js';
import { ListTable } from '../component/ListTable.js';
import { Loader } from '../component/Loader.js';
import { GroupAddIcon } from '../svg/GroupAddIcon.js';

export default function AdminGroupList() {
  const { route } = useLocation();
  const [groups, setGroups] = useState(null);
  const { isLoading } = useLoader(load, Array.isArray(groups));

  useTitle('Group List');

  async function load() {
    try {
      const r = await api.adminGroupList();
      setGroups(r.json ?? r);
    } catch {
      route('/403');
    }
  }

  function updateGroup(group) {
    selectedItem.value = group;
    route('/admin/group-settings/' + group.id);
  }

  async function deleteGroup(id) {
    if (!confirm('Delete this group?')) return;
    await api.adminDeleteGroup(id);
    await load();
  }

  return html`
    <${AdminNav} />

    <${Content}>
      ${isLoading
        ? html`<${Loader} />`
        : html`<${ListTable}
            title="List of groups"
            icon="${html`<title>Add group</title> <${GroupAddIcon} />`}"
            addLink="/admin/new-group"
            columns="${['Name']}"
            items="${groups}"
            renderRow="${(g) => html`
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
            `}"
          /> `}
    <//>
  `;
}

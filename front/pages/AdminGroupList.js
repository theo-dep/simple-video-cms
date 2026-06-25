import { html } from 'htm/preact';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { group, groups, invalidateGroups, loadGroups } from '../store/admin.js';
import { Content } from '../component/Content.js';
import { AdminNav } from '../component/UserNav.js';
import { Drawer } from '../component/Drawer.js';
import { ListTable } from '../component/ListTable.js';
import { Loader } from '../component/Loader.js';
import { GroupAddIcon } from '../svg/GroupAddIcon.js';

export default function AdminGroupList() {
  const { route } = useLocation();
  const { isLoading } = useLoader(loadGroups, Array.isArray(groups.value));

  useTitle('Group List');

  function updateGroup(selectedGroup) {
    group.value = selectedGroup;
    route('/admin/group-settings/' + group.value.id);
  }

  async function deleteGroup(id) {
    const name = groups.value?.find((g) => g.id === id)?.name ?? 'this group';
    if (!confirm(`Delete ${name}?`)) return;
    await api.adminDeleteGroup(id);
    invalidateGroups(); // reset stats
    await loadGroups();
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
            items="${groups.value}"
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

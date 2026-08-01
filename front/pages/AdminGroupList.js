import { html } from 'htm/preact';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { selectedGroup, groups, invalidateGroups, loadGroups } from '../store/admin.js';
import { Content } from '../component/Content.js';
import { AdminNav } from '../component/UserNav.js';
import { Drawer } from '../component/Drawer.js';
import { ListTable } from '../component/ListTable.js';
import { Loader } from '../component/Loader.js';
import { GroupAddIcon } from '../svg/GroupAddIcon.js';
import { confirm } from '../component/ConfirmDialog.js';

export default function AdminGroupList() {
  const { route } = useLocation();
  const { isLoading } = useLoader(loadGroups, Array.isArray(groups.value));

  useTitle('Group List');

  function updateGroup(group) {
    selectedGroup.value = group;
    route('/admin/group-settings/' + selectedGroup.value.id);
  }

  async function deleteGroup(id) {
    const name = groups.value?.find((g) => g.id === id)?.name ?? 'this group';
    if (!(await confirm(`Delete ${name} group?`))) return;
    await api.adminDeleteGroup(id);
    invalidateGroups(); // reset stats
    await loadGroups();
  }

  return html`
    <${AdminNav} />

    <${Content}>
      ${
        isLoading
          ? html`<${Loader} />`
          : html`<${ListTable}
              title="List of groups"
              icon="${html`<title>Add group</title> <${GroupAddIcon} />`}"
              addLink="/admin/new-group"
              columns="${[{ label: 'Name', key: 'name' }]}"
              items="${groups.value}"
              searchKeys="${['name']}"
              renderRow="${(g) => html`
                <tr key=${g.id}>
                  <td>${g.name}</td>
                  <td>
                    <div class="pure-g">
                      <div class="table-button pure-u-1 pure-u-sm-1-3">
                        ${
                          (!!g.users?.length || !!g.videos?.length) &&
                          html`<${Drawer}
                            label="Users and Rights"
                            items=${[
                              { label: 'Group Users', elements: g.users.map((u) => u.name) ?? [] },
                              { label: 'Video Rights', elements: g.videos.map((v) => v.title) ?? [] },
                            ]}
                          />`
                        }
                      </div>
                      <div class="table-button pure-u-1 pure-u-sm-1-3">
                        <a onClick=${() => updateGroup(g)}>Update</a>
                      </div>
                      <div class="table-button pure-u-1 pure-u-sm-1-3">
                        <a onClick=${() => deleteGroup(g.id)}>Delete</a>
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

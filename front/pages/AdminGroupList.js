import { html } from 'htm/preact';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { selectedGroup, groups, loadGroups, invalidateGroups } from '../store/admin.js';
import { Content } from '../component/Content.js';
import { AdminNav } from '../component/UserNav.js';
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
              addContent="${html`<svg class="svg-button"><${GroupAddIcon} /></svg> New group`}"
              addLink="/admin/new-group"
              columns="${[{ key: 'name', label: 'Name', sortValue: (g) => g.name }]}"
              items="${groups.value}"
              searchKeys="${['name']}"
              renderExpanded="${(g) => html`
                <div class="list-expanded-actions">
                  <a onClick=${() => updateGroup(g)}>Update</a>
                  <a onClick=${() => deleteGroup(g.id)}>Delete</a>
                </div>
                ${
                  (!!g.users?.length || !!g.videos?.length) &&
                  html`
                    <div class="list-expanded-info">
                      ${
                        !!g.users?.length &&
                        html`
                          <div class="list-expanded-info-row">
                            <h4>Users</h4>
                            ${g.users?.map((u) => html`<p>${u.name}</p>`)}
                          </div>
                        `
                      }
                      ${
                        !!g.videos?.length &&
                        html`
                          <div class="list-expanded-info-row">
                            <h4>Video Rights</h4>
                            ${g.videos?.map((v) => html`<p>${v.title}</p>`)}
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

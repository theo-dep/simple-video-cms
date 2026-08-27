import { html } from 'htm/preact';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { users, videos, loadUsers, loadVideos, invalidateGroups } from '../store/admin.js';
import { AdminNav } from '../component/HeaderNav.js';
import { Form } from '../component/Form.js';
import { MultiSelectDropDown } from '../component/SelectDropDown.js';
import { Loader } from '../component/Loader.js';
import { validateText } from '../utils/validation.js';

export default function AdminNewGroup() {
  const { route } = useLocation();
  const { isLoading: isUsersLoading } = useLoader(loadUsers, Array.isArray(users.value));
  const { isLoading: isVideosLoading } = useLoader(loadVideos, Array.isArray(videos.value));

  useTitle('New Group');

  async function onGroupSubmit(e) {
    const form = e.target;
    const name = form.elements['name'].value.trim();
    validateText(name);

    const userSelect = form.elements['user-ids'];
    const userIds = userSelect ? Array.from(userSelect.selectedOptions).map((o) => o.value) : [];

    const videoSelect = form.elements['video-ids'];
    const videoIds = videoSelect ? Array.from(videoSelect.selectedOptions).map((o) => o.value) : [];

    await api.adminAddGroup(name, userIds, videoIds);
    invalidateGroups(); // force to refresh the group list and stats
    route('/admin/group-list');
  }

  return html`
    <${AdminNav} />

    ${
      isUsersLoading || isVideosLoading
        ? html`<${Loader} />`
        : html`
            <${Form} title="Add a new group" buttonTitle="Create" onSubmitAction=${onGroupSubmit}>
              <div class="form-control-group">
                <input class="input" type="text" name="name" placeholder="name" required autofocus />
              </div>
              ${
                !!users.value.length &&
                html`
                  <div class="form-control-group">
                    <${MultiSelectDropDown} name="user-ids" placeholder="Add users to group (optional)">
                      ${users.value.map((u) => html`<option key=${u.id} value=${u.id}>${u.name}</option>`)}
                    <//>
                  </div>
                `
              }
              ${
                !!videos.value.length &&
                html`
                  <div class="form-control-group">
                    <${MultiSelectDropDown} name="video-ids" placeholder="Add videos to group (optional)">
                      ${videos.value.map((v) => html`<option key=${v.id} value=${v.id}>${v.title}</option>`)}
                    <//>
                  </div>
                `
              }
            <//>
          `
    }
  `;
}

import { html } from 'htm/preact';
import { useState } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { useSearch } from '../hook/useSearch.js';
import { refreshRequested } from '../store/auth.js';
import { groups, users, videos, loadGroups, loadUsers, loadVideos, invalidateVideos } from '../store/admin.js';
import { AdminNav } from '../component/UserNav.js';
import { Form } from '../component/Form.js';
import { MultiSelectDropDown } from '../component/MultiSelectDropDown.js';
import { Loader } from '../component/Loader.js';
import { Dropdown } from '../component/DropDown.js';
import { validateField } from '../utils/validation.js';

export default function AdminNewVideo() {
  const { route } = useLocation();
  const [fileName, setFileName] = useState('');
  const [title, setTitle] = useState('');
  const { results, search } = useSearch(videos.value, ['title']);
  const { isLoading: isGroupsLoading } = useLoader(loadGroups, Array.isArray(groups.value));
  const { isLoading: isUsersLoading } = useLoader(loadUsers, Array.isArray(users.value));
  const { isLoading: isVideosLoading } = useLoader(loadVideos, Array.isArray(videos.value));

  useTitle('New Video');

  function onFileChange(e) {
    const file = e.target.files[0];
    setFileName(file.name);

    if (!title.value) {
      setTitle(file.name.replace(/\.[^/.]+$/, ''));
    }
    e.target.closest('.file-drop-area')?.classList.remove('is-active');
  }

  function onTitleInput(e) {
    const value = e.target.value;
    setTitle(value);
    search(value);
  }

  async function onVideoSubmit(e) {
    const form = e.target;
    const fileInput = form.elements['file'];
    const video = fileInput.files[0] || null;
    const title = form.elements['title'].value.trim();
    validateField(title);

    const groupSelect = form.elements['group-ids'];
    const userSelect = form.elements['user-ids'];
    const groupIds = groupSelect ? Array.from(groupSelect.selectedOptions).map((o) => o.value) : [];
    const userIds = userSelect ? Array.from(userSelect.selectedOptions).map((o) => o.value) : [];
    await api.adminAddVideo(video, title, groupIds, userIds);
    invalidateVideos(); // force to refresh the video list and stats
    refreshRequested.value = true; // add this video to the admin video list (current user)
    route('/admin/video-list');
  }

  return html`
    <${AdminNav} />

    ${
      isGroupsLoading || isUsersLoading
        ? html`<${Loader} />`
        : html`
            <${Form} title="Upload video" buttonTitle="Upload" onSubmitAction=${onVideoSubmit}>
              <div
                class="pure-control-group file-drop-area"
                onDragEnter=${(e) => e.currentTarget.classList.add('is-active')}
                onDragLeave=${(e) => e.currentTarget.classList.remove('is-active')}
                onDrop=${(e) => e.currentTarget.classList.remove('is-active')}
              >
                <span class="fake-button">Choose file</span>
                <span class="file-message">${fileName || 'or drag a video here'}</span>
                <input
                  class="pure-input-1 file-input"
                  type="file"
                  accept="video/mp4,video/webm,video/ogg,video/quicktime"
                  name="file"
                  onChange=${onFileChange}
                  required
                />
              </div>
              <div class="pure-control-group">
                <input
                  onInput=${onTitleInput}
                  value="${title}"
                  class="pure-input-1"
                  type="text"
                  name="title"
                  id="title"
                  placeholder="Video title"
                  required
                />
              </div>

              ${
                !isVideosLoading &&
                title &&
                !!results.length &&
                html` <div class="pure-control-group">
                  <${Dropdown}
                    title="Similar videos"
                    liElements=${results.slice(0, 3).map(
                      (v) => html`
                        <li>
                          <a href=${'/video/' + v.id} target="_blank">${v.title}</a>
                        </li>
                      `
                    )}
                  />
                </div>`
              }
              ${
                !!groups.value.length &&
                html`
                  <div class="pure-control-group">
                    <${MultiSelectDropDown} name="group-ids" placeholder="Add groups to video (optional)">
                      ${groups.value.map((g) => html`<option key=${g.id} value=${g.id}>${g.name}</option>`)}
                    <//>
                  </div>
                `
              }
              ${
                !!users.value.length &&
                html`
                  <div class="pure-control-group">
                    <${MultiSelectDropDown} name="user-ids" placeholder="Add users to video (optional)">
                      ${users.value.map((u) => html`<option key=${u.id} value=${u.id}>${u.name}</option>`)}
                    <//>
                  </div>
                `
              }
            <//>
          `
    }
  `;
}

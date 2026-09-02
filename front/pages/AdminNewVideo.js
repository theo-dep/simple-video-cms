import { html } from 'htm/preact';
import { useState } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { useSearch } from '../hook/useSearch.js';
import { refreshRequested } from '../store/auth.js';
import {
  groups,
  users,
  locations,
  authors,
  tags,
  videos,
  loadGroups,
  loadUsers,
  loadLocations,
  loadAuthors,
  loadTags,
  loadVideos,
  invalidateVideos,
  onAddedLocation,
  onEditLocation,
  onDeletedLocation,
  onAddedAuthor,
  onEditAuthor,
  onDeletedAuthor,
  onAddedTag,
  onEditTag,
  onDeletedTag,
} from '../store/admin.js';
import { AdminNav } from '../component/HeaderNav.js';
import { Form } from '../component/Form.js';
import { MultiSelectDropDown, SingleSelectEditableDropDown, MultiSelectEditableDropDown } from '../component/SelectDropDown.js';
import { Loader } from '../component/Loader.js';
import { Dropdown } from '../component/DropDown.js';
import { RestrictedInput } from '../component/RestrictedInput.js';
import { validateText, validateDate, TEXT_VALIDATION_TOOLTIP, DATE_VALIDATION_TOOLTIP } from '../utils/validation.js';
import { formatVideo } from '../utils/formatVideo.js';

export default function AdminNewVideo() {
  const { route } = useLocation();
  const [fileName, setFileName] = useState('');
  const [title, setTitle] = useState('');
  const { results, search } = useSearch(videos.value, ['title']);
  const { isLoading: isLocationsLoading } = useLoader(loadLocations, Array.isArray(locations.value));
  const { isLoading: isAuthorsLoading } = useLoader(loadAuthors, Array.isArray(authors.value));
  const { isLoading: isTagsLoading } = useLoader(loadTags, Array.isArray(tags.value));
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
    validateText(title);
    const date = form.elements['date'].value.trim();
    validateDate(date);
    const locationSelect = form.elements['location-id'];
    const locationId = locationSelect ? locationSelect.selectedOptions[0]?.value || null : null;
    const authorSelect = form.elements['author-ids'];
    const authorIds = authorSelect ? Array.from(authorSelect.selectedOptions).map((o) => o.value) : [];
    const tagSelect = form.elements['tag-ids'];
    const tagIds = tagSelect ? Array.from(tagSelect.selectedOptions).map((o) => o.value) : [];
    const groupSelect = form.elements['group-ids'];
    const groupIds = groupSelect ? Array.from(groupSelect.selectedOptions).map((o) => o.value) : [];
    const userSelect = form.elements['user-ids'];
    const userIds = userSelect ? Array.from(userSelect.selectedOptions).map((o) => o.value) : [];

    await api.adminAddVideo(video, title, date, locationId, authorIds, tagIds, groupIds, userIds);
    invalidateVideos(); // force to refresh the video list and stats
    refreshRequested.value = true; // add this video to the admin video list (current user)
    route('/admin/video-list');
  }

  return html`
    <${AdminNav} />

    ${
      isLocationsLoading || isAuthorsLoading || isTagsLoading || isGroupsLoading || isUsersLoading
        ? html`<${Loader} />`
        : html`
            <${Form} title="Upload video" buttonTitle="Upload" onSubmitAction=${onVideoSubmit}>
              <div
                class="form-control-group file-drop-area"
                onDragEnter=${(e) => e.currentTarget.classList.add('is-active')}
                onDragLeave=${(e) => e.currentTarget.classList.remove('is-active')}
                onDrop=${(e) => e.currentTarget.classList.remove('is-active')}
              >
                <span class="fake-button">Choose file</span>
                <span class="file-message">${fileName || 'or drag a video here'}</span>
                <input
                  class="input-1 file-input"
                  type="file"
                  accept="video/mp4,video/webm,video/ogg,video/quicktime"
                  name="file"
                  onChange=${onFileChange}
                  required
                />
              </div>
              <div class="form-control-group">
                <${RestrictedInput}
                  onInput=${onTitleInput}
                  value="${title}"
                  name="title"
                  id="title"
                  placeholder="Video title"
                  tooltip=${TEXT_VALIDATION_TOOLTIP}
                  required
                />
              </div>

              ${
                !isVideosLoading &&
                title &&
                !!results.length &&
                html` <div class="form-control-group">
                  <${Dropdown}
                    title="Similar videos"
                    liElements=${results.slice(0, 3).map(
                      (v) => html`
                        <li>
                          <a href=${'/video/' + v.id} target="_blank">${formatVideo(v)}</a>
                        </li>
                      `
                    )}
                  />
                </div>`
              }

              <div class="form-control-group">
                <${RestrictedInput} name="date" id="date" placeholder="Video date (optional)" tooltip=${DATE_VALIDATION_TOOLTIP} />
              </div>

              <div class="form-control-group">
                <${SingleSelectEditableDropDown}
                  name="location-id"
                  placeholder="Video location (optional)"
                  onAddedOption=${onAddedLocation}
                  onDeletedOption=${onDeletedLocation}
                  onEditOption=${onEditLocation}
                >
                  ${locations.value.map((p) => html`<option key=${p.id} value=${p.id}>${p.name}</option>`)}
                <//>
              </div>

              <div class="form-control-group">
                <${MultiSelectEditableDropDown}
                  name="author-ids"
                  placeholder="Add authors to video (optional)"
                  onAddedOption=${onAddedAuthor}
                  onDeletedOption=${onDeletedAuthor}
                  onEditOption=${onEditAuthor}
                >
                  ${authors.value.map((a) => html`<option key=${a.id} value=${a.id}>${a.name}</option>`)}
                <//>
              </div>

              <div class="form-control-group">
                <${MultiSelectEditableDropDown}
                  name="tag-ids"
                  placeholder="Add tags to video (optional)"
                  onAddedOption=${onAddedTag}
                  onDeletedOption=${onDeletedTag}
                  onEditOption=${onEditTag}
                >
                  ${tags.value.map((t) => html`<option key=${t.id} value=${t.id}>${t.name}</option>`)}
                <//>
              </div>

              ${
                !!groups.value.length &&
                html`
                  <div class="form-control-group">
                    <${MultiSelectDropDown} name="group-ids" placeholder="Add groups to video (optional)">
                      ${groups.value.map((g) => html`<option key=${g.id} value=${g.id}>${g.name}</option>`)}
                    <//>
                  </div>
                `
              }
              ${
                !!users.value.length &&
                html`
                  <div class="form-control-group">
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

import { html } from 'htm/preact';
import { useEffect, useRef, useState } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { useSearch } from '../hook/useSearch.js';
import { refreshRequested } from '../store/auth.js';
import {
  selectedVideo,
  groups,
  users,
  locations,
  authors,
  tags,
  videos,
  loadVideo,
  loadGroups,
  loadUsers,
  loadLocations,
  loadAuthors,
  loadTags,
  loadVideos,
  invalidateAdminLists,
  onAddedLocation,
  onDeletedLocation,
  onAddedAuthor,
  onDeletedAuthor,
  onAddedTag,
  onDeletedTag,
} from '../store/admin.js';
import { AdminNav } from '../component/HeaderNav.js';
import { Form } from '../component/Form.js';
import { MultiSelectDropDown, SingleSelectEditableDropDown, MultiSelectEditableDropDown } from '../component/SelectDropDown.js';
import { Loader } from '../component/Loader.js';
import { Dropdown } from '../component/DropDown.js';
import { confirm } from '../component/ConfirmDialog.js';
import { validateField } from '../utils/validation.js';
import { formatVideo } from '../utils/formatVideo.js';

export default function AdminUpdateVideo({ videoId }) {
  videoId = Number(videoId);
  const { route } = useLocation();
  const [title, setTitle] = useState('');
  const { results, search } = useSearch(videos.value, ['title']);
  const dateRef = useRef(null);
  const { isLoading: isVideoLoading } = useLoader(() => loadVideo(videoId), selectedVideo.value?.id === videoId, [videoId]);
  const { isLoading: isLocationsLoading } = useLoader(loadLocations, Array.isArray(locations.value));
  const { isLoading: isAuthorsLoading } = useLoader(loadAuthors, Array.isArray(authors.value));
  const { isLoading: isTagsLoading } = useLoader(loadTags, Array.isArray(tags.value));
  const { isLoading: isGroupsLoading } = useLoader(loadGroups, Array.isArray(groups.value));
  const { isLoading: isUsersLoading } = useLoader(loadUsers, Array.isArray(users.value));
  const { isLoading: isVideosLoading } = useLoader(loadVideos, Array.isArray(videos.value));

  useTitle(`${selectedVideo.value?.title || 'Video'} Settings`);

  const isLoading = isVideoLoading || isLocationsLoading || isAuthorsLoading || isTagsLoading || isGroupsLoading || isUsersLoading;

  useEffect(() => {
    if (selectedVideo.value && dateRef.current && !isVideoLoading) {
      setTitle(selectedVideo.value.title);
      dateRef.current.value = selectedVideo.value.date ?? '';
    }
  }, [selectedVideo.value, dateRef.current, isVideoLoading, isLoading]);

  function isLocationSelected(locationId) {
    return selectedVideo.value?.location?.id === locationId;
  }

  function isAuthorSelected(authorId) {
    return selectedVideo.value?.authors?.find((a) => a.id === authorId);
  }

  function isTagSelected(tagId) {
    return selectedVideo.value?.tags?.find((t) => t.id === tagId);
  }

  function isGroupSelected(groupId) {
    return selectedVideo.value?.groups?.find((g) => g.id === groupId);
  }

  function isUserSelected(userId) {
    return selectedVideo.value?.users?.find((u) => u.id === userId);
  }

  function onTitleInput(e) {
    const value = e.target.value;
    setTitle(value);
    search(value);
  }

  async function onVideoSubmit(e) {
    const form = e.target;
    const title = form.elements['title'].value.trim();
    validateField(title);

    const oldTitle = selectedVideo.value?.title ?? 'this video';
    const confirmMessage = title === oldTitle ? `Update ${title} video?` : `Update ${oldTitle} video name to ${title}?`;
    if (!(await confirm(confirmMessage))) return;

    const date = form.elements['date'].value.trim();
    const locationSelect = form.elements['location-id'];
    const locationId = locationSelect ? locationSelect.value : null;
    const authorSelect = form.elements['author-ids'];
    const authorIds = authorSelect ? Array.from(authorSelect.selectedOptions).map((o) => o.value) : [];
    const tagSelect = form.elements['tag-ids'];
    const tagIds = tagSelect ? Array.from(tagSelect.selectedOptions).map((o) => o.value) : [];
    const groupSelect = form.elements['group-ids'];
    const groupIds = groupSelect ? Array.from(groupSelect.selectedOptions).map((o) => o.value) : [];
    const userSelect = form.elements['user-ids'];
    const userIds = userSelect ? Array.from(userSelect.selectedOptions).map((o) => o.value) : [];

    await api.adminUpdateVideo(videoId, title, date, locationId, authorIds, tagIds, groupIds, userIds);
    invalidateAdminLists(); // force to refresh the video, user and group list
    refreshRequested.value = true; // update this video to the admin video list (current user)
    route('/admin/video-list');
  }

  return html`
    <${AdminNav} />

    ${
      isLoading
        ? html`<${Loader} />`
        : html`
            <${Form} title="Change video title, group and user rights" buttonTitle="Update" onSubmitAction=${onVideoSubmit}>
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
                    open="${false}"
                    liElements=${results
                      .filter((v) => v.title !== selectedVideo.value?.title)
                      .slice(0, 3)
                      .map(
                        (v) => html`
                          <li>
                            <a href=${'/video/' + v.id} target="_blank">${formatVideo(v)}</a>
                          </li>
                        `
                      )}
                  />
                </div>`
              }

              <div class="pure-control-group">
                <input ref=${dateRef} class="pure-input-1" type="text" name="date" id="date" placeholder="Video date (optional)" />
              </div>

              <div class="pure-control-group">
                <${SingleSelectEditableDropDown}
                  name="location-id"
                  placeholder="Video location (optional)"
                  onAddedOption=${onAddedLocation}
                  onDeletedOption=${onDeletedLocation}
                >
                  ${locations.value.map((p) => html`<option key=${p.id} value=${p.id} selected=${isLocationSelected(p.id)}>${p.name}</option>`)}
                <//>
              </div>

              <div class="pure-control-group">
                <${MultiSelectEditableDropDown}
                  name="author-ids"
                  placeholder="Add authors to video (optional)"
                  onAddedOption=${onAddedAuthor}
                  onDeletedOption=${onDeletedAuthor}
                >
                  ${authors.value.map((a) => html`<option key=${a.id} value=${a.id} selected=${isAuthorSelected(a.id)}>${a.name}</option>`)}
                <//>
              </div>

              <div class="pure-control-group">
                <${MultiSelectEditableDropDown}
                  name="tag-ids"
                  placeholder="Add tags to video (optional)"
                  onAddedOption=${onAddedTag}
                  onDeletedOption=${onDeletedTag}
                >
                  ${tags.value.map((t) => html`<option key=${t.id} value=${t.id} selected=${isTagSelected(t.id)}>${t.name}</option>`)}
                <//>
              </div>

              ${
                !!groups.value.length &&
                html`
                  <div class="pure-control-group">
                    <${MultiSelectDropDown} name="group-ids" placeholder="Add groups to video (optional)">
                      ${groups.value.map((g) => html`<option key=${g.id} value=${g.id} selected=${isGroupSelected(g.id)}>${g.name}</option>`)}
                    <//>
                  </div>
                `
              }
              ${
                !!users.value.length &&
                html`
                  <div class="pure-control-group">
                    <${MultiSelectDropDown} name="user-ids" placeholder="Add users to video (optional)">
                      ${users.value.map((u) => html`<option key=${u.id} value=${u.id} selected=${isUserSelected(u.id)}>${u.name}</option>`)}
                    <//>
                  </div>
                `
              }
            <//>
          `
    }
  `;
}

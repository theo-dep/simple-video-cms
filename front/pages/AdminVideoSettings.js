import { html } from 'htm/preact';
import { useState, useEffect } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { selectedItem } from '../store/selection.js';
import { AdminNav } from '../component/UserNav.js';
import { Form, useMultiSelect } from '../component/Form.js';

export default function AdminUpdateVideo({ videoId }) {
  videoId = Number(videoId);
  const { route } = useLocation();
  const [video, setVideo] = useState(selectedItem.value);
  const [groups, setGroups] = useState(null);
  const [users, setUsers] = useState(null);
  const selectGroupsRef = useMultiSelect([video, groups]);
  const selectUsersRef = useMultiSelect([video, users]);

  useTitle(video ? `${video.title} Settings` : 'Video Settings');

  useEffect(() => {
    if (video === null || video.id !== videoId) {
      api
        .adminVideo(videoId)
        .then((r) => setVideo(r.json ?? r))
        .catch(() => route('/403'));
    }

    if (!Array.isArray(groups)) {
      api
        .adminGroupList()
        .then((g) => setGroups(g.json ?? g))
        .catch(() => route('/403'));
    }

    if (!Array.isArray(users)) {
      api
        .adminUserList()
        .then((u) => setUsers(u.json ?? u))
        .catch(() => route('/403'));
    }
  }, [videoId]);

  function isGroupSelected(groupId) {
    return video.groups.find((g) => g.id === groupId);
  }

  function isUserSelected(userId) {
    return video.users.find((u) => u.id === userId);
  }

  async function onVideoSubmit(e) {
    if (!confirm('Update this video?')) return;

    const form = e.target;
    const title = form.elements['title'].value.trim();
    const groupSelect = form.elements['group-ids'];
    const userSelect = form.elements['user-ids'];
    const groupIds = groupSelect ? Array.from(groupSelect.selectedOptions).map((o) => o.value) : [];
    const userIds = userSelect ? Array.from(userSelect.selectedOptions).map((o) => o.value) : [];

    await api.adminUpdateVideo(videoId, title, groupIds, userIds);
    route('/admin/video-list');
  }

  return html`
    <${AdminNav} />

    ${video !== null &&
    Array.isArray(users) &&
    Array.isArray(groups) &&
    html`
      <${Form} title="Change video title, group and user rights" buttonTitle="Update" onSubmitAction=${onVideoSubmit}>
        <div class="pure-control-group">
          <input class="pure-input-1" type="text" name="title" value=${video.title} placeholder="Video title" required />
        </div>
        ${!!groups.length &&
        html`
          <div class="pure-control-group">
            <select
              ref=${selectGroupsRef}
              name="group-ids"
              class="pure-input-1"
              data-placeholder="Select groups (optional)"
              multiple
              data-multi-select
            >
              ${groups.map((g) => html`<option key=${g.id} value=${g.id} selected=${isGroupSelected(g.id)}>${g.name}</option>`)}
            </select>
          </div>
        `}
        ${!!users.length &&
        html`
          <div class="pure-control-group">
            <select ref=${selectUsersRef} name="user-ids" class="pure-input-1" data-placeholder="Select users (optional)" multiple data-multi-select>
              ${users.map((u) => html`<option key=${u.id} value=${u.id} selected=${isUserSelected(u.id)}>${u.name}</option>`)}
            </select>
          </div>
        `}
      <//>
    `}
  `;
}

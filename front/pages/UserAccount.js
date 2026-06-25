import { html } from 'htm/preact';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { user, refreshAuth } from '../store/auth.js';
import { UserNav } from '../component/UserNav.js';
import { InfoContent } from '../component/InfoContent.js';
import { FormContent } from '../component/Form.js';

export default function UserAccount() {
  useTitle(`${user.name.value} Account`);

  async function onUsernameSubmit(e) {
    const form = e.target;
    const username = form.elements['username'].value.trim();
    const password = form.elements['password'].value;

    await api.updateUsername(username, password);
    await refreshAuth(); // refresh the current user id
    form.reset();
    form.elements['username'].value = username;
  }

  async function onPasswordSubmit(e) {
    const form = e.target;
    const oldPassword = form.elements['old-password'].value;
    const newPassword = form.elements['new-password'].value;
    const confirmPassword = form.elements['confirm-password'].value;

    await api.updatePassword(oldPassword, newPassword, confirmPassword);
    form.reset();
  }

  return html`
    <${UserNav} />

    <${InfoContent}>
      <${FormContent} title="Change username" buttonTitle="Update" onSubmitAction=${onUsernameSubmit}>
        <div class="pure-control-group">
          <input class="pure-input-1" type="text" name="username" placeholder="username" value=${user.name.value} required />
        </div>
        <div class="pure-control-group">
          <input class="pure-input-1" type="password" name="password" placeholder="confirm password" required autofocus />
        </div>
      <//>

      <${FormContent} title="Change password" buttonTitle="Update" onSubmitAction=${onPasswordSubmit}>
        <div class="pure-control-group">
          <input class="pure-input-1" type="password" name="old-password" placeholder="old password" required />
        </div>
        <div class="pure-control-group">
          <input class="pure-input-1" type="password" name="new-password" placeholder="new password" required />
        </div>
        <div class="pure-control-group">
          <input class="pure-input-1" type="password" name="confirm-password" placeholder="confirm new password" required />
        </div>
      <//>
    <//>
  `;
}

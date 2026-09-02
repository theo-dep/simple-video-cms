import { html } from 'htm/preact';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { user, refreshRequested } from '../store/auth.js';
import { UserNav } from '../component/HeaderNav.js';
import { InfoContent } from '../component/InfoContent.js';
import { FormContent } from '../component/Form.js';
import { PasswordInput } from '../component/PasswordInput.js';
import { RestrictedInput } from '../component/RestrictedInput.js';
import { validateText, TEXT_VALIDATION_TOOLTIP } from '../utils/validation.js';

export default function UserAccount() {
  useTitle(`${user.name.value} Account`);

  async function onUsernameSubmit(e) {
    const form = e.target;
    const username = form.elements['username'].value.trim();
    const password = form.elements['password'].value;
    validateText(username);

    await api.updateUsername(username, password);
    refreshRequested.value = true; // refresh the current user id
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
      <${FormContent} title="Change username" buttonTitle="Update" successMessage="Username updated!" onSubmitAction=${onUsernameSubmit}>
        <div class="form-control-group">
          <${RestrictedInput} name="username" placeholder="Username" value=${user.name.value} tooltip=${TEXT_VALIDATION_TOOLTIP} required />
        </div>
        <div class="form-control-group">
          <${PasswordInput} name="password" placeholder="confirm password" autofocus />
        </div>
      <//>

      <${FormContent} title="Change password" buttonTitle="Update" successMessage="Password updated!" onSubmitAction=${onPasswordSubmit}>
        <div class="form-control-group">
          <${PasswordInput} name="old-password" placeholder="old password" />
        </div>
        <div class="form-control-group">
          <${PasswordInput} name="new-password" placeholder="new password" />
        </div>
        <div class="form-control-group">
          <${PasswordInput} name="confirm-password" placeholder="confirm new password" />
        </div>
      <//>
    <//>
  `;
}

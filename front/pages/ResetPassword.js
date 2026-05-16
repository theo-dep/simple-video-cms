import { html } from 'htm/preact';
import { useState } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { InfoContent } from '../component/InfoContent.js';
import { UserNav } from '../component/UserNav.js';
import { PasswordInput } from '../component/PasswordInput.js';
import { SubmitButton } from '../component/SubmitButton.js';
import { Alert } from '../component/Alert.js';

export default function ResetPassword({ username = '' }) {
  const { route } = useLocation();
  const [alert, setAlert] = useState('');
  const [loading, setLoading] = useState(false);

  useTitle(`Reset ${username} Password`);

  async function onSubmit(e) {
    e.preventDefault();
    const form = e.target;
    const uname = form.elements['username'].value;
    const password = form.elements['password'].value;
    const confirm = form.elements['confirm-password'].value;

    setLoading(true);
    setAlert('');
    try {
      await api.addPassword(uname, password, confirm);
      route('/login');
    } catch (err) {
      setAlert(err.message || 'Failed to set password');
    } finally {
      setLoading(false);
    }
  }

  return html`
    <${UserNav} />

    <${InfoContent}>
      <h3>Set a new password</h3>

      <form class="pure-form pure-form-aligned" onSubmit=${onSubmit}>
        <fieldset>
          <${Alert} message=${alert} />
          <div class="pure-control-group">
            <input class="pure-input-1" type="text" name="username" placeholder="username" value=${username} required />
          </div>
          <div class="pure-control-group">
            <${PasswordInput} name="password" placeholder="password" autofocus />
          </div>
          <div class="pure-control-group">
            <${PasswordInput} name="confirm-password" placeholder="confirm password" />
          </div>
          <div class="pure-control-group">
            <${SubmitButton} label="Create" loading=${loading} id="create-button" />
          </div>
        </fieldset>
      </form>
    <//>
  `;
}

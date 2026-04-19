import { html } from 'htm/preact';
import { useState } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { refreshAuth } from '../store/auth.js';
import { videoIdRedirected } from '../store/redirect.js';
import { api } from '../api.js';
import { InfoContent } from '../component/InfoContent.js';
import { UserNav } from '../component/UserNav.js';
import { PasswordInput } from '../component/PasswordInput.js';
import { SubmitButton } from '../component/SubmitButton.js';
import { Alert } from '../component/Alert.js';

export default function Login() {
  const { route } = useLocation();
  const [alert, setAlert] = useState('');
  const [loading, setLoading] = useState(false);
  const [showForgot, setShowForgot] = useState(false);

  async function onLogin(e) {
    e.preventDefault();
    const form = e.target;
    const username = form.elements['username'].value;
    const password = form.elements['password'].value;

    setLoading(true);
    setAlert('');
    try {
      const { status } = await api.login(username, password);
      if (status === 204) {
        route('/add-password/' + encodeURIComponent(username));
        return;
      }
      await refreshAuth();
      if (videoIdRedirected.value !== '') {
        route('/watch-video/' + videoIdRedirected.value);
        videoIdRedirected.value = '';
      } else {
        route('/');
      }
    } catch (err) {
      setAlert(err.message || 'Login failed');
    } finally {
      setLoading(false);
    }
  }

  async function onFirstConnection(e) {
    e.preventDefault();
    const form = e.target;
    const username = form.elements['username'].value;
    route('/add-password' + (username ? '/' + encodeURIComponent(username) : ''));
  }

  return html`
    <${UserNav} />

    <${InfoContent}>
      <form class="pure-form pure-form-aligned" onSubmit=${onLogin}>
        <fieldset>
          <${Alert} message=${alert} />
          <div class="pure-control-group">
            <input class="pure-input-1" type="text" name="username" placeholder="username" required autofocus />
          </div>
          <div class="pure-control-group">
            <${PasswordInput} name="password" placeholder="password" />
          </div>
          <div class="pure-control-group">
            <${SubmitButton} label="Login" loading=${loading} id="login-button" />
          </div>
        </fieldset>
      </form>

      <div class="add-user">
        <a onClick=${() => setShowForgot(!showForgot)}>Forgot password?</a>
      </div>
      <div class="add-user" style="visibility: ${showForgot ? 'visible' : 'hidden'}">Ask an admin to reset your password</div>

      <h4 class="lined">Or</h4>
      <h3>First connection?</h3>

      <form class="pure-form pure-form-aligned" onSubmit=${onFirstConnection}>
        <div class="pure-control-group">
          <input class="pure-input-1" type="text" name="username" placeholder="enter a valid username" />
        </div>
        <div class="pure-control-group">
          <${SubmitButton} label="Create" loading=${loading} id="add-password-button" />
        </div>
      </form>
    <//>
  `;
}

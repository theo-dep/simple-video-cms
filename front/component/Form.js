import { html } from 'htm/preact';
import { useState } from 'preact/hooks';
import { InfoContent } from './InfoContent.js';
import { SubmitButton } from './SubmitButton.js';
import { Alert } from './Alert.js';

export function FormContent({ title, buttonTitle, successMessage, onSubmitAction, children }) {
  const [loading, setLoading] = useState(false);
  const [alert, setAlert] = useState('');

  async function onSubmit(e) {
    e.preventDefault();

    setLoading(true);
    setAlert('');
    try {
      await onSubmitAction(e);
      if (successMessage) setAlert(successMessage);
    } catch (err) {
      setAlert(err.message || 'Unknown error');
    } finally {
      setLoading(false);
    }
  }

  return html`
    <h3>${title}</h3>

    <form class="form" onSubmit=${onSubmit}>
      <fieldset>
        <${Alert} message=${alert} />
        ${children}
        <div class="form-control-group">
          <${SubmitButton} label="${buttonTitle}" loading=${loading} id="submit-button" />
        </div>
      </fieldset>
    </form>
  `;
}

export function Form({ title, buttonTitle, successMessage, onSubmitAction, children }) {
  return html`
    <${InfoContent}>
      <${FormContent} title=${title} buttonTitle=${buttonTitle} successMessage=${successMessage} onSubmitAction=${onSubmitAction}> ${children} <//>
    <//>
  `;
}

import { render } from 'preact';
import { html } from 'htm/preact';
import { App } from './component/App.js';

import './console.js';

render(html`<${App} />`, document.body);

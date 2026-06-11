import { render } from 'preact';
import { html } from 'htm/preact';
import { App } from './component/App.js';

import './console.js';

const headRoot = document.getElementById('head-root');
headRoot.remove();

const root = document.getElementById('root');
root.remove();

render(html`<${App} />`, document.body);

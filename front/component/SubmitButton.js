import { html } from 'htm/preact';

const spinnerSvg = html`
  <svg fill="currentColor" width="24" height="24" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
    <style>
      .sp9y7u {
        animation: spfUkk 4.8s linear infinite;
        animation-delay: -4.8s;
      }
      .spDF2s {
        animation-delay: -3.2s;
      }
      .spq27e {
        animation-delay: -1.6s;
      }
      @keyframes spfUkk {
        8.33% {
          x: 13px;
          y: 1px;
        }
        25% {
          x: 13px;
          y: 1px;
        }
        33.3% {
          x: 13px;
          y: 13px;
        }
        50% {
          x: 13px;
          y: 13px;
        }
        58.33% {
          x: 1px;
          y: 13px;
        }
        75% {
          x: 1px;
          y: 13px;
        }
        83.33% {
          x: 1px;
          y: 1px;
        }
      }
    </style>
    <rect class="sp9y7u" x="1" y="1" rx="1" width="10" height="10" />
    <rect class="sp9y7u spDF2s" x="1" y="1" rx="1" width="10" height="10" />
    <rect class="sp9y7u spq27e" x="1" y="1" rx="1" width="10" height="10" />
  </svg>
`;

export function SubmitButton({ label, loading, id = 'submit-btn' }) {
  function handleClick(e) {
    const form = e.target.closest('form');
    if (form) form.requestSubmit();
  }

  return html`
    <div class="pure-form">
      <button
        id=${id}
        class=${'pure-input-1 button pure-button' + (loading ? ' button-spinner' : '')}
        disabled=${loading}
        type="button"
        onClick=${handleClick}
      >
        ${loading ? spinnerSvg : label}
      </button>
    </div>
  `;
}

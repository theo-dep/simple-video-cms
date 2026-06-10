import { api } from './api.js';

const originalConsole = {
  log: console.log.bind(console),
  error: console.error.bind(console),
  warn: console.warn.bind(console),
  info: console.info.bind(console),
};

function serializeArg(arg) {
  if (arg instanceof Error) {
    return `${arg.name}: ${arg.message}${arg.stack ? `\n${arg.stack}` : ''}`;
  }
  if (typeof arg === 'object' && arg !== null) {
    try {
      return JSON.stringify(arg);
    } catch {
      return String(arg);
    }
  }
  return String(arg);
}

let isSending = false;

function sendLogToServer(level, message) {
  if (isSending) return;
  isSending = true;

  const logData = {
    level: level,
    message: message,
    timestamp: new Date().toISOString(),
    host: window.location.hostname,
    userAgent: navigator.userAgent,
    path: window.location.pathname,
  };

  api
    .logs(logData)
    .catch((e) => {
      originalConsole.error('Failed to send log to server:', e);
    })
    .finally(() => {
      isSending = false;
    });
}

const LEVELS = ['log', 'error', 'warn', 'info'];

for (const level of LEVELS) {
  console[level] = function (...args) {
    originalConsole[level](...args);
    sendLogToServer(level, args.map(serializeArg).join(' '));
  };
}

window.onerror = function (message, source, lineno, colno, error) {
  const errorMessage = error ? `${error.name}: ${error.message}\n${error.stack ?? ''}` : String(message);
  sendLogToServer('error', `Uncaught error: ${errorMessage} at ${source}:${lineno}:${colno}`);
  return true; // do not show error
};

window.addEventListener('unhandledrejection', function (event) {
  sendLogToServer('error', `Unhandled rejection: ${serializeArg(event.reason)}`);
});

window.addEventListener(
  'error',
  function (event) {
    if (event.target && event.target !== window) {
      sendLogToServer('error', `Resource failed to load: ${event.target.src || event.target.href}`);
    }
  },
  true
);

if ('serviceWorker' in navigator) {
  navigator.serviceWorker.addEventListener('message', function (event) {
    if (event.data?.type === 'SW_LOG') {
      const { level, message } = event.data;
      originalConsole[level]('[SW]', ...message);
      sendLogToServer(level, `[SW] ${Array.isArray(message) ? message.map(serializeArg).join(' ') : message}`);
    }
  });
}

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

const FLUSH_INTERVAL_MS = 3000;
const MAX_BATCH_SIZE = 25;
const MAX_QUEUE_SIZE = 200; // safety net if sending keeps failing

let queue = [];
let flushTimer = null;
let inFlight = false;

function buildCallerInfo() {
  const lines = new Error().stack?.split('\n');
  // [0]=Error [1]=buildCallerInfo [2]=console.X override [3]=actual caller
  const callerLine = lines?.[3];
  if (!callerLine) return {};

  const stripDomain = (url) => url.replace(/^https?:\/\/[^/]+/, '');

  // named: "at fnName (file:line:col)"
  const named = /at\s+(.+?)\s+\((.+):(\d+):\d+\)/.exec(callerLine);
  if (named) return { function: named[1], file: stripDomain(named[2]), line: Number.parseInt(named[3], 10) };
  // anonymous: "at file:line:col"
  const anon = /at\s+(.+):(\d+):\d+/.exec(callerLine);
  if (anon) return { file: stripDomain(anon[1]), line: Number.parseInt(anon[2], 10) };
  return null;
}

function buildLogEntry(level, message, callerInfo = {}) {
  return {
    level,
    message,
    timestamp: new Date().toISOString(),
    host: window.location.hostname,
    userAgent: navigator.userAgent,
    path: window.location.pathname,
    ...(callerInfo !== null && { function: callerInfo.function, file: callerInfo.file, line: callerInfo.line }),
  };
}

function enqueueLog(level, message, callerInfo = {}) {
  queue.push(buildLogEntry(level, message, callerInfo));

  if (queue.length > MAX_QUEUE_SIZE) {
    // keep the most recent logs, drop the oldest ones
    queue.splice(0, queue.length - MAX_QUEUE_SIZE);
  }

  if (queue.length >= MAX_BATCH_SIZE) {
    flush();
  } else if (!flushTimer) {
    flushTimer = setTimeout(flush, FLUSH_INTERVAL_MS);
  }
}

function flush(useBeacon = false) {
  if (flushTimer) {
    clearTimeout(flushTimer);
    flushTimer = null;
  }

  if (queue.length === 0) return;

  // Beacon: best-effort on page unload, we can't wait for an in-flight
  // request to finish, so this bypasses the inFlight check.
  if (useBeacon && navigator.sendBeacon) {
    const batch = queue;
    queue = [];
    const blob = new Blob([JSON.stringify(batch)], { type: 'application/json' });
    const sent = navigator.sendBeacon(api.logsPath(), blob);
    if (!sent) {
      originalConsole.error('Failed to send log batch via sendBeacon');
    }
    return;
  }

  // One request at a time: if a flush is already in flight (slow server),
  // postpone this one instead of piling up parallel requests.
  if (inFlight) {
    if (!flushTimer) {
      flushTimer = setTimeout(flush, FLUSH_INTERVAL_MS);
    }
    return;
  }

  const batch = queue;
  queue = [];
  inFlight = true;

  api
    .logs(batch)
    .catch((e) => {
      originalConsole.error('Failed to send log batch to server:', e);
      // put the batch back at the front of the queue to retry later
      queue = batch.concat(queue);
      if (queue.length > MAX_QUEUE_SIZE) {
        queue.splice(0, queue.length - MAX_QUEUE_SIZE);
      }
    })
    .finally(() => {
      inFlight = false;
      if (queue.length > 0 && !flushTimer) {
        flushTimer = setTimeout(flush, FLUSH_INTERVAL_MS);
      }
    });
}

const ERROR_LEVEL = 'error';
const LEVELS = ['log', ERROR_LEVEL, 'warn', 'info'];

for (const level of LEVELS) {
  console[level] = function (...args) {
    originalConsole[level](...args);
    const callerInfo = level === ERROR_LEVEL ? buildCallerInfo() : {};
    enqueueLog(level, args.map(serializeArg).join(' '), callerInfo);
  };
}

window.onerror = function (message, source, lineno, colno, error) {
  const errorMessage = error ? `${error.name}: ${error.message}\n${error.stack ?? ''}` : String(message);
  enqueueLog(ERROR_LEVEL, `Uncaught error: ${errorMessage} at ${source}:${lineno}:${colno}`);
  return true; // do not show error
};

window.addEventListener('unhandledrejection', function (event) {
  enqueueLog(ERROR_LEVEL, `Unhandled rejection: ${serializeArg(event.reason)}`);
});

window.addEventListener(
  ERROR_LEVEL,
  function (event) {
    if (event.target && event.target !== window) {
      enqueueLog(ERROR_LEVEL, `Resource failed to load: ${event.target.src || event.target.href}`);
    }
  },
  true
);

if ('serviceWorker' in navigator) {
  navigator.serviceWorker.addEventListener('message', function (event) {
    if (event.data?.type === 'SW_LOG') {
      const { level, message } = event.data;
      originalConsole[level]('[SW]', ...message);
      enqueueLog(level, `[SW] ${Array.isArray(message) ? message.map(serializeArg).join(' ') : message}`);
    }
  });
}

// Safety net: flush whenever the page is going away (tab switch, close,
// navigation), via sendBeacon which survives even as the page unloads.
window.addEventListener('pagehide', () => flush(true));
document.addEventListener('visibilitychange', () => {
  if (document.visibilityState === 'hidden') {
    flush(true);
  }
});

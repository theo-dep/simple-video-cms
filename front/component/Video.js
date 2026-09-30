import { html } from 'htm/preact';
import { useEffect, useRef } from 'preact/hooks';
import videojs from 'video.js';
import { api } from '../api.js';

import 'videojs-yt-style';
import 'videojs-mobile-ui';

export default function Video({ videoId }) {
  const videoRef = useRef(null);
  const playerRef = useRef(null);

  useEffect(() => {
    if (!videoRef.current) return;

    // Dispose any existing player first to prevent memory leaks
    if (playerRef.current) {
      playerRef.current.dispose();
      playerRef.current = null;
    }

    const existingPlayer = videojs.getPlayer(videoRef.current);
    if (existingPlayer) {
      existingPlayer.dispose();
    }

    playerRef.current = videojs(videoRef.current, {
      html5: {
        vhs: {
          // Safari 11.1 MSE playback stalls after the initial buffer: let
          // Safari use its native HLS stack, the worker re-attaches the
          // video session on sessionless requests (same path as iPhone)
          overrideNative: !videojs.browser.IS_SAFARI,
          withCredentials: false,
        },
        nativeVideoTracks: false,
        nativeAudioTracks: false,
      },
      fluid: true,
      preload: 'metadata',
      playbackRates: [0.25, 0.5, 0.75, 1, 1.5, 2],
    });

    const player = playerRef.current;

    // player.mobileUi();
    player.ytStyle();

    let videoSession = null;
    // Timestamp of the last api call that refreshed the session server side
    // (add, start, reset): the server expires a session after 60 s of inactivity
    let sessionApiAt = 0;

    async function ensureVideoSession() {
      if (!videoSession) {
        const response = await api.addVideoSession(videoId).catch((err) => console.error(err));
        videoSession = response?.json?.session ?? null;
        sessionApiAt = Date.now();
      }
      return videoSession;
    }

    (async () => {
      // The server requires a session before any segment request, but a hanging request offline must not block playback.
      await Promise.race([ensureVideoSession(), new Promise((resolve) => setTimeout(() => resolve(null), 3000))]);

      player.src({
        src: api.videoPlaylistPath(videoId, videoSession),
        type: 'application/x-mpegURL',
      });
    })();

    // After a pause longer than the server session duration, the session is
    // expired: request a fresh one. The xhr hook and the worker send it with
    // the next segment requests, no player reload needed.
    const SESSION_RENEW_AFTER_MS = 50000;

    async function renewExpiredSession() {
      if (!videoSession || Date.now() - sessionApiAt < SESSION_RENEW_AFTER_MS) return;

      const response = await api.addVideoSession(videoId).catch((err) => console.error(err));
      const session = response?.json?.session ?? null;
      if (!session) return;

      videoSession = session;
      await api.startVideoSession(videoId, session).catch((err) => console.error(err));
      sessionApiAt = Date.now();
    }

    let isSessionStarted = false;
    async function ensureSessionStarted() {
      await renewExpiredSession();

      const session = await ensureVideoSession();
      if (!session) return;
      if (isSessionStarted) return;
      isSessionStarted = true;
      await api.startVideoSession(videoId, session).catch((err) => console.error(err));
      sessionApiAt = Date.now();
    }

    player.on('play', ensureSessionStarted);

    // Playback started offline has no session: create it when the connection
    // is back, the xhr hook below attaches it to the segment requests
    async function recoverVideoSession() {
      if (!videoSession) {
        await ensureSessionStarted();
        if (!videoSession) return;

        // Native HLS playback (e.g. iPhone) has no xhr hook: the playlist must
        // be loaded again for its segment uris to carry the session
        if (!player.tech({ IWillNotUseThisInPlugins: true }).vhs) {
          player.src({ src: api.videoPlaylistPath(videoId, videoSession), type: 'application/x-mpegURL' });
        }
      }
    }
    window.addEventListener('online', recoverVideoSession);

    // patch video.js to stop fetching a hls segment during seeking
    // this is made to synchronise the reset session api with seeking

    let isSeeking = false;
    let debounce = null;
    let lastBlocked = null;

    let originalVhsXhr = null;

    player.on('xhr-hooks-ready', () => {
      originalVhsXhr = player.tech({ IWillNotUseThisInPlugins: true }).vhs.xhr;

      player.tech({ IWillNotUseThisInPlugins: true }).vhs.xhr = function (options, callback) {
        // The playlist may carry an expired session (long pause): always send
        // the current one with segment requests
        let isSegment = false;
        if (options.uri) {
          const url = new URL(options.uri, location.href);
          isSegment = url.pathname.endsWith('.ts');
          if (isSegment && videoSession) {
            url.searchParams.set('session', videoSession);
            options.uri = url.href;
          }
        }

        if (isSeeking && isSegment) {
          lastBlocked = { options, callback };
          return {
            abort: () => {},
            addEventListener: () => {},
          };
        }

        return originalVhsXhr(options, callback);
      };
    });

    async function onSeekEnd() {
      if (videoSession) {
        await api.resetVideoSession(videoId, videoSession).catch((err) => console.error(err));
        sessionApiAt = Date.now();
      }
      isSeeking = false;

      if (lastBlocked && originalVhsXhr) {
        originalVhsXhr(lastBlocked.options, lastBlocked.callback);
        lastBlocked = null;
      }
    }

    player.on('seeking', () => {
      // Native HLS (iPhone, old Safari) has no blocked-xhr sync with the
      // reset api: fire the reset right away to stay ahead of the segment
      // request at the seek target
      if (!player.tech({ IWillNotUseThisInPlugins: true }).vhs && videoSession) {
        api.resetVideoSession(videoId, videoSession).catch((err) => console.error(err));
        sessionApiAt = Date.now();
      }

      isSeeking = true;
      clearTimeout(debounce);
      debounce = setTimeout(onSeekEnd, 300);
    });

    return () => {
      window.removeEventListener('online', recoverVideoSession);

      if (videoSession) {
        api.clearVideoSession(videoId, videoSession).catch((err) => console.error(err));
      }

      if (playerRef.current) {
        playerRef.current.dispose();
        playerRef.current = null;
      }
      clearTimeout(debounce);
    };
  }, [videoId]);

  return html`
    <video ref=${videoRef} onContextMenu=${(e) => e.preventDefault()} id="video-player" class="video-js vjs-default-skin" controls playsinline>
      <p class="vjs-no-js">
        To view this video please enable JavaScript and upgrade to a browser that
        <a href="https://videojs.com/html5-video-support/" target="_blank">supports HTML5 video</a>.
      </p>
    </video>
  `;
}

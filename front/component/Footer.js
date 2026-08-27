import { html } from 'htm/preact';

export function Footer() {
  return html`
    <div class="footer">
      <div class="footer-column">
        <p class="legal-license">
          This site is built with ❤️ using ${html``}
          <a href="https://preactjs.com/">preact</a>, <a href="https://yhirose.github.io/cpp-httplib/">cpp-httplib</a>, ${html``}
          <a href="https://videojs.com/">Video.js</a>, ${html``} <a href="https://www.ffmpeg.org/">FFmpeg</a>,
          <a href="https://sqliteorm.com/">SQLite ORM</a> and many awesome ${html``}
          <a href="https://gitlab.devau.co/theo/simple-video-cms/-/blob/prod/Readme.md#third-parties">libraries</a>.<br />
          All code on this site is licensed under the ${html``}
          <a href="https://gitlab.devau.co/theo/simple-video-cms/-/blob/prod/LICENSE">GPLv3</a> ${html``} unless otherwise stated.
        </p>
      </div>
      <div class="footer-column">
        <br />
        <p class="legal-link">
          <a href="https://gitlab.devau.co/theo/simple-video-cms">Open Source Project</a>
        </p>
      </div>
    </div>
  `;
}

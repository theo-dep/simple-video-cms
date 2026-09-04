import { html } from 'htm/preact';

export function Footer() {
  return html`
    <div class="footer">
      <div class="footer-column">
        <p class="legal-license">
          This site is built with ❤️ using ${html``}
          <!-- keep space -->
          <a href="https://preactjs.com/">preact</a>, ${html``}
          <!-- keep space -->
          <a href="https://yhirose.github.io/cpp-httplib/">cpp-httplib</a>, ${html``}
          <!-- keep space -->
          <a href="https://videojs.com/">Video.js</a>, ${html``}
          <!-- keep space -->
          <a href="https://www.ffmpeg.org/">FFmpeg</a>, ${html``}
          <!-- keep space -->
          <a href="https://sqliteorm.com/">SQLite ORM</a> and many awesome ${html``}
          <!-- keep space -->
          <a href="https://gitlab.devau.co/theo/simple-video-cms/-/blob/prod/Readme.md#third-parties">libraries</a>.
        </p>
        <p>
          All code on this site is licensed under the ${html``}
          <!-- keep space -->
          <a href="https://gitlab.devau.co/theo/simple-video-cms/-/blob/prod/LICENSE">GPLv3</a> unless otherwise stated.
        </p>
      </div>
      <div class="footer-column">
        <p class="legal-link">
          <a href="https://gitlab.devau.co/theo/simple-video-cms">Open Source Project</a>
        </p>
      </div>
    </div>
  `;
}

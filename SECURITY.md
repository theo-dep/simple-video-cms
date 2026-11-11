# Security Policy

## Supported Versions

Security updates are only provided for the latest release line of Simple Video CMS.
If you are running an older version, please upgrade first — the issue may already be fixed.

| Version | Supported          |
| ------- | ------------------ |
| 2.2.x   | :white_check_mark: |
| < 2.2   | :x:                |

## Reporting a Vulnerability

**Please do not open a public issue, merge request, or discussion for a security vulnerability.**

Use GitHub's private vulnerability reporting: go to the **Security** tab of this
repository and click **"Report a vulnerability"**. The report is visible only to
the project maintainers.

If you cannot use GitHub, open a **confidential issue** on the GitLab project
(gitlab.devau.co/theo/simple-video-cms) instead.

### What to include

- Affected version or commit, and how you deployed it (container image or bare metal).
- Description of the vulnerability and its potential impact.
- Steps to reproduce or a proof of concept.
- A suggested fix, if you have one.

### What to expect

- An acknowledgment of your report within **7 days**.
- A status update at least every **14 days** while the issue is being investigated.
- If the vulnerability is **accepted**: a fix will be developed, published as a new
  patch release, and you will be credited in the release notes unless you prefer to
  remain anonymous. Code scanning (CodeQL) results are also reviewed on every release.
- If the vulnerability is **declined**: we will explain why, and you are welcome to
  provide additional details.

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { parseHtml } from './html-parser.js';

describe('parseHtml', () => {
  const base = 'https://example.com';

  it('extracts manifest link', () => {
    const html = '<link rel="manifest" href="/manifest.json">';
    const result = parseHtml(html, base);
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.manifestUrl, 'https://example.com/manifest.json');
    }
  });

  it('extracts manifest link with double quotes and crossorigin', () => {
    const html = '<link rel="manifest" href="/app.webmanifest" crossorigin="use-credentials">';
    const result = parseHtml(html, base);
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.manifestUrl, 'https://example.com/app.webmanifest');
    }
  });

  it('extracts title', () => {
    const html = '<title>My Cool Site</title>';
    const result = parseHtml(html, base);
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.title, 'My Cool Site');
    }
  });

  it('extracts favicon', () => {
    const html = '<link rel="icon" href="/favicon.ico">';
    const result = parseHtml(html, base);
    assert.equal(result.success, true);
    if (result.success) {
      assert.deepEqual(result.data.icons, [
        { href: 'https://example.com/favicon.ico', sizes: undefined },
      ]);
    }
  });

  it('extracts apple-touch-icon with sizes', () => {
    const html = '<link rel="apple-touch-icon" sizes="180x180" href="/apple-icon.png">';
    const result = parseHtml(html, base);
    assert.equal(result.success, true);
    if (result.success) {
      assert.deepEqual(result.data.icons, [
        { href: 'https://example.com/apple-icon.png', sizes: '180x180' },
      ]);
    }
  });

  it('extracts theme-color meta', () => {
    const html = '<meta name="theme-color" content="#ff6600">';
    const result = parseHtml(html, base);
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.themeColor, '#ff6600');
    }
  });

  it('detects service worker registration in script', () => {
    const html = `<script>navigator.serviceWorker.register('/sw.js')</script>`;
    const result = parseHtml(html, base);
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.swHint, true);
    }
  });

  it('detects service worker registration with double quotes', () => {
    const html = `<script>navigator.serviceWorker.register("/service-worker.js")</script>`;
    const result = parseHtml(html, base);
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.swHint, true);
    }
  });

  it('returns null for missing fields', () => {
    const html = '<html><body>Hello</body></html>';
    const result = parseHtml(html, base);
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.manifestUrl, null);
      assert.equal(result.data.title, null);
      assert.equal(result.data.themeColor, null);
      assert.equal(result.data.swHint, false);
      assert.deepEqual(result.data.icons, []);
    }
  });

  it('resolves relative URLs against baseUrl', () => {
    const html = '<link rel="manifest" href="manifest.json"><link rel="icon" href="img/icon.png">';
    const result = parseHtml(html, 'https://example.com/app/');
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.manifestUrl, 'https://example.com/app/manifest.json');
      assert.equal(result.data.icons[0]?.href, 'https://example.com/app/img/icon.png');
    }
  });

  it('handles absolute manifest URLs', () => {
    const html = '<link rel="manifest" href="https://cdn.example.com/manifest.json">';
    const result = parseHtml(html, base);
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.manifestUrl, 'https://cdn.example.com/manifest.json');
    }
  });

  it('handles multiple icons', () => {
    const html = `
      <link rel="icon" sizes="16x16" href="/icon-16.png">
      <link rel="icon" sizes="32x32" href="/icon-32.png">
      <link rel="apple-touch-icon" sizes="180x180" href="/apple.png">
    `;
    const result = parseHtml(html, base);
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.icons.length, 3);
    }
  });
});

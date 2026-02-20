import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { extractMetadata } from './metadata-extractor.js';

describe('extractMetadata', () => {
  it('uses manifest name as primary source', () => {
    const result = extractMetadata({
      manifest: { name: 'Manifest Name', short_name: 'MN' },
      htmlMeta: {
        manifestUrl: null,
        title: 'HTML Title',
        icons: [],
        themeColor: null,
        swHint: false,
      },
      finalUrl: 'https://example.com',
    });
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.name, 'Manifest Name');
    }
  });

  it('falls back to short_name when name is missing', () => {
    const result = extractMetadata({
      manifest: { short_name: 'Short' },
      htmlMeta: {
        manifestUrl: null,
        title: 'Title',
        icons: [],
        themeColor: null,
        swHint: false,
      },
      finalUrl: 'https://example.com',
    });
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.name, 'Short');
    }
  });

  it('falls back to HTML title when manifest has no name', () => {
    const result = extractMetadata({
      manifest: null,
      htmlMeta: {
        manifestUrl: null,
        title: 'Page Title',
        icons: [],
        themeColor: null,
        swHint: false,
      },
      finalUrl: 'https://example.com',
    });
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.name, 'Page Title');
    }
  });

  it('falls back to hostname when nothing else available', () => {
    const result = extractMetadata({
      manifest: null,
      htmlMeta: {
        manifestUrl: null,
        title: null,
        icons: [],
        themeColor: null,
        swHint: false,
      },
      finalUrl: 'https://my-app.example.com/path',
    });
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.name, 'my-app.example.com');
    }
  });

  it('prefers 192px icon from manifest', () => {
    const result = extractMetadata({
      manifest: {
        icons: [
          { src: '/icon-48.png', sizes: '48x48' },
          { src: '/icon-192.png', sizes: '192x192' },
          { src: '/icon-512.png', sizes: '512x512' },
        ],
      },
      htmlMeta: {
        manifestUrl: null,
        title: null,
        icons: [{ href: 'https://example.com/favicon.ico', sizes: undefined }],
        themeColor: null,
        swHint: false,
      },
      finalUrl: 'https://example.com',
    });
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.iconUrl, '/icon-192.png');
    }
  });

  it('picks largest manifest icon when no 192px available', () => {
    const result = extractMetadata({
      manifest: {
        icons: [
          { src: '/icon-48.png', sizes: '48x48' },
          { src: '/icon-512.png', sizes: '512x512' },
        ],
      },
      htmlMeta: {
        manifestUrl: null,
        title: null,
        icons: [],
        themeColor: null,
        swHint: false,
      },
      finalUrl: 'https://example.com',
    });
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.iconUrl, '/icon-512.png');
    }
  });

  it('falls back to HTML favicon when manifest has no icons', () => {
    const result = extractMetadata({
      manifest: {},
      htmlMeta: {
        manifestUrl: null,
        title: null,
        icons: [{ href: 'https://example.com/favicon.ico', sizes: undefined }],
        themeColor: null,
        swHint: false,
      },
      finalUrl: 'https://example.com',
    });
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.iconUrl, 'https://example.com/favicon.ico');
    }
  });

  it('uses manifest display mode', () => {
    const result = extractMetadata({
      manifest: { display: 'fullscreen' },
      htmlMeta: {
        manifestUrl: null,
        title: null,
        icons: [],
        themeColor: null,
        swHint: false,
      },
      finalUrl: 'https://example.com',
    });
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.displayMode, 'fullscreen');
    }
  });

  it('defaults display to browser when no manifest', () => {
    const result = extractMetadata({
      manifest: null,
      htmlMeta: {
        manifestUrl: null,
        title: null,
        icons: [],
        themeColor: null,
        swHint: false,
      },
      finalUrl: 'https://example.com',
    });
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.displayMode, 'browser');
    }
  });

  it('extracts theme color from manifest over HTML', () => {
    const result = extractMetadata({
      manifest: { theme_color: '#111' },
      htmlMeta: {
        manifestUrl: null,
        title: null,
        icons: [],
        themeColor: '#222',
        swHint: false,
      },
      finalUrl: 'https://example.com',
    });
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.themeColor, '#111');
    }
  });

  it('falls back theme color to HTML meta', () => {
    const result = extractMetadata({
      manifest: {},
      htmlMeta: {
        manifestUrl: null,
        title: null,
        icons: [],
        themeColor: '#333',
        swHint: false,
      },
      finalUrl: 'https://example.com',
    });
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.themeColor, '#333');
    }
  });

  it('includes startUrl from manifest', () => {
    const result = extractMetadata({
      manifest: { start_url: '/app' },
      htmlMeta: {
        manifestUrl: null,
        title: null,
        icons: [],
        themeColor: null,
        swHint: false,
      },
      finalUrl: 'https://example.com',
    });
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.startUrl, '/app');
    }
  });
});

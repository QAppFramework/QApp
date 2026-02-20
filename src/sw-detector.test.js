import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { detectServiceWorker } from './sw-detector.js';

describe('detectServiceWorker', () => {
  /** @type {import('./html-parser.js').HtmlMeta} */
  const noSwMeta = {
    manifestUrl: null,
    title: null,
    icons: [],
    themeColor: null,
    swHint: false,
  };

  /** @type {import('./html-parser.js').HtmlMeta} */
  const swHintMeta = {
    manifestUrl: null,
    title: null,
    icons: [],
    themeColor: null,
    swHint: true,
  };

  it('detects SW from HTML hint', async () => {
    const result = await detectServiceWorker(swHintMeta, 'http://example.com');
    assert.equal(result.success, true);
    assert.equal(result.data.detected, true);
    assert.equal(result.data.method, 'html-hint');
  });

  it('reports no SW for plain site without hint', async () => {
    const result = await detectServiceWorker(noSwMeta, 'http://example.com');
    assert.equal(result.success, true);
    assert.equal(result.data.detected, false);
    assert.equal(result.data.method, 'none');
  });
});

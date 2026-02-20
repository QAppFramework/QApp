import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { validateManifest } from './manifest-schema.js';

describe('validateManifest', () => {
  it('accepts a minimal valid manifest', () => {
    const result = validateManifest({ name: 'My App' });
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.name, 'My App');
    }
  });

  it('accepts an empty object (all fields optional)', () => {
    const result = validateManifest({});
    assert.equal(result.success, true);
  });

  it('accepts a full manifest', () => {
    const manifest = {
      name: 'Test PWA',
      short_name: 'Test',
      start_url: '/',
      scope: '/',
      display: 'standalone',
      icons: [
        { src: '/icon-192.png', sizes: '192x192', type: 'image/png' },
        { src: '/icon-512.png', sizes: '512x512', type: 'image/png' },
      ],
      theme_color: '#ffffff',
      background_color: '#000000',
      orientation: 'portrait',
      dir: 'ltr',
      lang: 'en',
      id: '/app',
      shortcuts: [{ name: 'Open', url: '/open' }],
    };
    const result = validateManifest(manifest);
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.name, 'Test PWA');
      assert.equal(result.data.icons?.length, 2);
      assert.equal(result.data.display, 'standalone');
    }
  });

  it('strips unknown fields', () => {
    const result = validateManifest({ name: 'App', unknown_field: 42 });
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal('unknown_field' in result.data, false);
    }
  });

  it('rejects non-object input', () => {
    const result = validateManifest('not an object');
    assert.equal(result.success, false);
  });

  it('rejects null input', () => {
    const result = validateManifest(null);
    assert.equal(result.success, false);
  });

  it('rejects name as number', () => {
    const result = validateManifest({ name: 42 });
    assert.equal(result.success, false);
  });

  it('rejects icons as non-array', () => {
    const result = validateManifest({ icons: 'not-array' });
    assert.equal(result.success, false);
  });

  it('accepts display values from spec', () => {
    for (const display of ['fullscreen', 'standalone', 'minimal-ui', 'browser']) {
      const result = validateManifest({ display });
      assert.equal(result.success, true, `display: ${display} should be valid`);
    }
  });

  it('rejects invalid display value', () => {
    const result = validateManifest({ display: 'invalid' });
    assert.equal(result.success, false);
  });
});

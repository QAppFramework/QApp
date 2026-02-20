import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { fetchManifest } from './manifest-fetcher.js';

describe('fetchManifest', () => {
  it('returns error for non-existent manifest URL', async () => {
    const result = await fetchManifest('http://example.com/nonexistent-manifest.json');
    assert.equal(result.success, false);
  });

  it('returns error for unreachable host', async () => {
    const result = await fetchManifest('http://no-such-host-xyz.invalid/manifest.json');
    assert.equal(result.success, false);
    if (!result.success) {
      assert.equal(typeof result.error, 'string');
    }
  });

  it('returns error when manifest is not valid JSON', async () => {
    // example.com returns HTML, not JSON
    const result = await fetchManifest('http://example.com');
    assert.equal(result.success, false);
  });
});

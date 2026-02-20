import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { classifyUrl } from './classify-pipeline.js';

describe('classifyUrl', () => {
  it('rejects invalid URL format', async () => {
    const result = await classifyUrl('not-a-url');
    assert.equal(result.success, false);
    if (!result.success) {
      assert.match(result.error, /Invalid URL/i);
    }
  });

  it('returns error for unreachable host', async () => {
    const result = await classifyUrl('http://no-such-host-xyz.invalid');
    assert.equal(result.success, false);
  });

  it('classifies example.com as WS (no manifest)', async () => {
    const result = await classifyUrl('http://example.com');
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.classification.level, 'WS');
      assert.equal(result.data.classification.hasManifest, false);
      assert.equal(typeof result.data.metadata.name, 'string');
      assert.equal(typeof result.data.finalUrl, 'string');
    }
  });
});

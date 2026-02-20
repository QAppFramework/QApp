import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { checkReachability } from './url-reachability.js';

describe('checkReachability', () => {
  it('reports example.com as reachable via http', async () => {
    const result = await checkReachability('http://example.com');
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.reachable, true);
      assert.equal(typeof result.data.statusCode, 'number');
      assert.equal(typeof result.data.finalUrl, 'string');
    }
  });

  it('populates all fields on success', async () => {
    const result = await checkReachability('http://example.com');
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(typeof result.data.reachable, 'boolean');
      assert.equal(typeof result.data.finalUrl, 'string');
      assert.equal(typeof result.data.isHttps, 'boolean');
      assert.equal(typeof result.data.statusCode, 'number');
      assert.equal(typeof result.data.redirected, 'boolean');
    }
  });

  it('returns error for unreachable host', async () => {
    const result = await checkReachability('http://this-domain-does-not-exist-xyz123.invalid');
    assert.equal(result.success, false);
    if (!result.success) {
      assert.equal(typeof result.error, 'string');
    }
  });

  it('returns error for invalid URL', async () => {
    const result = await checkReachability('not-a-url');
    assert.equal(result.success, false);
  });
});

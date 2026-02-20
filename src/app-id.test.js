import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { generateAppId } from './app-id.js';

describe('generateAppId', () => {
  describe('valid URLs', () => {
    /** @type {Array<[string, string]>} */
    const cases = [
      ['https://github.com', 'github-com'],
      ['http://github.com', 'github-com'],
      ['https://www.google.com', 'www-google-com'],
      ['http://localhost:3000', 'localhost-3000'],
      ['https://example.co.uk', 'example-co-uk'],
      ['https://EXAMPLE.COM', 'example-com'],
      ['https://example.com/path?q=1#frag', 'example-com'],
      ['http://192.168.1.1:8080', '192-168-1-1-8080'],
      ['https://sub.domain.example.com', 'sub-domain-example-com'],
    ];

    for (const [url, expected] of cases) {
      it(`${url} → ${expected}`, () => {
        const result = generateAppId(url);
        assert.equal(result.success, true);
        if (result.success) {
          assert.equal(result.data, expected);
        }
      });
    }
  });

  it('ignores default ports (80, 443)', () => {
    // URL constructor strips default ports
    const r1 = generateAppId('http://example.com:80');
    assert.equal(r1.success, true);
    if (r1.success) assert.equal(r1.data, 'example-com');

    const r2 = generateAppId('https://example.com:443');
    assert.equal(r2.success, true);
    if (r2.success) assert.equal(r2.data, 'example-com');
  });

  describe('invalid inputs', () => {
    const invalid = [
      ['', 'Invalid URL'],
      ['not-a-url', 'Invalid URL'],
      ['ftp://example.com', 'URL must use http or https protocol'],
    ];

    for (const [input, expectedError] of invalid) {
      it(`rejects "${String(input)}"`, () => {
        const result = generateAppId(/** @type {string} */ (input));
        assert.equal(result.success, false);
        if (!result.success) {
          assert.equal(result.error, expectedError);
        }
      });
    }
  });

  it('same host always produces same ID', () => {
    const r1 = generateAppId('https://example.com/page1');
    const r2 = generateAppId('https://example.com/page2?q=1');
    assert.equal(r1.success, true);
    assert.equal(r2.success, true);
    if (r1.success && r2.success) {
      assert.equal(r1.data, r2.data);
    }
  });
});

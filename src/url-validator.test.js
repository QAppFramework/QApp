import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { isValidUrl } from './url-validator.js';

describe('isValidUrl', () => {
  describe('valid URLs', () => {
    const valid = [
      'https://example.com',
      'http://example.com',
      'https://example.com/path/to/page',
      'https://example.com?query=value',
      'https://example.com#fragment',
      'https://sub.domain.example.com',
      'https://example.museum',
      'https://example.co.uk',
      'https://example.com/path?q=1&r=2#top',
    ];

    for (const url of valid) {
      it(`accepts ${url}`, () => {
        assert.equal(isValidUrl(url), true);
      });
    }
  });

  describe('invalid URLs', () => {
    const invalid = [
      '',
      'example.com',
      'ftp://example.com',
      'https://',
      'https://example',
      'https://example .com',
      'just some text',
      'http://',
      '://example.com',
    ];

    for (const url of invalid) {
      it(`rejects "${url}"`, () => {
        assert.equal(isValidUrl(url), false);
      });
    }
  });

  describe('edge cases', () => {
    it('accepts URL with trailing slash', () => {
      assert.equal(isValidUrl('https://example.com/'), true);
    });

    it('accepts URL with deep path', () => {
      assert.equal(isValidUrl('https://example.com/a/b/c/d'), true);
    });

    it('accepts URL with query and fragment', () => {
      assert.equal(isValidUrl('https://example.com/p?k=v#sec'), true);
    });

    it('rejects URL with space in domain', () => {
      assert.equal(isValidUrl('https://exa mple.com'), false);
    });

    it('rejects bare protocol', () => {
      assert.equal(isValidUrl('https://'), false);
    });
  });
});

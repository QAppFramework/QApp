import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { resolveIcon, generateLetterIcon, DEFAULT_ICON } from './icon-resolver.js';

describe('generateLetterIcon', () => {
  it('generates SVG data URI from name', () => {
    const icon = generateLetterIcon('GitHub');
    assert.ok(icon.startsWith('data:image/svg+xml,'));
  });

  it('uses first letter uppercase', () => {
    const icon = generateLetterIcon('example');
    const svg = decodeURIComponent(icon.replace('data:image/svg+xml,', ''));
    assert.ok(svg.includes('>E<'), `SVG should contain letter E, got: ${svg}`);
  });

  it('handles empty name with fallback letter', () => {
    const icon = generateLetterIcon('');
    assert.ok(icon.startsWith('data:image/svg+xml,'));
    const svg = decodeURIComponent(icon.replace('data:image/svg+xml,', ''));
    assert.ok(svg.includes('>?<'));
  });

  it('produces deterministic color for same name', () => {
    const a = generateLetterIcon('test');
    const b = generateLetterIcon('test');
    assert.equal(a, b);
  });

  it('produces different colors for different names', () => {
    const a = generateLetterIcon('alpha');
    const b = generateLetterIcon('beta');
    assert.notEqual(a, b);
  });
});

describe('DEFAULT_ICON', () => {
  it('is a valid SVG data URI', () => {
    assert.ok(DEFAULT_ICON.startsWith('data:image/svg+xml,'));
  });
});

describe('resolveIcon', () => {
  it('returns existing iconUrl unchanged', async () => {
    const result = await resolveIcon('/icon.png', 'http://example.com', 'My App');
    assert.equal(result, '/icon.png');
  });

  it('probes /favicon.ico for null iconUrl', async () => {
    // example.com has a favicon.ico
    const result = await resolveIcon(null, 'http://example.com', 'Example');
    assert.equal(typeof result, 'string');
    assert.ok(result.length > 0);
  });

  it('returns letter icon or default for unreachable favicon', async () => {
    const result = await resolveIcon(
      null,
      'http://this-domain-does-not-exist-xyz123.invalid',
      'Test'
    );
    assert.ok(result.startsWith('data:image/svg+xml,'));
  });
});

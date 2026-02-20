import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { buildAppMetadata, validateAppMetadata } from './app-metadata.js';

describe('buildAppMetadata', () => {
  /** @returns {import('./app-metadata.js').AppMetadataInput} */
  function validInput() {
    return {
      appId: 'github-com',
      name: 'GitHub',
      url: 'https://github.com',
      level: 'WS',
      iconPath: '/home/user/.local/share/qapp/icons/github-com.png',
      desktopPath: '/home/user/.local/share/applications/qapp-github-com.desktop',
      wrapperPath: '/usr/bin/qapp-wrapper',
    };
  }

  it('builds metadata with correct fields', () => {
    const result = buildAppMetadata(validInput());
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.appId, 'github-com');
      assert.equal(result.data.name, 'GitHub');
      assert.equal(result.data.url, 'https://github.com');
      assert.equal(result.data.level, 'WS');
      assert.ok(result.data.installedAt.length > 0);
    }
  });

  it('generates an ISO 8601 timestamp', () => {
    const result = buildAppMetadata(validInput());
    assert.equal(result.success, true);
    if (result.success) {
      // Must parse as valid date
      const date = new Date(result.data.installedAt);
      assert.ok(!isNaN(date.getTime()));
    }
  });

  it('accepts all valid levels', () => {
    for (const level of ['WS', 'WAPP', 'PWAPP']) {
      const input = validInput();
      input.level = level;
      const result = buildAppMetadata(input);
      assert.equal(result.success, true);
    }
  });

  it('rejects empty appId', () => {
    const input = validInput();
    input.appId = '';
    const result = buildAppMetadata(input);
    assert.equal(result.success, false);
  });

  it('rejects empty name', () => {
    const input = validInput();
    input.name = '';
    const result = buildAppMetadata(input);
    assert.equal(result.success, false);
  });

  it('rejects invalid URL', () => {
    const input = validInput();
    input.url = 'not-a-url';
    const result = buildAppMetadata(input);
    assert.equal(result.success, false);
  });

  it('rejects invalid level', () => {
    const input = validInput();
    input.level = 'INVALID';
    const result = buildAppMetadata(input);
    assert.equal(result.success, false);
  });
});

describe('validateAppMetadata', () => {
  it('validates correct metadata', () => {
    const data = {
      appId: 'github-com',
      name: 'GitHub',
      url: 'https://github.com',
      level: 'WS',
      iconPath: '/path/to/icon.png',
      desktopPath: '/path/to/desktop',
      wrapperPath: '/usr/bin/qapp-wrapper',
      installedAt: '2026-02-20T12:00:00.000Z',
    };
    const result = validateAppMetadata(data);
    assert.equal(result.success, true);
  });

  it('rejects incomplete metadata', () => {
    const result = validateAppMetadata({ appId: 'test' });
    assert.equal(result.success, false);
  });

  it('rejects non-object', () => {
    const result = validateAppMetadata('not-an-object');
    assert.equal(result.success, false);
  });
});

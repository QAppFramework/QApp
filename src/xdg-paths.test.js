import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { homedir } from 'node:os';
import { join } from 'node:path';
import { resolveAppPaths } from './xdg-paths.js';

const HOME = homedir();
const LOCAL_SHARE = join(HOME, '.local', 'share');

describe('resolveAppPaths', () => {
  it('resolves correct paths for a standard app ID', () => {
    const result = resolveAppPaths('github-com');
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(
        result.data.desktopFile,
        join(LOCAL_SHARE, 'applications', 'qapp-github-com.desktop')
      );
      assert.equal(result.data.iconFile, join(LOCAL_SHARE, 'qapp', 'icons', 'github-com.png'));
      assert.equal(result.data.metadataFile, join(LOCAL_SHARE, 'qapp', 'apps', 'github-com.json'));
    }
  });

  it('uses svg extension when specified', () => {
    const result = resolveAppPaths('example-com', 'svg');
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.iconFile, join(LOCAL_SHARE, 'qapp', 'icons', 'example-com.svg'));
    }
  });

  it('uses ico extension when specified', () => {
    const result = resolveAppPaths('example-com', 'ico');
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.iconFile, join(LOCAL_SHARE, 'qapp', 'icons', 'example-com.ico'));
    }
  });

  it('includes directory paths for mkdir', () => {
    const result = resolveAppPaths('test-app');
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.iconsDir, join(LOCAL_SHARE, 'qapp', 'icons'));
      assert.equal(result.data.appsDir, join(LOCAL_SHARE, 'qapp', 'apps'));
      assert.equal(result.data.applicationsDir, join(LOCAL_SHARE, 'applications'));
    }
  });

  it('handles app ID with port', () => {
    const result = resolveAppPaths('localhost-3000');
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(
        result.data.desktopFile,
        join(LOCAL_SHARE, 'applications', 'qapp-localhost-3000.desktop')
      );
    }
  });

  it('rejects empty app ID', () => {
    const result = resolveAppPaths('');
    assert.equal(result.success, false);
    if (!result.success) {
      assert.equal(result.error, 'App ID must not be empty');
    }
  });
});

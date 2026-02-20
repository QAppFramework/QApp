import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { stat } from 'node:fs/promises';
import { homedir } from 'node:os';
import { join } from 'node:path';
import { installApp } from './app-installer.js';
import { uninstallApp } from './app-uninstaller.js';

const HOME = homedir();
const localShare = join(HOME, '.local', 'share');

/**
 * Install a test app and return metadata.
 */
async function installTestApp() {
  const svg =
    '<svg xmlns="http://www.w3.org/2000/svg"><rect width="10" height="10" fill="blue"/></svg>';
  const result = await installApp(
    {
      metadata: {
        name: 'Uninstall Test',
        iconUrl: 'data:image/svg+xml,' + encodeURIComponent(svg),
      },
      classification: { level: 'WS' },
      finalUrl: 'https://uninstall-test.example.com',
    },
    { wrapperPath: '/usr/bin/qapp-wrapper' }
  );
  assert.equal(result.success, true);
  return result;
}

describe('uninstallApp', () => {
  it('removes all installed files', async () => {
    const installResult = await installTestApp();
    if (!installResult.success) return;

    const appId = installResult.data.appId;
    const desktopPath = installResult.data.desktopPath;
    const iconPath = installResult.data.iconPath;
    const metaPath = join(localShare, 'qapp', 'apps', `${appId}.json`);

    // Verify files exist before uninstall
    assert.ok((await stat(desktopPath)).isFile());
    assert.ok((await stat(iconPath)).isFile());
    assert.ok((await stat(metaPath)).isFile());

    // Uninstall
    const result = await uninstallApp(appId);
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.removed.length, 3);
    }

    // Verify files are gone
    await assert.rejects(stat(desktopPath));
    await assert.rejects(stat(iconPath));
    await assert.rejects(stat(metaPath));
  });

  it('returns error for non-existent app', async () => {
    const result = await uninstallApp('nonexistent-app-xyz');
    assert.equal(result.success, false);
    if (!result.success) {
      assert.ok(result.error.includes('not installed'));
    }
  });

  it('returns error for empty app ID', async () => {
    const result = await uninstallApp('');
    assert.equal(result.success, false);
    if (!result.success) {
      assert.equal(result.error, 'App ID must not be empty');
    }
  });
});

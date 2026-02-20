import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { installApp } from './app-installer.js';
import { uninstallApp } from './app-uninstaller.js';
import { listInstalledApps } from './app-lister.js';

/**
 * Install a test app with a unique URL.
 * @param {string} host
 * @param {string} name
 */
async function installTestApp(host, name) {
  const svg = '<svg xmlns="http://www.w3.org/2000/svg"><rect width="10" height="10"/></svg>';
  return installApp(
    {
      metadata: { name, iconUrl: 'data:image/svg+xml,' + encodeURIComponent(svg) },
      classification: { level: 'WS' },
      finalUrl: `https://${host}`,
    },
    { wrapperPath: '/usr/bin/qapp-wrapper' }
  );
}

describe('listInstalledApps', () => {
  it('returns empty array when no apps installed', async () => {
    const result = await listInstalledApps();
    assert.equal(result.success, true);
    if (result.success) {
      assert.ok(Array.isArray(result.data));
    }
  });

  it('lists installed apps sorted by name', async () => {
    const r1 = await installTestApp('zebra-test.example.com', 'Zebra Test');
    const r2 = await installTestApp('alpha-test.example.com', 'Alpha Test');
    assert.equal(r1.success, true);
    assert.equal(r2.success, true);

    const listResult = await listInstalledApps();
    assert.equal(listResult.success, true);

    if (listResult.success) {
      const testApps = listResult.data.filter(
        (a) => a.appId === 'zebra-test-example-com' || a.appId === 'alpha-test-example-com'
      );
      assert.equal(testApps.length, 2);
      assert.equal(testApps[0]?.name, 'Alpha Test');
      assert.equal(testApps[1]?.name, 'Zebra Test');
    }

    // Cleanup
    await uninstallApp('zebra-test-example-com');
    await uninstallApp('alpha-test-example-com');
  });
});

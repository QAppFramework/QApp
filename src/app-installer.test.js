import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { rm, readFile, stat } from 'node:fs/promises';
import { homedir } from 'node:os';
import { join } from 'node:path';
import { installApp } from './app-installer.js';

/**
 * Build a mock ClassifyResult with a data URI icon (no network needed).
 * @param {Partial<import('./app-installer.js').ClassifyResult>} [overrides]
 * @returns {import('./app-installer.js').ClassifyResult}
 */
function mockClassifyResult(overrides = {}) {
  const svg =
    '<svg xmlns="http://www.w3.org/2000/svg"><rect width="10" height="10" fill="red"/></svg>';
  return {
    metadata: {
      name: 'Test App',
      iconUrl: 'data:image/svg+xml,' + encodeURIComponent(svg),
    },
    classification: { level: 'WS' },
    finalUrl: 'https://example.com',
    ...overrides,
  };
}

describe('installApp', () => {
  const HOME = homedir();
  const localShare = join(HOME, '.local', 'share');

  it('installs app with correct files', async () => {
    const input = mockClassifyResult();
    const result = await installApp(input, { wrapperPath: '/usr/bin/qapp-wrapper' });

    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.appId, 'example-com');
      assert.equal(result.data.name, 'Test App');
      assert.equal(result.data.url, 'https://example.com');
      assert.equal(result.data.level, 'WS');
      assert.ok(result.data.installedAt.length > 0);

      // Verify files exist
      const desktopStat = await stat(result.data.desktopPath);
      assert.ok(desktopStat.isFile());

      const iconStat = await stat(result.data.iconPath);
      assert.ok(iconStat.isFile());

      // Verify .desktop content
      const desktopContent = await readFile(result.data.desktopPath, 'utf-8');
      assert.ok(desktopContent.includes('[Desktop Entry]'));
      assert.ok(desktopContent.includes('Name=Test App'));
      assert.ok(desktopContent.includes('Exec=/usr/bin/qapp-wrapper example-com https://example.com'));

      // Verify metadata JSON
      const metaPath = join(localShare, 'qapp', 'apps', 'example-com.json');
      const metaContent = await readFile(metaPath, 'utf-8');
      const meta = /** @type {{ appId: string }} */ (JSON.parse(metaContent));
      assert.equal(meta.appId, 'example-com');
    }

    // Cleanup
    await rm(join(localShare, 'applications', 'qapp-example-com.desktop'), { force: true });
    await rm(join(localShare, 'qapp', 'icons', 'example-com.svg'), { force: true });
    await rm(join(localShare, 'qapp', 'apps', 'example-com.json'), { force: true });
  });

  it('reinstall overwrites existing files', async () => {
    const input = mockClassifyResult();
    const opts = { wrapperPath: '/usr/bin/qapp-wrapper' };

    const r1 = await installApp(input, opts);
    assert.equal(r1.success, true);

    const r2 = await installApp(input, opts);
    assert.equal(r2.success, true);

    if (r2.success) {
      // Files still exist after reinstall
      const desktopStat = await stat(r2.data.desktopPath);
      assert.ok(desktopStat.isFile());
    }

    // Cleanup
    if (r2.success) {
      await rm(r2.data.desktopPath, { force: true });
      await rm(r2.data.iconPath, { force: true });
      const metaPath = join(localShare, 'qapp', 'apps', 'example-com.json');
      await rm(metaPath, { force: true });
    }
  });

  it('uses SVG extension for SVG data URI icons', async () => {
    const input = mockClassifyResult();
    const result = await installApp(input, { wrapperPath: '/usr/bin/qapp-wrapper' });

    assert.equal(result.success, true);
    if (result.success) {
      assert.ok(result.data.iconPath.endsWith('.svg'));

      // Cleanup
      await rm(result.data.desktopPath, { force: true });
      await rm(result.data.iconPath, { force: true });
      await rm(join(localShare, 'qapp', 'apps', 'example-com.json'), { force: true });
    }
  });

  it('returns error for invalid URL', async () => {
    const input = mockClassifyResult({ finalUrl: 'not-valid' });
    const result = await installApp(input, { wrapperPath: '/usr/bin/qapp-wrapper' });

    assert.equal(result.success, false);
    if (!result.success) {
      assert.ok(result.error.includes('App ID'));
    }
  });
});

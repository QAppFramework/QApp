import { describe, it, beforeEach, afterEach } from 'node:test';
import assert from 'node:assert/strict';
import { mkdtemp, rm, readFile, stat } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { downloadIcon } from './icon-downloader.js';

/** @type {string} */
let tempDir;

beforeEach(async () => {
  tempDir = await mkdtemp(join(tmpdir(), 'icon-dl-'));
});

afterEach(async () => {
  await rm(tempDir, { recursive: true, force: true });
});

describe('downloadIcon', () => {
  it('saves a URL-encoded SVG data URI', async () => {
    const svg = '<svg xmlns="http://www.w3.org/2000/svg"><rect width="10" height="10"/></svg>';
    const dataUri = 'data:image/svg+xml,' + encodeURIComponent(svg);
    const dest = join(tempDir, 'icon.svg');

    const result = await downloadIcon(dataUri, dest);
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.format, 'svg');
      assert.equal(result.data.path, dest);

      const content = await readFile(dest, 'utf-8');
      assert.equal(content, svg);
    }
  });

  it('saves a base64-encoded PNG data URI', async () => {
    // Minimal 1x1 transparent PNG
    const pngBase64 =
      'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAAC0lEQVQI12NgAAIABQABNjN9GQAAAAlwSFlzAAAWJQAAFiUBSVIk8AAAAA0lEQVQI12P4z8BQDwAEgAF/QualzQAAAABJRU5ErkJggg==';
    const dataUri = 'data:image/png;base64,' + pngBase64;
    const dest = join(tempDir, 'icon.png');

    const result = await downloadIcon(dataUri, dest);
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.format, 'png');
      const info = await stat(dest);
      assert.ok(info.size > 0);
    }
  });

  it('creates parent directories recursively', async () => {
    const svg = '<svg xmlns="http://www.w3.org/2000/svg"><rect/></svg>';
    const dataUri = 'data:image/svg+xml,' + encodeURIComponent(svg);
    const dest = join(tempDir, 'deep', 'nested', 'dir', 'icon.svg');

    const result = await downloadIcon(dataUri, dest);
    assert.equal(result.success, true);
    if (result.success) {
      const info = await stat(dest);
      assert.ok(info.isFile());
    }
  });

  it('returns error for empty source', async () => {
    const result = await downloadIcon('', join(tempDir, 'empty.png'));
    assert.equal(result.success, false);
    if (!result.success) {
      assert.equal(result.error, 'Icon source must not be empty');
    }
  });

  it('returns error for unreachable HTTP URL', async () => {
    const result = await downloadIcon(
      'http://127.0.0.1:1/nonexistent.png',
      join(tempDir, 'fail.png')
    );
    assert.equal(result.success, false);
    if (!result.success) {
      assert.ok(result.error.startsWith('Failed to save icon:'));
    }
  });
});

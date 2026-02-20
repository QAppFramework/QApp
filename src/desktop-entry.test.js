import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { generateDesktopEntry } from './desktop-entry.js';

describe('generateDesktopEntry', () => {
  it('generates a valid .desktop file', () => {
    const result = generateDesktopEntry({
      name: 'GitHub',
      exec: '/usr/bin/qapp-wrapper https://github.com',
      icon: '/home/user/.local/share/qapp/icons/github-com.png',
    });
    assert.equal(result.success, true);
    if (result.success) {
      assert.ok(result.data.startsWith('[Desktop Entry]\n'));
      assert.ok(result.data.includes('Type=Application'));
      assert.ok(result.data.includes('Name=GitHub'));
      assert.ok(result.data.includes('Exec=/usr/bin/qapp-wrapper https://github.com'));
      assert.ok(result.data.includes('Icon=/home/user/.local/share/qapp/icons/github-com.png'));
      assert.ok(result.data.includes('Terminal=false'));
      assert.ok(result.data.includes('StartupNotify=true'));
      assert.ok(result.data.includes('Categories=Network;WebBrowser;'));
      assert.ok(result.data.endsWith('\n'));
    }
  });

  it('includes Comment when provided', () => {
    const result = generateDesktopEntry({
      name: 'My App',
      exec: '/usr/bin/qapp-wrapper http://localhost:3000',
      icon: '/path/to/icon.png',
      comment: 'A web application',
    });
    assert.equal(result.success, true);
    if (result.success) {
      assert.ok(result.data.includes('Comment=A web application'));
    }
  });

  it('uses custom categories when provided', () => {
    const result = generateDesktopEntry({
      name: 'My App',
      exec: '/usr/bin/qapp-wrapper http://example.com',
      icon: '/path/to/icon.png',
      categories: 'Development;IDE;',
    });
    assert.equal(result.success, true);
    if (result.success) {
      assert.ok(result.data.includes('Categories=Development;IDE;'));
    }
  });

  it('handles special characters in name', () => {
    const result = generateDesktopEntry({
      name: "Lpm's App & Tools",
      exec: '/usr/bin/qapp-wrapper http://example.com',
      icon: '/path/to/icon.png',
    });
    assert.equal(result.success, true);
    if (result.success) {
      assert.ok(result.data.includes("Name=Lpm's App & Tools"));
    }
  });

  it('rejects empty name', () => {
    const result = generateDesktopEntry({
      name: '',
      exec: '/usr/bin/qapp-wrapper http://example.com',
      icon: '/path/to/icon.png',
    });
    assert.equal(result.success, false);
    if (!result.success) {
      assert.equal(result.error, 'Name is required');
    }
  });

  it('rejects empty exec', () => {
    const result = generateDesktopEntry({
      name: 'My App',
      exec: '',
      icon: '/path/to/icon.png',
    });
    assert.equal(result.success, false);
    if (!result.success) {
      assert.equal(result.error, 'Exec is required');
    }
  });

  it('rejects empty icon', () => {
    const result = generateDesktopEntry({
      name: 'My App',
      exec: '/usr/bin/qapp-wrapper http://example.com',
      icon: '',
    });
    assert.equal(result.success, false);
    if (!result.success) {
      assert.equal(result.error, 'Icon is required');
    }
  });
});

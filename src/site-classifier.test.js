import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { classifySite } from './site-classifier.js';

describe('classifySite', () => {
  it('classifies PWAPP when manifest and service worker present', () => {
    const result = classifySite({ manifest: { name: 'App' }, sw: true });
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.level, 'PWAPP');
      assert.equal(result.data.hasManifest, true);
      assert.equal(result.data.hasServiceWorker, true);
    }
  });

  it('classifies WAPP when manifest present but no service worker', () => {
    const result = classifySite({ manifest: { name: 'App' }, sw: false });
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.level, 'WAPP');
      assert.equal(result.data.hasManifest, true);
      assert.equal(result.data.hasServiceWorker, false);
    }
  });

  it('classifies WS when no manifest', () => {
    const result = classifySite({ manifest: null, sw: false });
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.level, 'WS');
      assert.equal(result.data.hasManifest, false);
      assert.equal(result.data.hasServiceWorker, false);
    }
  });

  it('classifies WS when manifest is null even with SW', () => {
    const result = classifySite({ manifest: null, sw: true });
    assert.equal(result.success, true);
    if (result.success) {
      assert.equal(result.data.level, 'WS');
      assert.equal(result.data.hasManifest, false);
      assert.equal(result.data.hasServiceWorker, true);
    }
  });

  it('includes description for each level', () => {
    const pwapp = classifySite({ manifest: { name: 'A' }, sw: true });
    const wapp = classifySite({ manifest: { name: 'B' }, sw: false });
    const ws = classifySite({ manifest: null, sw: false });

    assert.equal(pwapp.success, true);
    assert.equal(wapp.success, true);
    assert.equal(ws.success, true);

    if (pwapp.success && wapp.success && ws.success) {
      assert.equal(typeof pwapp.data.description, 'string');
      assert.equal(typeof wapp.data.description, 'string');
      assert.equal(typeof ws.data.description, 'string');
      assert.notEqual(pwapp.data.description, wapp.data.description);
    }
  });
});

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { APP_NAME } from './index.js';

describe('index', () => {
  it('exports APP_NAME as PWAApp', () => {
    assert.equal(APP_NAME, 'PWAApp');
  });
});

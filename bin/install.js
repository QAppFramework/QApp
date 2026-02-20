#!/usr/bin/env node

/**
 * CLI entry point for app installation.
 *
 * Usage: node bin/install.js <url> [--wrapper-path <path>]
 * Runs classifyUrl → installApp → JSON stdout.
 * Exit 0 on success, 1 on error.
 */

import { classifyUrl } from '../src/classify-pipeline.js';
import { installApp } from '../src/app-installer.js';

const args = process.argv.slice(2);
const url = args.find((a) => !a.startsWith('--'));

if (!url) {
  process.stderr.write('Usage: node bin/install.js <url> [--wrapper-path <path>]\n');
  process.exit(1);
}

// Parse --wrapper-path option
let wrapperPath = '/usr/bin/qapp-wrapper';
const wpIdx = args.indexOf('--wrapper-path');
if (wpIdx !== -1 && args[wpIdx + 1]) {
  wrapperPath = /** @type {string} */ (args[wpIdx + 1]);
}

// Step 1: Classify the URL
const classifyResult = await classifyUrl(url);
if (!classifyResult.success) {
  process.stdout.write(JSON.stringify({ error: classifyResult.error }, null, 2) + '\n');
  process.exit(1);
}

// Step 2: Install the app
const installResult = await installApp(classifyResult.data, { wrapperPath });
if (!installResult.success) {
  process.stdout.write(JSON.stringify({ error: installResult.error }, null, 2) + '\n');
  process.exit(1);
}

process.stdout.write(JSON.stringify(installResult.data, null, 2) + '\n');
process.exit(0);

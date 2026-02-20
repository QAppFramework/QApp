/**
 * App installer — orchestrates the full install flow.
 *
 * Steps: appId → paths → downloadIcon → buildMetadata → writeDesktop → writeMetadata.
 * @module app-installer
 */

import { writeFile, mkdir } from 'node:fs/promises';
import { generateAppId } from './app-id.js';
import { resolveAppPaths } from './xdg-paths.js';
import { generateDesktopEntry } from './desktop-entry.js';
import { buildAppMetadata } from './app-metadata.js';
import { downloadIcon } from './icon-downloader.js';

/** @typedef {import('./app-metadata.js').AppMetadata} AppMetadata */

/**
 * @typedef {{
 *   metadata: { name: string, iconUrl: string },
 *   classification: { level: string },
 *   finalUrl: string,
 * }} ClassifyResult
 */

/**
 * @typedef {{
 *   wrapperPath: string,
 * }} InstallOptions
 */

/**
 * Install a classified site as a standalone app.
 *
 * Orchestrates:
 * 1. Generate app ID from URL
 * 2. Resolve XDG paths
 * 3. Download/save icon
 * 4. Generate .desktop entry
 * 5. Write .desktop file
 * 6. Build and write metadata JSON
 *
 * @param {ClassifyResult} classifyResult - Output from classifyUrl pipeline.
 * @param {InstallOptions} options - Install options.
 * @returns {Promise<{ success: true, data: AppMetadata } | { success: false, error: string }>}
 */
export async function installApp(classifyResult, options) {
  // Step 1: Generate app ID
  const idResult = generateAppId(classifyResult.finalUrl);
  if (!idResult.success) {
    return { success: false, error: `App ID: ${idResult.error}` };
  }
  const appId = idResult.data;

  // Step 2: Determine icon extension from source
  const iconSource = classifyResult.metadata.iconUrl;
  const iconExt =
    iconSource.startsWith('data:image/svg') || iconSource.includes('.svg') ? 'svg' : 'png';

  // Step 3: Resolve paths
  const pathsResult = resolveAppPaths(appId, iconExt);
  if (!pathsResult.success) {
    return { success: false, error: `Paths: ${pathsResult.error}` };
  }
  const paths = pathsResult.data;

  // Step 4: Download/save icon
  const iconResult = await downloadIcon(iconSource, paths.iconFile);
  if (!iconResult.success) {
    return { success: false, error: `Icon: ${iconResult.error}` };
  }

  // Step 5: Generate .desktop entry
  const desktopResult = generateDesktopEntry({
    name: classifyResult.metadata.name,
    exec: `${options.wrapperPath} ${appId} ${classifyResult.finalUrl}`,
    icon: paths.iconFile,
    comment: `Web app: ${classifyResult.finalUrl}`,
  });
  if (!desktopResult.success) {
    return { success: false, error: `Desktop entry: ${desktopResult.error}` };
  }

  // Step 6: Write .desktop file
  try {
    await mkdir(paths.applicationsDir, { recursive: true });
    await writeFile(paths.desktopFile, desktopResult.data, 'utf-8');
  } catch (err) {
    const msg = err instanceof Error ? err.message : 'Unknown error';
    return { success: false, error: `Write .desktop: ${msg}` };
  }

  // Step 7: Build metadata
  const metaResult = buildAppMetadata({
    appId,
    name: classifyResult.metadata.name,
    url: classifyResult.finalUrl,
    level: classifyResult.classification.level,
    iconPath: paths.iconFile,
    desktopPath: paths.desktopFile,
    wrapperPath: options.wrapperPath,
  });
  if (!metaResult.success) {
    return { success: false, error: `Metadata: ${metaResult.error}` };
  }

  // Step 8: Write metadata JSON
  try {
    await mkdir(paths.appsDir, { recursive: true });
    await writeFile(paths.metadataFile, JSON.stringify(metaResult.data, null, 2) + '\n', 'utf-8');
  } catch (err) {
    const msg = err instanceof Error ? err.message : 'Unknown error';
    return { success: false, error: `Write metadata: ${msg}` };
  }

  return { success: true, data: metaResult.data };
}

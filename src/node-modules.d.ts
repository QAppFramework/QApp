/**
 * Minimal type declarations for Node.js built-in modules.
 *
 * We cannot use @types/node directly in jsconfig.json ("types": ["node"])
 * because it pulls in punycode.js via the type reference chain, and
 * checkJs: true then fails on that third-party JS file.
 *
 * This file provides just enough types for the modules we actually use.
 */

declare module 'node:fs/promises' {
  export function writeFile(path: string, data: string | Buffer, options?: string): Promise<void>;
  export function readFile(path: string, options: 'utf-8' | 'utf8'): Promise<string>;
  export function readFile(path: string): Promise<Buffer>;
  export function mkdir(
    path: string,
    options?: { recursive?: boolean }
  ): Promise<string | undefined>;
  export function rm(
    path: string,
    options?: { recursive?: boolean; force?: boolean }
  ): Promise<void>;
  export function stat(
    path: string
  ): Promise<{ isFile(): boolean; isDirectory(): boolean; size: number }>;
  export function readdir(path: string): Promise<string[]>;
  export function mkdtemp(prefix: string): Promise<string>;
}

declare module 'node:path' {
  export function join(...paths: string[]): string;
  export function dirname(path: string): string;
  export function resolve(...paths: string[]): string;
  export function basename(path: string, ext?: string): string;
}

declare module 'node:os' {
  export function homedir(): string;
  export function tmpdir(): string;
}

declare var Buffer: {
  from(data: string, encoding?: string): Buffer;
  from(data: ArrayBuffer): Buffer;
};

interface Buffer extends Uint8Array {}
